#include <task/sched.h>
#include <misc/list.h>
#include <misc/log.h>

struct {
	struct list_head queue;
	spinlock_t lock;
} __attribute__((aligned(CACHE_LINE_SIZE))) runq[CPU_NUM];

void init_runq()
{
	for (int i = 0; i < CPU_NUM; i++)
		INIT_LIST_HEAD(&runq[i].queue);
}

bool is_runq_empty(int hartid) { return list_empty(&runq[hartid].queue); }

void enqueue_proc(int hartid, struct proc *p)
{
	spinlock_acquire(&runq[hartid].lock);
	list_add_tail(&p->runq, &runq[hartid].queue);
	spinlock_release(&runq[hartid].lock);
}

struct proc *dequeue_proc(int hartid)
{
	if (is_runq_empty(hartid)) {
		return NULL;
	}
	spinlock_acquire(&runq[hartid].lock);
	struct list_head *lh = runq[hartid].queue.next;
	list_del(lh);
	spinlock_release(&runq[hartid].lock);
	return container_of(lh, struct proc, runq);
}

void sched_yield()
{
	struct proc *p = thiscpu()->proc;
	if (p) {
		switch(p->state) {
		case PROC_RUNNING:
			p->state = PROC_RUNNABLE;
			enqueue_proc(thiscpu()->id, p);
			break;
		case PROC_ZOMBIE:
			free_proc(p);
			break;
		default:
			// 非 RUNNING 状态无需入队
			break;
		}
	}
	struct proc *next = dequeue_proc(thiscpu()->id);
	if (next) {
		next->state = PROC_RUNNING;
		thiscpu()->proc = next;
		if (p)
			context_switch(&p->ctx, &next->ctx);
		else
			context_switch(&thiscpu()->ctx, &next->ctx);
	} else {
		infof("cpu %d idle", thiscpu()->id);
		thiscpu()->proc = NULL;
		context_switch(&p->ctx, &thiscpu()->ctx);
	}
}
