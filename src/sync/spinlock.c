#include <sync/spinlock.h>
#include <trap/trap.h>

void spinlock_acquire(spinlock_t *lock)
{
#if CPU_NUM == 1
	(void)lock;
	intr_off();
#else
#endif
}

void spinlock_release(spinlock_t *lock) {
#if CPU_NUM == 1
	(void)lock;
	intr_on();
#else
#endif
}
