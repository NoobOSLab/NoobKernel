#ifndef __MM_CONFIG_H__
#define __MM_CONFIG_H__

#include <misc/stddef.h>

#define KB ((size_t)1024)
#define MB (1024 * KB)
#define GB (1024 * MB)

#define MEM_SIZE (4 * GB)

#define PAGE_SHIFT 12
#define PAGE_SIZE (1ULL << PAGE_SHIFT)

#define PA2PTE(pa) ((((u64)pa) >> PAGE_SHIFT) << 10)
#define PTE2PA(pte) (((pte) >> 10) << PAGE_SHIFT)

#define PTE_V (1L << 0) // Valid - 页表项是否有效
#define PTE_R (1L << 1) // Readable - 允许读取
#define PTE_W (1L << 2) // Writable - 允许写入
#define PTE_X (1L << 3) // Executable - 允许执行
#define PTE_U (1L << 4) // User - 用户态是否可访问(1-允许,0-禁止)
#define PTE_G (1L << 5) // Global - 全局映射(TLB在切换地址空间时无需刷新该项)
#define PTE_A (1L << 6) // Accessed - 页是否被访问过
#define PTE_D (1L << 7) // Dirty - 页是否被修改过
#define PTE_FLAGS(pte) ((pte) & 0x3FF)
#define PXMASK 0x1FF // 9 bits
#define PXSHIFT(level) (PAGE_SHIFT + (9 * (level)))
#define PX(level, va) ((((u64)(va)) >> PXSHIFT(level)) & PXMASK)

#define PAGE_NUM (MEM_SIZE / PAGE_SIZE)
#define PAGE_ALIGN_UP(a) ALIGN_UP(a, PAGE_SIZE)
#define PAGE_ALIGN_DOWN(a) ALIGN_DOWN(a, PAGE_SIZE)
#define PAGE_ALIGNED(a) ALIGNED(a, PAGE_SIZE)

#if defined(QEMU)
#define PM_START (size_t)0x80000000
#elif defined(VF2)
#define PM_START (size_t)0x40000000
#endif
#define PM_END (PM_START + MEM_SIZE)

#define VM_START (size_t)0x00000000
#define VM_END (size_t)(1ULL << (9 + 9 + 9 + 12 - 1))

#define SLAB_MIN_ORDER 3
#define SLAB_MAX_ORDER 11
#define SLAB_BLOB_SIZE (MEM_SIZE / 2048)
#define SLAB_ALIGN_UP(a) ALIGN_UP(a, SLAB_BLOB_SIZE)
#define SLAB_ALIGN_DOWN(a) ALIGN_DOWN(a, SLAB_BLOB_SIZE)
#define SLAB_ALIGNED(a) ALIGNED(a, SLAB_BLOB_SIZE)

#define EARLY_SLAB_SIZE (SLAB_BLOB_SIZE * 16)

#define BUDDY_MAX_ORDER 11
#define BUDDY_BLOB_SIZE ((1ULL << BUDDY_MAX_ORDER) * PAGE_SIZE)
#define BUDDY_ALIGN_UP(a) ALIGN_UP(a, BUDDY_BLOB_SIZE)
#define BUDDY_ALIGN_DOWN(a) ALIGN_DOWN(a, BUDDY_BLOB_SIZE)
#define BUDDY_ALIGNED(a) ALIGNED(a, BUDDY_BLOB_SIZE)

#define USTACK_SIZE (8 * PAGE_SIZE)
#define KSTACK_SIZE (PAGE_SIZE)
#define TRAP_PAGE_SIZE (PAGE_SIZE)

#endif // __MM_CONFIG_H__