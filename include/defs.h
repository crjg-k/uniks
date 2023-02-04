#ifndef __DEFS_H__
#define __DEFS_H__

#ifndef NULL
	#define NULL ((void *)0)
#endif

#ifndef and
	#define and &&
#endif
#ifndef or
	#define or ||
#endif

#define __always_inline inline __attribute__((always_inline))
#define __noinline	__attribute__((noinline))
#define __noreturn	__attribute__((noreturn))

#include <types.h>

#endif /* !__DEFS_H__ */
