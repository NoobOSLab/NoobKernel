#ifndef __MM_PM_H__
#define __MM_PM_H__

#include <mm/config.h>

#define PMF_BUDDY 0x01
#define PMF_SLAB 0x02
#define PMF_STATIC 0x04 // 静态内存，不参与分配回收

struct page {
	u32 refs;
	u16 flags;
	u8 order;
	void *private;
};

int pm_init();
void print_pm_layout();
struct page *addr2page(void *addr);
void *page2addr(struct page *page);
int pm_page_get(void *addr);
int pm_page_put(void *addr);
void *page_alloc(u8 flags);
int page_free(void *ptr);
size_t get_free_pages_num();

#endif