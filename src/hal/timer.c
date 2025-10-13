#include <hal/timer.h>
#include <hal/riscv.h>
#include <hal/sbi.h>

void (*timer_callback)(void) = NULL;

tick_t time_now(){
	return r_time();
}

void set_timer(tick_t time, void (*callback)(void)){
	if(!callback)
		return;
	timer_callback = callback;
	sbi_set_timer(time);
}

void handle_timer(){
	timer_callback();
}
