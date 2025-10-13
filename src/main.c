#include <misc/log.h>
#include <misc/string.h>
#include <mm/pm.h>
#include <misc/complier.h>
#include <async/trap.h>
#include <hal/timer.h>

extern char *s_bss;
extern char *e_bss;

static inline void clear_bss() {
	memset(s_bss, 0, e_bss - s_bss);
}

static void timer_callback() {
	set_timer(time_now() + ms_to_ticks(1000), timer_callback);
	infof("time_now: %zu", time_now());
}

void main(u64 hartid, void *_) {
	if (hartid == 0) {
		clear_bss();
		infof("Hello world");
		print_pm_layout();
		pm_init();
		trap_init();
		tick_t timer = time_now() + ms_to_ticks(1000);
		set_timer(timer, timer_callback);
	}
	while (1) {
		wfi();
	}
}