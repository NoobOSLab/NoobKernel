#ifndef __TASK_PROC_H__
#define __TASK_PROC_H__

#include <misc/list.h>
#include <misc/stddef.h>
#include <sync/spinlock.h>

#define PROC_UNUSED 0
#define PROC_EMBRYO 1
#define PROC_RUNNABLE 2
#define PROC_RUNNING 3
#define PROC_SLEEPING 4
#define PROC_ZOMBIE 5

struct context {
	u64 ra;
	u64 sp;

	// callee-saved
	u64 s0;
	u64 s1;
	u64 s2;
	u64 s3;
	u64 s4;
	u64 s5;
	u64 s6;
	u64 s7;
	u64 s8;
	u64 s9;
	u64 s10;
	u64 s11;
};

struct cpu {
	struct proc *proc;
	struct context ctx;
	u64 intr_state;
	int intr_depth;
	int id;
};

struct proc {
	char comm[16];
	pid_t pid;
	pid_t tgid;

	pagetable_t pagetable;
	struct list_head vma;

	struct context ctx;
	struct trapframe *tf;
	void *kstack;

	int state;
	struct list_head runq;

	struct proc *parent;
	struct list_head children;
	struct list_head sibling;

	spinlock_t lock;
};

void init_cpu(u64 id);
struct cpu *thiscpu();

struct proc *alloc_proc();
void free_proc(struct proc *p);

#endif