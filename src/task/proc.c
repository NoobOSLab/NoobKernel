#include <task/proc.h>
#include <config.h>
#include <hal/riscv.h>
#include <mm/kalloc.h>
#include <mm/pagetable.h>
#include <mm/vma.h>
#include <sync/atomic.h>

extern pagetable_t kpagetable;

static struct cpu cpus[CPU_NUM];
static char dummy_idle_stack[8] __attribute__((aligned(16)));
extern void idle_loop(void);
static atomic64_t next_pid = ATOMIC64_INIT(PID_MIN);

pid_t alloc_pid(void)
{
	int64_t current;
	int pid;

	do {
		current = atomic64_read(&next_pid);

		if (current > PID_MAX) {
			// 理论上几乎不可能发生（需分配 21 亿次）
			// 可选择 panic、回绕或返回错误
			// 这里简单回绕到 PID_MIN
			current = atomic64_cmpxchg(&next_pid, current, PID_MIN);
			// 重试
			continue;
		}

		// 尝试将 next_pid 从 current 增加到 current+1
		int64_t new_val = current + 1;
		if (atomic64_cmpxchg(&next_pid, current, new_val) == current) {
			pid = (int)
			    current; // safe: current <= PID_MAX = INT32_MAX
			break;
		}
		// 否则重试（其他 CPU 修改了 next_pid）

	} while (1);

	return pid;
}

void init_cpu(u64 id)
{
	cpus[id].ctx.ra = (uintptr_t)idle_loop;
	cpus[id].ctx.sp =
	    (uintptr_t)(dummy_idle_stack + sizeof(dummy_idle_stack));
	cpus[id].proc = NULL;
	w_tp(id);
}

inline struct cpu *thiscpu() { return &cpus[r_tp()]; }

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
