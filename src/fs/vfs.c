#include <fs/vfs.h>
#include <misc/log.h>
#include <misc/string.h>
#include <misc/errno.h>
#include <misc/complier.h>
#include <mm/kalloc.h>
#include <sync/spinlock.h>

static struct {
	struct list_head fs_list;
	struct list_head mount_list;
	struct list_head super_list;
	spinlock_t lock;
	struct mount *root_mount;
} vfs_state;

void vfs_init(void)
{
	INIT_LIST_HEAD(&vfs_state.fs_list);
	INIT_LIST_HEAD(&vfs_state.mount_list);
	INIT_LIST_HEAD(&vfs_state.super_list);
	vfs_state.lock = SPINLOCK_INITIALIZER("vfs");
	vfs_state.root_mount = NULL;

	super_init();
	inode_init();
	dentry_init();
	file_init();

	infof("vfs initialized");
}

int register_filesystem(struct file_system_type *fs)
{
	if (!fs || !fs->name || !fs->mount) {
		return -EINVAL;
	}

	spinlock_acquire(&vfs_state.lock);

	struct file_system_type *existing;
	list_for_each_entry(existing, &vfs_state.fs_list, fs_list)
	{
		if (strcmp(existing->name, fs->name) == 0) {
			spinlock_release(&vfs_state.lock);
			return -EEXIST;
		}
	}

	INIT_LIST_HEAD(&fs->fs_list);
	fs->fs_lock = SPINLOCK_INITIALIZER(fs->name);
	list_add(&fs->fs_list, &vfs_state.fs_list);

	spinlock_release(&vfs_state.lock);

	infof("filesystem '%s' registered", fs->name);
	return 0;
}

int unregister_filesystem(struct file_system_type *fs)
{
	if (!fs) {
		return -EINVAL;
	}

	spinlock_acquire(&vfs_state.lock);

	bool found = false;
	struct file_system_type *existing;
	list_for_each_entry(existing, &vfs_state.fs_list, fs_list)
	{
		if (existing == fs) {
			found = true;
			break;
		}
	}

	if (!found) {
		spinlock_release(&vfs_state.lock);
		return -ENOENT;
	}

	list_del(&fs->fs_list);
	spinlock_release(&vfs_state.lock);

	infof("filesystem '%s' unregistered", fs->name);
	return 0;
}

struct file_system_type *get_fs(const char *name)
{
	if (!name) {
		return NULL;
	}

	spinlock_acquire(&vfs_state.lock);

	struct file_system_type *fs;
	list_for_each_entry(fs, &vfs_state.fs_list, fs_list)
	{
		if (strcmp(fs->name, name) == 0) {
			spinlock_release(&vfs_state.lock);
			return fs;
		}
	}

	spinlock_release(&vfs_state.lock);
	return NULL;
}

struct super_block *vfs_mount(struct file_system_type *fs_type, dev_t dev,
			      void *data)
{
	if (!fs_type) {
		return PTR(-EINVAL);
	}

	struct super_block *sb = fs_type->mount(fs_type, dev, data);
	if (IS_ERR(sb)) {
		return sb;
	}

	spinlock_acquire(&vfs_state.lock);
	list_add(&sb->s_list, &vfs_state.super_list);
	spinlock_release(&vfs_state.lock);

	infof("mounted filesystem '%s' on device %d:%d", fs_type->name,
	      MAJOR(dev), MINOR(dev));
	return sb;
}

int vfs_umount(struct super_block *sb)
{
	if (!sb) {
		return -EINVAL;
	}

	spinlock_acquire(&vfs_state.lock);
	list_del(&sb->s_list);
	spinlock_release(&vfs_state.lock);

	if (sb->s_type && sb->s_type->kill_sb) {
		sb->s_type->kill_sb(sb);
	}

	infof("unmounted filesystem");
	return 0;
}

