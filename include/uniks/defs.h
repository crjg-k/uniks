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


#include <uniks/types.h>

#define __always_inline inline __attribute__((always_inline))
#define __noinline	__attribute__((noinline))
#define __noreturn	__attribute__((noreturn))


extern char stext[], etext[];
extern char sdata[], edata[];
extern char sbss[], ebss[];
extern char end[];
extern char bootstackbottom0[], bootstacktop0[];
extern char bootstackbottom1[], bootstacktop1[];
extern char KERNEL_BASE_ADDR[];


#endif /* !__DEFS_H__ */
