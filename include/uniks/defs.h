#ifndef __DEFS_H__
#define __DEFS_H__


#include <stdarg.h>
#include <stdatomic.h>


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

#ifndef EOT
	#define EOT ((char)4)
#endif

#define UNIKS_MSG "\e[1m[\tuniks]\e[0m==> "


enum {
	RED = 31,
	GREEN = 32,
	BLUE = 34,
	GRAY = 90,
	YELLOW = 93,
};


#include <uniks/types.h>

#define __always_inline inline __attribute__((always_inline))
#define __noinline	__attribute__((noinline))
#define __noreturn	__attribute__((noreturn))
#define __packed	__attribute__((packed))
#define __naked		__attribute__((naked))
#define __aligned(N)	__attribute__((aligned(N)))


#define set_var_bit(var, bit)	({ (var) |= (bit); })
#define clear_var_bit(var, bit) ({ (var) &= ~(bit); })
// The value of this expression: 0 => zero, 1 => non-zero
#define get_var_bit(var, bit)	({ (var) & (bit); })
#define get_high_32bit(var)	({ (var) >> 32; })


#define SECTORSIZE 512


#endif /* !__DEFS_H__ */