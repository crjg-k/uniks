#include <uniks/kstdlib.h>


__always_inline int64_t div_round_up(int64_t num, int64_t size)
{
	return (num + size - 1) / size;
}