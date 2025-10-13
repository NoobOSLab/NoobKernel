#ifndef __HAL_TIMER_H__
#define __HAL_TIMER_H__

#include <config.h>

tick_t time_now();
void set_timer(tick_t time, void (*callback)(void));

#endif