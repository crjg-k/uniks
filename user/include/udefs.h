#ifndef __USER_INCLUDE_UDEFS_H__
#define __USER_INCLUDE_UDEFS_H__


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


#define __always_inline inline __attribute__((always_inline))
#define __noinline	__attribute__((noinline))
#define __noreturn	__attribute__((noreturn))
#define __packed	__attribute__((packed))
#define __naked		__attribute__((naked))
#define __aligned(N)	__attribute__((aligned(N)))


#endif /* !__USER_INCLUDE_UDEFS_H__ */