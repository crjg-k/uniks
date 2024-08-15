#ifndef __KSTDLIB_H__
#define __KSTDLIB_H__


#include <uniks/defs.h>


#define MAX(a, b) \
	({ \
		typeof(a) tmp_a = (a), tmp_b = (b); \
		(tmp_a) < (tmp_b) ? (tmp_b) : (tmp_a); \
	})
#define MIN(a, b) \
	({ \
		typeof(a) tmp_a = (a), tmp_b = (b); \
		(tmp_a) < (tmp_b) ? (tmp_a) : (tmp_b); \
	})
#define SWAP(a, b) \
	({ \
		typeof(a) temp = a; \
		a = b; \
		b = temp; \
	})


int64_t div_round_up(int64_t fisrt, int64_t second);
uintptr_t alignaddr_up(uintptr_t addr, size_t alignment);
uintptr_t alignaddr_down(uintptr_t addr, size_t alignment);


#endif /* !__KSTDLIB_H__ */