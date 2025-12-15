#pragma once

#if defined(QEMU)
#include <platform/qemu_virt.h>
#endif

#define LOG_LEVEL 3

#define TIMER_IRQ_HZ 100

#define PAGE_SHIFT 12
#define PAGE_SIZE (1ULL << PAGE_SHIFT)
#define PAGE_NUM (MEM_SIZE / PAGE_SIZE)

#define PM_END (PM_START + MEM_SIZE)

#define SLAB_MAX_OBJ_SIZE (8192)
#define SLAB_BLOB_SIZE (16 * PAGE_SIZE)

#define EARLY_HEAP_SIZE (PAGE_SIZE)

#define BUDDY_MAX_ORDER 11
#define BUDDY_BLOB_SIZE ((1ULL << BUDDY_MAX_ORDER) * PAGE_SIZE)

#define USTACK_SIZE (8 * PAGE_SIZE)
#define KSTACK_SIZE (PAGE_SIZE)
#define TRAP_PAGE_SIZE (PAGE_SIZE)
#define IDLE_STACK_SIZE (PAGE_SIZE)
#define BOOT_STACK_SIZE (PAGE_SIZE)

#ifndef __ASSEMBLY__

#include <misc/stddef.h>
extern char skernel[];
extern char ekernel[];

static inline uint64_t ns_to_cputime(uint64_t ns)
{
	return (ns * TIMEBASE_FREQ) / 1000000000ULL;
}

static inline uint64_t us_to_cputime(uint64_t us)
{
	return (us * TIMEBASE_FREQ) / 1000000ULL;
}

static inline uint64_t ms_to_cputime(uint64_t ms)
{
	return (ms * TIMEBASE_FREQ) / 1000ULL;
}

static inline uint64_t sec_to_cputime(uint64_t sec)
{
	return sec * TIMEBASE_FREQ;
}

static inline uint64_t cputime_to_ns(uint64_t cputime)
{
	return (cputime * 1000000000ULL) / TIMEBASE_FREQ;
}

static inline uint64_t cputime_to_us(uint64_t cputime)
{
	return (cputime * 1000000ULL) / TIMEBASE_FREQ;
}

static inline uint64_t cputime_to_ms(uint64_t cputime)
{
	return (cputime * 1000ULL) / TIMEBASE_FREQ;
}

#define SBI_BASE PM_START
#define KERNEL_BASE PAGE_ALIGN_DOWN((uintptr_t)skernel)
#define KERNEL_END PAGE_ALIGN_UP((uintptr_t)ekernel)

#define EARLY_HEAP_BASE PAGE_ALIGN_UP(KERNEL_END + PAGE_SIZE)
#define EARLY_HEAP_END (EARLY_HEAP_BASE + EARLY_HEAP_SIZE)

#define BUDDY_SYSTEM_BASE BUDDY_ALIGN_UP(EARLY_HEAP_END)
#define BUDDY_SYSTEM_END (BUDDY_ALIGN_DOWN(PM_END))
#define BUDDY_BLOB_NUM                                                         \
	((BUDDY_SYSTEM_END - BUDDY_SYSTEM_BASE) / BUDDY_BLOB_SIZE)

#define PAGE_ALIGN_UP(a) ALIGN_UP(a, PAGE_SIZE)
#define PAGE_ALIGN_DOWN(a) ALIGN_DOWN(a, PAGE_SIZE)
#define PAGE_ALIGNED(a) ALIGNED(a, PAGE_SIZE)

#define SLAB_ALIGN_UP(a) ALIGN_UP(a, SLAB_BLOB_SIZE)
#define SLAB_ALIGN_DOWN(a) ALIGN_DOWN(a, SLAB_BLOB_SIZE)
#define SLAB_ALIGNED(a) ALIGNED(a, SLAB_BLOB_SIZE)

#define BUDDY_ALIGN_UP(a) ALIGN_UP(a, BUDDY_BLOB_SIZE)
#define BUDDY_ALIGN_DOWN(a) ALIGN_DOWN(a, BUDDY_BLOB_SIZE)
#define BUDDY_ALIGNED(a) ALIGNED(a, BUDDY_BLOB_SIZE)

#define PID_MIN 2
#define PID_MAX INT32_MAX

#endif //__ASSEMBLY__