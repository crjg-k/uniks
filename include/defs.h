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

#ifndef NULL
	#define NULL ((void *)0)
#endif


#define __always_inline inline __attribute__((always_inline))
#define __noinline	__attribute__((noinline))
#define __noreturn	__attribute__((noreturn))


extern char stext[], etext[];
extern char sdata[], edata[];
extern char sbss[], ebss[];
extern char end[];


#include <types.h>
#include <kassert.h>
#include <log.h>

#endif /* !__DEFS_H__ */
