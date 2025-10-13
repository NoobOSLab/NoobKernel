#ifndef __MISC_COMPLIER_H__
#define __MISC_COMPLIER_H__

#include <misc/stdint.h>

/* 编译器屏障（防止编译器重排序） */
#define barrier() __asm__ __volatile__("" ::: "memory")

/* 内存屏障（全屏障） */
static inline void mb(void) {
	__asm__ __volatile__("fence rw,rw" ::: "memory");
}

/* 读内存屏障 */
static inline void rmb(void) {
	__asm__ __volatile__("fence r,r" ::: "memory");
}

/* 写内存屏障 */
static inline void wmb(void) {
	__asm__ __volatile__("fence w,w" ::: "memory");
}

/* 读取 cycle CSR（需 M 模式或 S 模式允许） */
static inline uint64_t read_cycle(void) {
	uint64_t val;
	__asm__ __volatile__("rdcycle %0" : "=r"(val));
	return val;
}

/* 读取 time CSR（需 M 模式或 S 模式允许） */
static inline uint64_t read_time(void) {
	uint64_t val;
	__asm__ __volatile__("rdtime %0" : "=r"(val));
	return val;
}

/* 读取 instret CSR（指令计数） */
static inline uint64_t read_instret(void) {
	uint64_t val;
	__asm__ __volatile__("rdinstret %0" : "=r"(val));
	return val;
}

#define SEC(sec) __attribute__((section(sec), aligned(8)))

/* 原子操作封装（使用 GCC 内建原子函数） */
#define atomic_load(ptr) __atomic_load_n((ptr), __ATOMIC_SEQ_CST)
#define atomic_store(ptr, val) __atomic_store_n((ptr), (val), __ATOMIC_SEQ_CST)
#define atomic_add(ptr, val) __atomic_add_fetch((ptr), (val), __ATOMIC_SEQ_CST)
#define atomic_sub(ptr, val) __atomic_sub_fetch((ptr), (val), __ATOMIC_SEQ_CST)
#define atomic_and(ptr, val) __atomic_and_fetch((ptr), (val), __ATOMIC_SEQ_CST)
#define atomic_or(ptr, val) __atomic_or_fetch((ptr), (val), __ATOMIC_SEQ_CST)
#define atomic_xor(ptr, val) __atomic_xor_fetch((ptr), (val), __ATOMIC_SEQ_CST)

/* 比较并交换（Compare-and-Swap） */
#define atomic_cas(ptr, expected, desired)                                     \
	__atomic_compare_exchange_n((ptr), (expected), (desired), false,       \
				    __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

/* 编译时常量检测（C11 _Static_assert 的封装） */
#ifndef static_assert
#define static_assert _Static_assert
#endif

/* 防止函数内联 */
#define noinline __attribute__((noinline))

/* 强制内联 */
#define always_inline __attribute__((always_inline)) inline

/* 告知编译器某值可能被异步修改（如硬件寄存器） */
#define volatile_access(x) (*(volatile typeof(x) *)&(x))

/* 编译器提示：该分支很可能/不太可能执行 */
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

/* 空操作（用于占位或延迟） */
static inline void nop(void) {
	__asm__ __volatile__("nop");
}

static inline void wfi(void) {
	__asm__ __volatile__("wfi");
}

#endif