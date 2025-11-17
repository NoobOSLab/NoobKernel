#ifndef __SYNC_SPINLOCK_H__
#define __SYNC_SPINLOCK_H__

#include <config.h>


#if CPU_NUM == 1
typedef struct{
}spinlock_t;
#else
#endif

void spinlock_acquire(spinlock_t *lock);
void spinlock_release(spinlock_t *lock);

#endif