int vfs_mount_root(struct file_system_type *fs_type, dev_t dev)
{
	struct super_block *sb = vfs_mount(fs_type, dev, NULL);
	if (IS_ERR(sb)) {
		return PTR_ERR(sb);
	}

	struct mount *mnt = kmalloc(sizeof(struct mount));
	if (!mnt) {
		vfs_umount(sb);
		return -ENOMEM;
	}

	mnt->mnt_parent = NULL;
	mnt->mnt_mountpoint = sb->s_root;
	mnt->mnt_root = sb->s_root;
	mnt->mnt_sb = sb;
	INIT_LIST_HEAD(&mnt->mnt_mounts);
	INIT_LIST_HEAD(&mnt->mnt_child);
	mnt->mnt_lock = SPINLOCK_INITIALIZER("mount");
	mnt->mnt_flags = 0;

	spinlock_acquire(&vfs_state.lock);
	vfs_state.root_mount = mnt;
	list_add(&mnt->mnt_child, &vfs_state.mount_list);
	spinlock_release(&vfs_state.lock);

	infof("root filesystem mounted");
	return 0;
}

struct dentry *vfs_get_root(void)
{
	spinlock_acquire(&vfs_state.lock);
	struct dentry *root =
	    vfs_state.root_mount ? vfs_state.root_mount->mnt_root : NULL;
	spinlock_release(&vfs_state.lock);
	return root;
}

struct mount *vfs_get_root_mount(void)
{
	spinlock_acquire(&vfs_state.lock);
	struct mount *mnt = vfs_state.root_mount;
	spinlock_release(&vfs_state.lock);
	return mnt;
}

struct mount *vfs_lookup_mount(struct dentry *mountpoint)
{
	if (!mountpoint) {
		return NULL;
	}

	spinlock_acquire(&vfs_state.lock);

	struct mount *mnt;
	list_for_each_entry(mnt, &vfs_state.mount_list, mnt_child)
	{
		if (mnt->mnt_mountpoint == mountpoint) {
			spinlock_release(&vfs_state.lock);
			return mnt;
		}
	}

	spinlock_release(&vfs_state.lock);
	return NULL;
}

void vfs_mount_get(struct mount *mnt)
{
	if (!mnt) {
		return;
	}

	spinlock_acquire(&mnt->mnt_lock);
	mnt->mnt_refcnt++;
	spinlock_release(&mnt->mnt_lock);
}

void vfs_mount_put(struct mount *mnt)
{
	if (!mnt) {
		return;
	}

	spinlock_acquire(&mnt->mnt_lock);
	mnt->mnt_refcnt--;

	if (mnt->mnt_refcnt == 0) {
		spinlock_release(&mnt->mnt_lock);

		spinlock_acquire(&vfs_state.lock);
		list_del(&mnt->mnt_child);
		if (mnt->mnt_parent) {
			list_del(&mnt->mnt_mounts);
		}
		spinlock_release(&vfs_state.lock);

		if (mnt->mnt_sb) {
			super_put(mnt->mnt_sb);
		}

		if (mnt->mnt_mountpoint) {
			dentry_put(mnt->mnt_mountpoint);
		}

		if (mnt->mnt_root) {
			dentry_put(mnt->mnt_root);
		}

		kfree(mnt);
		return;
	}

	spinlock_release(&mnt->mnt_lock);
}

struct file *vfs_open(const char *path, u32 flags)
{
	if (!path) {
		return PTR(-EINVAL);
	}

	struct dentry *dentry = vfs_path_lookup(NULL, path, LOOKUP_FOLLOW);
	if (IS_ERR(dentry)) {
		if ((flags & O_CREAT) && PTR_ERR(dentry) == -ENOENT) {
			struct path parent;
			struct qstr last;
			int ret = vfs_path_parent(NULL, path, &parent, &last);
			if (ret < 0) {
				return PTR((long)ret);
			}

			if (!parent.dentry->d_inode ||
			    !S_ISDIR(parent.dentry->d_inode->i_mode)) {
				dentry_put(parent.dentry);
				return PTR(-ENOTDIR);
			}

			if (!parent.dentry->d_inode->i_op ||
			    !parent.dentry->d_inode->i_op->create) {
				dentry_put(parent.dentry);
				return PTR(-ENOSYS);
			}

			struct dentry *new_dentry = dentry_alloc(
			    parent.dentry, parent.dentry->d_sb, last.name);
			if (IS_ERR(new_dentry)) {
				dentry_put(parent.dentry);
				return PTR(PTR_ERR(new_dentry));
			}

			umode_t mode = S_IFREG | 0644;
			ret = parent.dentry->d_inode->i_op->create(
			    parent.dentry->d_inode, new_dentry, mode);
			if (ret < 0) {
				dentry_free(new_dentry);
				dentry_put(parent.dentry);
				return PTR((long)ret);
			}

			dentry_insert(new_dentry);
			dentry_put(parent.dentry);
			dentry = new_dentry;
		} else {
			return PTR((long)PTR_ERR(dentry));
		}
	}

