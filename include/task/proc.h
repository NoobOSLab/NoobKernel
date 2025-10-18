#ifndef __TASK_PROC_H__
#define __TASK_PROC_H__

#include <misc/list.h>
#include <misc/stddef.h>

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
	struct context context;
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
};

void set_cpu(u64 id);
struct cpu *thiscpu();

#endif