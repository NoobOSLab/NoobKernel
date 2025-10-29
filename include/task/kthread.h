#ifndef __TASK_KTHREAD_H__
#define __TASK_KTHREAD_H__

struct proc *kthread_create(int (*fn)(void *), void *arg, const char *name);
void kthread_exit(void) __attribute__((noreturn));

#endif