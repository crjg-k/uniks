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

#ifndef EOF
	#define EOF ((char)-1)
#endif

#define UNIKS_MSG "[\tuniks]==> "


#include <uniks/types.h>

#define __always_inline inline __attribute__((always_inline))
#define __noinline	__attribute__((noinline))
#define __noreturn	__attribute__((noreturn))
#define __packed	__attribute__((packed))


#define set_variable_bit(var, bit)   ({ (var) |= (bit); })
#define clear_variable_bit(var, bit) ({ (var) &= ~(bit); })
#define get_variable_bit(var, bit)   ({ (var) & (bit); })

#define get_high_32bit(var) ({ (var) >> 32; })


extern char stext[], etext[];
extern char sdata[], edata[];
extern char sbss[], ebss[];
extern char end[];
extern char bootstackbottom0[], bootstacktop0[];
extern char bootstackbottom1[], bootstacktop1[];
extern char KERNEL_BASE_ADDR[];


#endif /* !__DEFS_H__ */
