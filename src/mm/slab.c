#include <mm/slab.h>
#include <mm/config.h>
#include <mm/layout.h>
#include <misc/stdbool.h>
#include <misc/string.h>
#include <misc/errno.h>
#include <misc/log.h>
#include <misc/math.h>
#include <mm/buddy.h>
#include <mm/pm.h>
#include <misc/list.h>
#include <mm/kalloc.h>

extern bool buddy_inited;
bool slab_locked = false; // 是否锁定slab分配器

const u16 SLAB_MAGIC = 0x51AB; // 魔数，用于验证slab的有效性
static struct list_head slab_free_list[SLAB_MAX_ORDER - SLAB_MIN_ORDER + 1];

// (x + 7) / 8 + sizeof(struct slab) + (1 << order) * x = SLAB_BLOB_SIZE
// x + 7 + 8 * (1 << order) * x = 8 * (SLAB_BLOB_SIZE - sizeof(struct slab))
// x = (8 * (SLAB_BLOB_SIZE - sizeof(struct slab)) - 7) / (8 * (1 << order) + 1)
static size_t object_num(u8 order)
{
	if (order < SLAB_MIN_ORDER || order > SLAB_MAX_ORDER) {
		return 0;
	}
	size_t D = 8 * (1 << order) + 1;
	size_t N = 8 * (SLAB_BLOB_SIZE - sizeof(struct slab)) - 7;
	return N / D;
}

int slab_init()
{
	int i = 0, ret;
	for (i = SLAB_MIN_ORDER; i <= SLAB_MAX_ORDER; i++) {
		INIT_LIST_HEAD(&slab_free_list[i - SLAB_MIN_ORDER]);
	}
	i = 0;
	for (uintptr_t p = EARLY_SLAB_BASE; p < EARLY_SLAB_END;
	     p += SLAB_BLOB_SIZE) {
		// tracef("slab_init: creating slab at %p", (void *)p);
		ret = slab_create((void *)p,
				  SLAB_MIN_ORDER + i % (SLAB_MAX_ORDER -
							SLAB_MIN_ORDER + 1));
		i++;
		if (ret < 0)
			return ret;
	}
	return 0;
}

int slab_create(void *addr, u8 order)
{
	if (addr == NULL || order < SLAB_MIN_ORDER || order > SLAB_MAX_ORDER ||
	    !SLAB_ALIGNED(addr)) {
		return -EINVAL;
	}
	for (uintptr_t p = 0; p < SLAB_BLOB_SIZE; p += PAGE_SIZE) {
		struct page *page = addr2page((void *)(p + (uintptr_t)addr));
		page->flags |= PMF_SLAB;
	}
	struct slab *s = (struct slab *)addr;
	u32 total = object_num(order);
	u32 bitmap_size = ALIGN_UP(object_num(order), 8) / 8; // 位图大小/Bytes
	u32 offset = (1 << order) *
		     (SLAB_BLOB_SIZE / (1 << order) - object_num(order));
	tracef("slab_create: creating slab at %p with order %u, total %u, offset %u, bitmap size %u",
	       addr, order, total, offset, bitmap_size);
	*s = (struct slab){
		.total = total,
		.free = total,
		.offset = offset,
		.order = order,
		.magic = SLAB_MAGIC,
	};
	memset(s->bitmap, 0, bitmap_size); // 初始化位图为0
	list_add_tail(&s->list, &slab_free_list[order - SLAB_MIN_ORDER]);
	tracef("slab_create: adding slab %p with order %u", s, order);
	return 0;
}

int slab_destroy(void *addr)
{
	if (addr == NULL || !SLAB_ALIGNED(addr) ||
	    ((uintptr_t)addr >= EARLY_SLAB_BASE &&
	     (uintptr_t)addr < EARLY_SLAB_END)) {
		return -EINVAL;
	}
	struct slab *s = (struct slab *)addr;
	if (s->magic != SLAB_MAGIC) {
		return -EINVAL; // 检查魔数
	}
	if (s->free != s->total) {
		return -EBUSY; // 不能释放未全部释放的slab
	}
	if (s->order < SLAB_MIN_ORDER || s->order > SLAB_MAX_ORDER) {
		return -EINVAL; // 无效的阶数
	}
	s->magic = 0;
	list_del(&s->list);
	for (uintptr_t p = 0; p < SLAB_BLOB_SIZE; p += PAGE_SIZE) {
		struct page *page = addr2page((void *)(p + (uintptr_t)addr));
		page->flags &= ~PMF_SLAB;
	}
	return 0;
}

