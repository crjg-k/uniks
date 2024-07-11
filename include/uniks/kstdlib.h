#ifndef __KSTDLIB_H__
#define __KSTDLIB_H__


#include <uniks/defs.h>


#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define SWAP(a, b, type) \
	({ \
		type temp = a; \
		a = b; \
		b = temp; \
	})


int64_t div_round_up(int64_t fisrt, int64_t second);
uintptr_t alignaddr_up(uintptr_t addr, size_t alignment);
uintptr_t alignaddr_down(uintptr_t addr, size_t alignment);


#endif /* !__KSTDLIB_H__ */