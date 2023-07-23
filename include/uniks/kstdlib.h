#ifndef __KSTDLIB_H__
#define __KSTDLIB_H__


#include <uniks/defs.h>


#define MAX(a, b) (a < b ? b : a)
#define MIN(a, b) (a < b ? a : b)


int64_t div_round_up(int64_t fisrt, int64_t second);


#endif /* !__KSTDLIB_H__ */