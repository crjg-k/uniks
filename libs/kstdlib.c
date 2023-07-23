#include <uniks/kstdlib.h>


__always_inline int64_t div_round_up(int64_t first, int64_t second)
{
	return (first + second - 1) / second;
}