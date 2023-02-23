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


#include <types.h>

#define __always_inline inline __attribute__((always_inline))
#define __noinline	__attribute__((noinline))
#define __noreturn	__attribute__((noreturn))


extern char stext[], etext[];
extern char sdata[], edata[];
extern char sbss[], ebss[];
extern char end[];
extern char bootstack0[], bootstacktop0[];
extern char bootstack1[], bootstacktop1[];
extern char KERNEL_BASE_ADDR[];


#include <log.h>


#endif /* !__DEFS_H__ */