	struct file *file = file_open(dentry, flags);
	if (IS_ERR(file)) {
		dentry_put(dentry);
		return file;
	}

	return file;
}

int vfs_close(struct file *file) { return file_close(file); }

ssize_t vfs_read(struct file *file, void *buf, size_t count)
{
	return file_read(file, buf, count);
}

ssize_t vfs_write(struct file *file, const void *buf, size_t count)
{
	return file_write(file, buf, count);
}

loff_t vfs_lseek(struct file *file, loff_t offset, int whence)
{
	return file_lseek(file, offset, whence);
}

int vfs_mkdir(const char *path, umode_t mode)
{
	if (!path) {
		return -EINVAL;
	}

	struct path parent;
	struct qstr last;
	int ret = vfs_path_parent(NULL, path, &parent, &last);
	if (ret < 0) {
		return ret;
	}

	if (!parent.dentry->d_inode ||
	    !S_ISDIR(parent.dentry->d_inode->i_mode)) {
		dentry_put(parent.dentry);
		return -ENOTDIR;
	}

	if (!parent.dentry->d_inode->i_op ||
	    !parent.dentry->d_inode->i_op->mkdir) {
		dentry_put(parent.dentry);
		return -ENOSYS;
	}

	struct dentry *dentry =
	    dentry_alloc(parent.dentry, parent.dentry->d_sb, last.name);
	if (IS_ERR(dentry)) {
		dentry_put(parent.dentry);
		return PTR_ERR(dentry);
	}

	mode = S_IFDIR | (mode & 0777);
	ret = parent.dentry->d_inode->i_op->mkdir(parent.dentry->d_inode,
						  dentry, mode);
	if (ret < 0) {
		dentry_free(dentry);
		dentry_put(parent.dentry);
		return ret;
	}

	dentry_insert(dentry);
	dentry_put(dentry);
	dentry_put(parent.dentry);

	return 0;
}

int vfs_rmdir(const char *path)
{
	if (!path) {
		return -EINVAL;
	}

	struct dentry *dentry = vfs_path_lookup(NULL, path, LOOKUP_FOLLOW);
	if (IS_ERR(dentry)) {
		return PTR_ERR(dentry);
	}

	if (!dentry->d_inode || !S_ISDIR(dentry->d_inode->i_mode)) {
		dentry_put(dentry);
		return -ENOTDIR;
	}

	if (!dentry->d_parent || !dentry->d_parent->d_inode) {
		dentry_put(dentry);
		return -ENOENT;
	}

	if (!dentry->d_parent->d_inode->i_op ||
	    !dentry->d_parent->d_inode->i_op->rmdir) {
		dentry_put(dentry);
		return -ENOSYS;
	}

	int ret = dentry->d_parent->d_inode->i_op->rmdir(
	    dentry->d_parent->d_inode, dentry);
	if (ret < 0) {
		dentry_put(dentry);
		return ret;
	}

	dentry_delete(dentry);
	return 0;
}

