#ifndef __MM_BUDDY_H__
#define __MM_BUDDY_H__

#include <misc/stddef.h>

int buddy_init();
void *buddy_alloc(size_t size);
int buddy_free(void *ptr);

void buddy_test();
#endif /* __MM_BUDDY_H__ */