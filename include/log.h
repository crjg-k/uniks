#ifndef __LOG_H__
#define __LOG_H__

#include <defs.h>

extern void kprintf(const char *fmt, ...);
static uint64_t __always_inline r_mhartid()
{
	uint64_t x = 0;
	return x;
}
extern void sbi_shutdown();

#if defined(LOG_LEVEL_ERROR)

	#define USE_LOG_ERROR

#endif	 // LOG_LEVEL_ERROR

#if defined(LOG_LEVEL_WARN)

	#define USE_LOG_ERROR
	#define USE_LOG_WARN

#endif	 // LOG_LEVEL_ERROR

#if defined(LOG_LEVEL_INFO)

	#define USE_LOG_ERROR
	#define USE_LOG_WARN
	#define USE_LOG_INFO

#endif	 // LOG_LEVEL_INFO

#if defined(LOG_LEVEL_DEBUG)

	#define USE_LOG_ERROR
	#define USE_LOG_WARN
	#define USE_LOG_INFO
	#define USE_LOG_DEBUG

#endif	 // LOG_LEVEL_DEBUG

#if defined(LOG_LEVEL_TRACE)

	#define USE_LOG_ERROR
	#define USE_LOG_WARN
	#define USE_LOG_INFO
	#define USE_LOG_DEBUG
	#define USE_LOG_TRACE

#endif	 // LOG_LEVEL_TRACE

enum LOG_COLOR {
	RED = 31,
	GREEN = 32,
	BLUE = 34,
	GRAY = 90,
	YELLOW = 93,
};

#if defined(USE_LOG_ERROR)
	#define errorf(fmt, ...) \
		({ \
			int64_t tid = r_mhartid(); \
			kprintf("\x1b[%dm[%s %d] " fmt "\x1b[0m\n", RED, \
				"ERROR", tid, ##__VA_ARGS__); \
		})
#else
	#define errorf(fmt, ...)
#endif	 // USE_LOG_ERROR

#if defined(USE_LOG_WARN)
	#define warnf(fmt, ...) \
		({ \
			int64_t tid = r_mhartid(); \
			kprintf("\x1b[%dm[%s %d] " fmt "\x1b[0m\n", YELLOW, \
				"WARN", tid, ##__VA_ARGS__); \
		})
#else
	#define warnf(fmt, ...)
#endif	 // USE_LOG_WARN

#if defined(USE_LOG_INFO)
	#define infof(fmt, ...) \
		({ \
			int64_t tid = r_mhartid(); \
			kprintf("\x1b[%dm[%s %d] " fmt "\x1b[0m\n", BLUE, \
				"INFO", tid, ##__VA_ARGS__); \
		})
#else
	#define infof(fmt, ...)
#endif	 // USE_LOG_INFO

#if defined(USE_LOG_DEBUG)
	#define debugf(fmt, ...) \
		({ \
			int64_t tid = r_mhartid(); \
			kprintf("\x1b[%dm[%s %d] " fmt "\x1b[0m\n", GREEN, \
				"DEBUG", tid, ##__VA_ARGS__); \
		})
#else
	#define debugf(fmt, ...)
#endif	 // USE_LOG_DEBUG

#if defined(USE_LOG_TRACE)
	#define tracef(fmt, ...) \
		({ \
			int64_t tid = r_mhartid(); \
			kprintf("\x1b[%dm[%s %d] " fmt "\x1b[0m\n", GRAY, \
				"TRACE", tid, ##__VA_ARGS__); \
		})
#else
	#define tracef(fmt, ...)
#endif	 // USE_LOG_TRACE

#define panic(fmt, ...) \
	({ \
		int64_t tid = r_mhartid(); \
		kprintf("\x1b[%dm[%s %d] %s:%d: " fmt "\x1b[0m\n", RED, \
			"PANIC", tid, __FILE__, __LINE__, ##__VA_ARGS__); \
		sbi_shutdown(); \
	})

#endif	 //! __LOG_H__
