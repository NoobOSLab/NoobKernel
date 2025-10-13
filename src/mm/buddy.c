#include <mm/buddy.h>
#include <mm/layout.h>
#include <misc/list.h>
#include <misc/math.h>
#include <mm/slab.h>
#include <misc/errno.h>
#include <mm/pm.h>
#include <misc/log.h>

bool buddy_inited = false;
size_t buddy_free_pages = 0;

struct mem_block {
	void *start;
	struct list_head list;
};

static struct list_head buddy_free_list[BUDDY_MAX_ORDER + 1];

static inline u8 size2order(size_t size)
{
	return (u8)(log2_ceil(size) - PAGE_SHIFT);
}

static inline void *get_buddy_addr(void *addr, u8 order)
{
	return (void *)((uintptr_t)addr ^ (PAGE_SIZE << order));
}

int buddy_init()
{
	for (int i = 0; i <= BUDDY_MAX_ORDER; i++) {
		INIT_LIST_HEAD(&buddy_free_list[i]);
	}
	for (uintptr_t p = BUDDY_SYSTEM_BASE; p < BUDDY_SYSTEM_END;
	     p += BUDDY_BLOB_SIZE) {
		struct page *pg = addr2page((void *)p);
		if (pg == NULL) {
			return -EINVAL;
		}
		struct mem_block *t = slab_alloc(sizeof(struct mem_block));
		if (t == NULL) {
			return -ENOMEM;
		}
		t->start = (void *)p;
		pg->private = t;
		pg->order = BUDDY_MAX_ORDER;
		buddy_free_pages += (1 << BUDDY_MAX_ORDER);
		list_add_tail(&t->list, &buddy_free_list[BUDDY_MAX_ORDER]);
	}
	buddy_inited = true;
	return 0;
};


static void merge_blocks(u8 order)
{
	struct list_head *p = NULL, *n;
	for (u8 i = 0; i < order; i++) {
		if (list_empty(&buddy_free_list[i]))
			continue;

		//这一层时间复杂度大概为O(nlogn)，可以优化成O(n)
		while (p != &buddy_free_list[i]) {
			struct mem_block *block, *buddy_block = NULL;
			struct page *page, *buddy_page;
			list_for_each_safe (p, n, &buddy_free_list[i]) {
				block = list_entry(p, struct mem_block, list);
				page = addr2page(block->start);
				buddy_page = addr2page(get_buddy_addr(
					block->start, page->order));
				if (buddy_page->private == NULL ||
				    buddy_page->order != i)
					continue;
				buddy_block = buddy_page->private;
				break;
			}
			if (buddy_block != NULL) {
				list_del(&block->list);
				list_del(&buddy_block->list);
				if ((uintptr_t)block->start >
				    (uintptr_t)buddy_block->start) {
					slab_free(block);
					page->private = NULL;
					page->order = BUDDY_MAX_ORDER + 1;
					buddy_page->order++;
					list_add(&buddy_block->list,
						 &buddy_free_list[buddy_page->order]);
				} else {
					slab_free(buddy_block);
					buddy_page->private = NULL;
					buddy_page->order = BUDDY_MAX_ORDER + 1;
					page->order++;
					list_add(&block->list,
						 &buddy_free_list[page->order]);
				}
			}
		}
	}
}

static void split_blocks(u8 order)
{
	for (u8 i = BUDDY_MAX_ORDER; i > order; i--) {
		if (list_empty(&buddy_free_list[i]))
			continue;
		struct mem_block *block =
			list_first_entry(&buddy_free_list[i], struct mem_block, list);
		struct page *page = addr2page(block->start);

		struct mem_block *buddy_block =
			slab_alloc(sizeof(struct mem_block));
		if (buddy_block == NULL) {
			return;
		}

		list_del(&block->list);
		page->order--;
		buddy_block->start = get_buddy_addr(block->start, page->order);
		struct page *buddy_page = addr2page(buddy_block->start);
		buddy_page->order = page->order;
		buddy_page->private = buddy_block;
		list_add_tail(&block->list, &buddy_free_list[i - 1]);
		list_add_tail(&buddy_block->list, &buddy_free_list[i - 1]);
	}
}

static void *buddy_alloc_inner(u8 order)
{
	void *addr = NULL;
	struct mem_block *block =
		list_first_entry(&buddy_free_list[order], struct mem_block, list);
	struct page *page = addr2page(block->start);

	page->private = NULL;
	addr = block->start;
	list_del(&block->list);
	slab_free(block);
	buddy_free_pages -= (1 << order);
	return addr;
}

void *buddy_alloc(size_t size)
{
	if (size < PAGE_SIZE || size > BUDDY_BLOB_SIZE) {
		errorf("buddy_alloc: invalid size %u", size);
		return NULL;
	}

	u8 order = size2order(size);
	if (!list_empty(&buddy_free_list[order])) {
		return buddy_alloc_inner(order);
	}
	debugf("no blocks avaliable, merging");
	merge_blocks(order);
	if (!list_empty(&buddy_free_list[order])) {
		return buddy_alloc_inner(order);
	}
	debugf("no blocks avaliable, spliting");
	split_blocks(order);
	if (!list_empty(&buddy_free_list[order])) {
		return buddy_alloc_inner(order);
	}
	return NULL;
}

int buddy_free(void *addr)
{
	struct page *page = addr2page(addr);
	if (page == NULL || !(page->flags & PMF_BUDDY) ||
	    page->order > BUDDY_MAX_ORDER || page->private != NULL)
		return -EINVAL;

	struct mem_block *block = slab_alloc(sizeof(struct mem_block));
	if (block == NULL)
		return -ENOMEM;

	block->start = addr;
	page->private = block;
	buddy_free_pages += (1 >> page->order);
	list_add(&block->list, &buddy_free_list[page->order]);
	return 0;
}

void buddy_print_free_list(u8 order)
{
	const int colors[] = { 92, 95, 96 };
	const int n = ARRAY_SIZE(colors);
	if (order > BUDDY_MAX_ORDER)
		return;
	printf("\x1b[%dmfree_list[%u]:", order % n, order);
	if (list_empty(&buddy_free_list[order])) {
		printf(" empty\x1b[0m\n");
		return;
	}
	struct list_head *p;
	int i = 0;
	list_for_each (p, &buddy_free_list[order]) {
		if (i % 8 == 0)
			printf("\n");
		printf("%p ", list_entry(p, struct mem_block, list)->start);
		i++;
	}
	printf("\x1b[0m\n");
}

void buddy_test(void)
{
	for (int order = 0; order < 12; order++) {
		void *addr[20];
		for (int i = 0; i < 20; i++) {
			addr[i] = buddy_alloc(PAGE_SIZE << order);
			if (addr[i] == NULL) {
				infof("%d: Failed to allocate block of order %d",
				      i, order);
			} else {
				infof("%d: Allocated block of order %d at address %p",
				      i, order, addr[i]);
			}
		}
		for (int i = 0; i < 20; i++) {
			if (addr[i] != NULL) {
				int ret = buddy_free(addr[i]);
				if (ret < 0) {
					infof("%d: Failed to free block at address %p: %s",
					      i, addr[i], strerror(ret));
				} else {
					infof("%d: Freed block at address %p",
					      i, addr[i]);
				}
			}
		}
	}
}