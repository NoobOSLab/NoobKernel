#ifndef __MM_KALLOC_H__
#define __MM_KALLOC_H__

#include <misc/stddef.h>

void *kalloc_page();
int kfree_page(void *addr);

void *kmalloc(size_t size);
void *kzalloc(size_t size);
void *kcalloc(size_t nitems, size_t size);
void *realloc(void *ptr, size_t size);
int kfree(void *ptr);

char *alloc_str(const char *str);

void kalloc_test();

#endif // __MM_KALLOC_H__