void *slab_alloc(size_t size)
{
	if (size == 0 || size > (1 << SLAB_MAX_ORDER)) {
		return NULL;
	}
	u8 order = log2_ceil(size);
	if (order < SLAB_MIN_ORDER) {
		order = SLAB_MIN_ORDER;
	}
	tracef("slab_alloc: requested size %u, calculated order %d", size,
	       order);

	struct list_head *p;
	struct slab *s;
	void *addr = NULL;
	list_for_each (p, &slab_free_list[order - SLAB_MIN_ORDER]) {
		s = list_entry(p, struct slab, list);
		if (s->free == 0)
			continue;
		for (u32 i = 0; i < s->total; i++) {
			if ((s->bitmap[i / 8] & (1 << (i % 8))) == 0) {
				s->bitmap[i / 8] |= (1 << (i % 8));
				tracef("slab_alloc: allocated block %d in slab at %p for order %d",
				       i, s, order);
				s->free--;
				addr = (void *)((uintptr_t)s + s->offset +
						i * (0x1 << order));
				tracef("slab_alloc: returning address %p for order %d",
				       addr, order);
				goto out;
			}
		}
	}
out:
	if (p->next == &slab_free_list[order - SLAB_MIN_ORDER] && buddy_inited &&
	    !slab_locked) {
		slab_locked = true;
		tracef("slab_alloc: slab at %p with order %u is dangerously low"
		       " on free blocks (%d left), triggering buddy allocation",
		       s, s->order, s->free);
		void *new_slab = buddy_alloc(SLAB_BLOB_SIZE);
		if (new_slab == NULL) {
			infof("slab_alloc: buddy_alloc failed for new slab");
		} else {
			int ret = slab_create(new_slab, order);

			if (ret < 0) {
				infof("slab_alloc: slab_create failed for new"
				      "slab at %p with order %d",
				      new_slab, order);
				buddy_free(new_slab);
			}
		}
		slab_locked = false;
	}
	return addr;
}

int slab_free(void *ptr)
{
	if (ptr == NULL) {
		errorf("slab_free: null pointer");
		return -EINVAL;
	}

	struct slab *s = (struct slab *)(SLAB_ALIGN_DOWN((uintptr_t)ptr));
	if (s->magic != SLAB_MAGIC) {
		warnf("slab_free: invalid magic number at %p", s);
		return -EINVAL; // 检查魔数
	}
	if (s->free == s->total) {
		warnf("slab_free: slab at %p is already fully released", s);
		return -EBUSY; // 已经全部释放
	}
	int i = ((uintptr_t)ptr - (uintptr_t)s - s->offset) / (1 << s->order);
	if (i < s->total && s->bitmap[i / 8] & (1 << (i % 8))) {
		s->bitmap[i / 8] &= ~(1 << (i % 8)); // 清除位图
		s->free++;
	} else
		return -EINVAL;

	if (s->total == s->free) {
		bool allow_destroy = false;
		struct slab *t;
		struct list_head *p, *n;
		list_for_each_safe (p, n,
				    &slab_free_list[s->order - SLAB_MIN_ORDER]) {
			t = list_entry(p, struct slab, list);
			if (t->free == t->total) {
				if (allow_destroy)
					slab_destroy(t);
				else
					allow_destroy = true;
			}
		}
	}

	return 0; // 成功释放
}

void slab_print_free_list(u8 order)
{
	const int colors[] = { 92, 95, 96 };
	const int n = ARRAY_SIZE(colors);
	if(order < SLAB_MIN_ORDER || order > SLAB_MAX_ORDER)
		return;
	struct list_head *p;
	struct slab *s;
	printf("\x1b[%dm --- order: %d --- \n", colors[order % n], order);
	list_for_each (p, &slab_free_list[order - SLAB_MIN_ORDER]) {
		s = list_entry(p, struct slab, list);
		printf("  Addr: %p, Total: %u, Free: %u, Offset: %u\n", s,
		       s->total, s->free, s->offset);
	}
	printf("\x1b[0m");
}

void slabs_print()
{
	for (int i = SLAB_MIN_ORDER; i <= SLAB_MAX_ORDER; i++) {
		slab_print_free_list(i);
	}	
}

void slab_test()
{
	for (int order = SLAB_MIN_ORDER; order <= SLAB_MAX_ORDER; order++) {
		void **addr = (void *)kcalloc(10000, 8);
		for (int i = 0; i < 10000; i++) {
			addr[i] = slab_alloc(1 << order);
			if (addr[i] == NULL) {
				break;
			}
		}
		infof("slab_test: allocated %d blocks of order %d", 10000,
		       order);
		for (int i = 0; i < 10000; i++) {
			if (addr[i] != NULL) {
				slab_free(addr[i]);
			} else {
				break;
			}
		}
		infof("slab_test: freed %d blocks of order %d", 10000, order);
		kfree(addr);
	}
	infof("slab test finished");
};