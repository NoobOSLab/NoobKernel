#ifndef __MM_SLAB_H__
#define __MM_SLAB_H__

#include <misc/stdint.h>
#include <misc/list.h>

//slab元数据存储在slab开头处
struct slab {
	struct list_head list;
	u32 total; // slab大小
	u32 free; // 剩余空闲块数
	u32 offset;
	u16 magic; // 魔数，用于验证
	u8 order; // slab的阶数
	u8 bitmap[]; // 位图
};

int slab_init();
int slab_create(void *addr, u8 order);
int slab_destroy(void *addr);
void *slab_alloc(size_t size);
int slab_free(void *addr);

void slabs_print();

void slab_test();

#endif // __MM_SLAB_H__