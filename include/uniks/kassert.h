#ifndef __KASSERT_H__
#define __KASSERT_H__

#include <platform/riscv.h>
#include <uniks/defs.h>
#include <uniks/kstdio.h>


#define panic(fmt, ...) \
	({ \
		extern void sbi_shutdown(); \
		pr.locking = 0; \
		int64_t hartid = cpuid(); \
		kprintf("\n\x1b[%dm[%s %x] %s:%d: " fmt "\x1b[0m\n", RED, \
			"PANIC", hartid, __FILE__, __LINE__, ##__VA_ARGS__); \
		sbi_shutdown(); \
		1; \
	})

#define BUG() ({ panic("BUG! /%s()", __func__); })


#define assert(_Expression) \
	(void)((!!(_Expression)) or \
	       (panic("Assertion failed: %s", #_Expression)))


#endif /* !__KASSERT_H__ */