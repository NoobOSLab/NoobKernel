#include <misc/log.h>
#include <misc/string.h>
#include <mm/pm.h>
#include <misc/complier.h>
#include <trap/trap.h>
#include <hal/timer.h>
#include <mm/vm.h>
#include <mm/kalloc.h>
#include <task/kthread.h>
#include <task/sched.h>

extern char *s_bss;
extern char *e_bss;

static inline void clear_bss() { memset(s_bss, 0, e_bss - s_bss); }
volatile bool sched_enabled = false;

int test_thread(void *arg)
{
	int id = *(int *)arg;
	for (int i = 0; i < 10; i++) {
		struct proc *p = thiscpu()->proc;
		infof("%s(%d) %d: %d", p->comm, p->pid, id, i);
		for (volatile u64 i = 0; i < 1000000000; i++)
			;
	}
	return id;
}

void init_kthreads(void)
{
	static int arg1 = 1, arg2 = 2;

	kthread_create(test_thread, &arg1, "thread1");
	kthread_create(test_thread, &arg2, "thread2");
	kthread_create(test_thread, &arg1, "thread3");
	kthread_create(test_thread, &arg2, "thread4");
}

void main(u64 hartid, void *_)
{
	if (hartid == 0) {
		clear_bss();
		init_cpu(hartid);
		infof("Hello world on cpu: %d", r_tp());
		print_pm_layout();
		pm_init();
		trap_init();
		timer_init();
		kvminit();
		init_runq();
		init_kthreads();
		sched_enabled = true;
		context_switch_to(&thiscpu()->idle.ctx);
		sched_yield();
	}
	while (1) {
		wfi();
	}
}