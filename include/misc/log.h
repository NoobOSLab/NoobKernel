#ifndef __MISC_LOG_H__
#define __MISC_LOG_H__

#include <misc/printf.h>
#include <hal/sbi.h>

#define DETAILED_LOG (0)

static void dummy(int _, ...) {
}

#if DETAILED_LOG
#define DETAILED_FORMAT "(%s:%d) "
#define DETAILED_PARAMS , __FILE__, __LINE__
#else
#define DETAILED_FORMAT "\t"
#define DETAILED_PARAMS
#endif // DETAILED_LOG

#if defined(LOG_LEVEL_ERROR)

#define USE_LOG_ERROR

#endif // LOG_LEVEL_ERROR

#if defined(LOG_LEVEL_WARN)

#define USE_LOG_ERROR
#define USE_LOG_WARN

#endif // LOG_LEVEL_ERROR

#if defined(LOG_LEVEL_INFO)

#define USE_LOG_ERROR
#define USE_LOG_WARN
#define USE_LOG_INFO

#endif // LOG_LEVEL_INFO

#if defined(LOG_LEVEL_DEBUG)

#define USE_LOG_ERROR
#define USE_LOG_WARN
#define USE_LOG_INFO
#define USE_LOG_DEBUG

#endif // LOG_LEVEL_DEBUG

#if defined(LOG_LEVEL_TRACE)

#define USE_LOG_ERROR
#define USE_LOG_WARN
#define USE_LOG_INFO
#define USE_LOG_DEBUG
#define USE_LOG_TRACE

#endif // LOG_LEVEL_TRACE

enum LOG_COLOR {
	RED = 31,
	GREEN = 32,
	BLUE = 36,
	GRAY = 90,
	YELLOW = 93,
};

#if defined(USE_LOG_ERROR)
#define errorf(fmt, ...)                                                       \
	do {                                                                   \
		printf("\x1b[%dm[%s]" DETAILED_FORMAT fmt "\x1b[0m\n", RED,    \
		       "ERROR" DETAILED_PARAMS, ##__VA_ARGS__);                \
	} while (0)
#else
#define errorf(fmt, ...) dummy(0, ##__VA_ARGS__)
#endif // USE_LOG_ERROR

#if defined(USE_LOG_WARN)
#define warnf(fmt, ...)                                                        \
	do {                                                                   \
		printf("\x1b[%dm[%s]" DETAILED_FORMAT fmt "\x1b[0m\n", YELLOW, \
		       "WARN" DETAILED_PARAMS, ##__VA_ARGS__);                 \
	} while (0)
#else
#define warnf(fmt, ...) dummy(0, ##__VA_ARGS__)
#endif // USE_LOG_WARN

#if defined(USE_LOG_INFO)
#define infof(fmt, ...)                                                        \
	do {                                                                   \
		printf("\x1b[%dm[%s]" DETAILED_FORMAT fmt "\x1b[0m\n", BLUE,   \
		       "INFO" DETAILED_PARAMS, ##__VA_ARGS__);                 \
	} while (0)
#else
#define infof(fmt, ...) dummy(0, ##__VA_ARGS__)
#endif // USE_LOG_INFO

#if defined(USE_LOG_DEBUG)
#define debugf(fmt, ...)                                                       \
	do {                                                                   \
		printf("\x1b[%dm[%s]" DETAILED_FORMAT fmt "\x1b[0m\n", GREEN,  \
		       "DEBUG" DETAILED_PARAMS, ##__VA_ARGS__);                \
	} while (0)
#else
#define debugf(fmt, ...) dummy(0, ##__VA_ARGS__)
#endif // USE_LOG_DEBUG

#if defined(USE_LOG_TRACE)
#define tracef(fmt, ...)                                                       \
	do {                                                                   \
		printf("\x1b[%dm[%s]" DETAILED_FORMAT fmt "\x1b[0m\n", GRAY,   \
		       "TRACE" DETAILED_PARAMS, ##__VA_ARGS__);                \
	} while (0)
#else
#define tracef(fmt, ...) dummy(0, ##__VA_ARGS__)
#endif // USE_LOG_TRACE

#define panic(fmt, ...)                                                        \
	do {                                                                   \
		printf("\x1b[%dm[%s]" DETAILED_FORMAT fmt "\x1b[0m\n", RED,    \
		       "PANIC" DETAILED_PARAMS, ##__VA_ARGS__);                \
		sbi_shutdown();                                                    \
		__builtin_unreachable();                                       \
	} while (0)

#endif //! __MISC_LOG_H__