int vfs_unlink(const char *path)
{
	if (!path) {
		return -EINVAL;
	}

	struct dentry *dentry = vfs_path_lookup(NULL, path, LOOKUP_FOLLOW);
	if (IS_ERR(dentry)) {
		return PTR_ERR(dentry);
	}

	if (!dentry->d_inode) {
		dentry_put(dentry);
		return -ENOENT;
	}

	if (S_ISDIR(dentry->d_inode->i_mode)) {
		dentry_put(dentry);
		return -EISDIR;
	}

	if (!dentry->d_parent || !dentry->d_parent->d_inode) {
		dentry_put(dentry);
		return -ENOENT;
	}

	if (!dentry->d_parent->d_inode->i_op ||
	    !dentry->d_parent->d_inode->i_op->unlink) {
		dentry_put(dentry);
		return -ENOSYS;
	}

	int ret = dentry->d_parent->d_inode->i_op->unlink(
	    dentry->d_parent->d_inode, dentry);
	if (ret < 0) {
		dentry_put(dentry);
		return ret;
	}

	dentry_delete(dentry);
	return 0;
}

int vfs_create(const char *path, umode_t mode)
{
	if (!path) {
		return -EINVAL;
	}

	struct path parent;
	struct qstr last;
	int ret = vfs_path_parent(NULL, path, &parent, &last);
	if (ret < 0) {
		return ret;
	}

	if (!parent.dentry->d_inode ||
	    !S_ISDIR(parent.dentry->d_inode->i_mode)) {
		dentry_put(parent.dentry);
		return -ENOTDIR;
	}

	if (!parent.dentry->d_inode->i_op ||
	    !parent.dentry->d_inode->i_op->create) {
		dentry_put(parent.dentry);
		return -ENOSYS;
	}

	struct dentry *dentry =
	    dentry_alloc(parent.dentry, parent.dentry->d_sb, last.name);
	if (IS_ERR(dentry)) {
		dentry_put(parent.dentry);
		return PTR_ERR(dentry);
	}

	mode = S_IFREG | (mode & 0777);
	ret = parent.dentry->d_inode->i_op->create(parent.dentry->d_inode,
						   dentry, mode);
	if (ret < 0) {
		dentry_free(dentry);
		dentry_put(parent.dentry);
		return ret;
	}

	dentry_insert(dentry);
	dentry_put(dentry);
	dentry_put(parent.dentry);

	return 0;
}

int vfs_rename(const char *old_path, const char *new_path)
{
	if (!old_path || !new_path) {
		return -EINVAL;
	}

	struct dentry *old_dentry =
	    vfs_path_lookup(NULL, old_path, LOOKUP_FOLLOW);
	if (IS_ERR(old_dentry)) {
		return PTR_ERR(old_dentry);
	}

	struct path new_parent;
	struct qstr new_last;
	int ret = vfs_path_parent(NULL, new_path, &new_parent, &new_last);
	if (ret < 0) {
		dentry_put(old_dentry);
		return ret;
	}

	if (!new_parent.dentry->d_inode ||
	    !S_ISDIR(new_parent.dentry->d_inode->i_mode)) {
		dentry_put(old_dentry);
		dentry_put(new_parent.dentry);
		return -ENOTDIR;
	}

	struct dentry *new_dentry = dentry_alloc(
	    new_parent.dentry, new_parent.dentry->d_sb, new_last.name);
	if (IS_ERR(new_dentry)) {
		dentry_put(old_dentry);
		dentry_put(new_parent.dentry);
		return PTR_ERR(new_dentry);
	}

	if (!old_dentry->d_parent || !old_dentry->d_parent->d_inode) {
		dentry_free(new_dentry);
		dentry_put(old_dentry);
		dentry_put(new_parent.dentry);
		return -ENOENT;
	}

	if (!old_dentry->d_parent->d_inode->i_op ||
	    !old_dentry->d_parent->d_inode->i_op->rename) {
		dentry_free(new_dentry);
		dentry_put(old_dentry);
		dentry_put(new_parent.dentry);
		return -ENOSYS;
	}

	ret = old_dentry->d_parent->d_inode->i_op->rename(
	    old_dentry->d_parent->d_inode, old_dentry,
	    new_parent.dentry->d_inode, new_dentry);
	if (ret < 0) {
		dentry_free(new_dentry);
		dentry_put(old_dentry);
		dentry_put(new_parent.dentry);
		return ret;
	}

	dentry_insert(new_dentry);
	dentry_delete(old_dentry);
	dentry_put(new_dentry);
	dentry_put(new_parent.dentry);

	return 0;
}
