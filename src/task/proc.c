#include <task/proc.h>
#include <config.h>
#include <hal/riscv.h>
#include <mm/kalloc.h>
#include <mm/pagetable.h>
#include <mm/vma.h>
#include <misc/complier.h>

extern pagetable_t kpagetable;

static struct cpu cpus[CPU_NUM];
static char dummy_idle_stack[8] __attribute__((aligned(16)));
extern void idle_loop(void);
pid_t next_pid = 1;

pid_t alloc_pid(void)
{
	return atomic_add(&next_pid, 1);
}

void init_cpu(u64 id)
{
	cpus[id].id = id;
	cpus[id].ctx.ra = (uintptr_t)idle_loop;
	cpus[id].ctx.sp =
	    (uintptr_t)(dummy_idle_stack + sizeof(dummy_idle_stack));
	cpus[id].proc = NULL;
	// 将cpu结构体地址写入tp寄存器，方便快速访问
	w_tp((uintptr_t)&cpus[id]);
}

inline struct cpu *thiscpu() { return (void *)r_tp(); }

struct proc *alloc_proc()
{
	struct proc *p = kzalloc(sizeof(struct proc));
	if (!p)
		return NULL;

	p->state = PROC_UNUSED;
	return p;
}

// 调用前需确保进程不在调度队列中，且没有子进程
void free_proc(struct proc *p)
{
	if (!p)
		return;

	// 1. 从父进程的 children 链表中删除自己
	if (p->parent) {
		// 【关键】获取父进程锁（防止父进程同时 wait()）
		spinlock_acquire(&p->parent->lock);
		list_del(
		    &p->sibling); // p->sibling 是在 parent->children 中的节点
		spinlock_release(&p->parent->lock);
		p->parent = NULL;
	}

	// 2. 释放内核栈
	if (p->kstack)
		kfree(p->kstack);

	// 3. 释放 trapframe
	if (p->tf)
		kfree(p->tf);

	// 4. 释放VMA和页表（如果是用户进程）
	if (p->pagetable && p->pagetable != kpagetable) {
		struct list_head *pos, *n;
		// 遍历 VMA，解除映射
		list_for_each_safe(pos, n, &p->vma)
		{
			struct vma *vma = list_entry(pos, struct vma, list);
			vma_remove(p, vma->start, vma->length);
		}
		pagetable_destroy(p->pagetable);
	}

	// 5. 释放 proc 结构体
	kfree(p);
}
