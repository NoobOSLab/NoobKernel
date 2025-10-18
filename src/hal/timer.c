#include <hal/riscv.h>
#include <hal/sbi.h>
#include <hal/timer.h>

static volatile tick_t kernel_tick;

inline tick_t tick()
{
	return kernel_tick;
}

void timer_init() {
	sbi_set_timer(r_time() + us_to_cputime(1000000ULL / TIMER_IRQ_HZ));
	kernel_tick = cputime_to_us(r_time()) * TIMER_IRQ_HZ / 1000000ULL;
}

void handle_timer() {
	sbi_set_timer(r_time() + us_to_cputime(1000000ULL / TIMER_IRQ_HZ));
	kernel_tick++;
}
