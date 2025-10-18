#include <misc/log.h>
#include <misc/string.h>
#include <mm/pm.h>
#include <misc/complier.h>
#include <async/trap.h>
#include <hal/timer.h>
#include <mm/vm.h>

extern char *s_bss;
extern char *e_bss;

static inline void clear_bss() { memset(s_bss, 0, e_bss - s_bss); }

void main(u64 hartid, void *_)
{
	if (hartid == 0) {
		clear_bss();
		set_cpu(hartid);
		infof("Hello world on cpu: %d", thiscpu()->id);
		print_pm_layout();
		pm_init();
		trap_init();
		timer_init();
		kvminit();
		tick_t t = 0;
		while (1) {
			if (tick() - t >= TIMER_IRQ_HZ) {
				t = tick();
				infof("time: %us", t / TIMER_IRQ_HZ);
			} else {
				wfi();
			}
		}
	}
	while (1) {
		wfi();
	}
}