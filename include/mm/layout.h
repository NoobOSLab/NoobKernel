#ifndef __MM_LAYOUT_H__
#define __MM_LAYOUT_H__
#include <misc/stdint.h>
#include <mm/config.h>

extern char skernel[];
extern char ekernel[];

/*** 物理内存布局 ***/
//设备地址空间
#ifdef QEMU
#define UART0 (size_t)0x10000000
#define VIRTIO0 (size_t)0x10001000
#define CLINT (size_t)0x02000000
#define PLIC (size_t)0x0c000000
#endif

//内核地址空间
#define SBI_BASE PM_START
#define KERNEL_BASE PAGE_ALIGN_DOWN((size_t)skernel)
#define KERNEL_END PAGE_ALIGN_UP((size_t)ekernel)

#define EARLY_SLAB_BASE SLAB_ALIGN_UP(KERNEL_END + PAGE_SIZE)
#define EARLY_SLAB_END (EARLY_SLAB_BASE + EARLY_SLAB_SIZE)

#define BUDDY_SYSTEM_BASE BUDDY_ALIGN_UP(EARLY_SLAB_END)
#define BUDDY_SYSTEM_END (BUDDY_ALIGN_DOWN(PM_END))
#define BUDDY_BLOB_NUM                                                         \
	((BUDDY_SYSTEM_END - BUDDY_SYSTEM_BASE) / BUDDY_BLOB_SIZE)

/*** 虚拟内存布局 ***/
//用户地址空间
#define BASE_ADDRESS (0x1000ULL)
#define MMAP_BOTTOM (0x60000000ULL)
#define USTACK_BASE (0x80000000ULL)
#define USER_TOP (0x80000000ULL)
//内存地址空间
#define MEMORY_VM_OFFSET (0)
#define PM_START_VM (PM_START + MEMORY_VM_OFFSET)
#define PM_END_VM (PM_END + MEMORY_VM_OFFSET)
#define KERNEL_BASE_VM (KERNEL_BASE + MEMORY_VM_OFFSET)
#define KERNEL_END_VM (KERNEL_END + MEMORY_VM_OFFSET)
//设备地址空间
#define DEVICE_VM_OFFSET (VM_END - 0x80000000ULL)
#define UART0_VM (UART0 + DEVICE_VM_OFFSET)
#define VIRTIO0_VM (VIRTIO0 + DEVICE_VM_OFFSET)
#define CLINT_VM (CLINT + DEVICE_VM_OFFSET)
#define PLIC_VM (PLIC + DEVICE_VM_OFFSET)

// 特权级切换页
#define TRAMPOLINE (VM_END - PAGE_SIZE)
// 中断状态保存页
#define TRAPFRAME (TRAMPOLINE - PAGE_SIZE)

#endif