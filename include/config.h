#ifndef __CONFIG_H__
#define __CONFIG_H__

#if defined (QEMU)
#include <platform/qemu_virt.h>
#endif

static inline uint64_t ns_to_ticks(uint64_t ns) {
    return (ns * TIMEBASE_FREQ) / 1000000000ULL;
}

static inline uint64_t us_to_ticks(uint64_t us) {
    return (us * TIMEBASE_FREQ) / 1000000ULL;
}

static inline uint64_t ms_to_ticks(uint64_t ms) {
    return (ms * TIMEBASE_FREQ) / 1000ULL;
}

static inline uint64_t sec_to_ticks(uint64_t sec) {
    return sec * TIMEBASE_FREQ;
}

static inline uint64_t ticks_to_ns(uint64_t ticks) {
    return (ticks * 1000000000ULL) / TIMEBASE_FREQ;
}

static inline uint64_t ticks_to_us(uint64_t ticks) {
    return (ticks * 1000000ULL) / TIMEBASE_FREQ;
}

static inline uint64_t ticks_to_ms(uint64_t ticks) {
    return (ticks * 1000ULL) / TIMEBASE_FREQ;
}



#endif