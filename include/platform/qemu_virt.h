#ifndef __PLATFORM_QEMU_VIRT_H__
#define __PLATFORM_QEMU_VIRT_H__

#include <misc/stddef.h>

#define CPU_NUM 1

#define CACHE_LINE_SIZE 64

#define TIMEBASE_FREQ 10000000UL
#define MEM_SIZE (512 * 1024 * 1024)
#define PM_START (uintptr_t)0x80000000

#define UART0 (uintptr_t)0x10000000
#define VIRTIO0 (uintptr_t)0x10001000
#define CLINT (uintptr_t)0x02000000
#define PLIC (uintptr_t)0x0c000000

#endif