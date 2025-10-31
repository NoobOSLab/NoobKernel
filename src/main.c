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

int test_thread(void *arg)
{
	int id = *(int *)arg;
	for (int i = 0; i < 10 + id; i++) {
		struct proc *p = thiscpu()->proc;
		infof("%s %d: %d", p->comm, id, i);
		// 模拟工作
		for (volatile int j = 0; j < 10000000; j++)
			;
		sched_yield(); // 主动让出
	}
	return id;
}

void init_kthreads(void)
{
	int arg1 = 1, arg2 = 2;

	kthread_create(test_thread, &arg1, "thread1");
	kthread_create(test_thread, &arg2, "thread2");
}

void main(u64 hartid, void *_)
{
	if (hartid == 0) {
		clear_bss();
		init_cpu(hartid);
		infof("Hello world on cpu: %d", thiscpu()->id);
		print_pm_layout();
		pm_init();
		trap_init();
		timer_init();
		kvminit();
		init_runq();
		init_kthreads();
		sched_yield();
	}
	while (1) {
		wfi();
	}
}