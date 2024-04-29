#include <uniks/kstdlib.h>


__always_inline int64_t div_round_up(int64_t first, int64_t second)
{
	return (first + second - 1) / second;
}

uintptr_t alignaddr_up(uintptr_t addr, size_t alignment)
{
	uintptr_t mask = alignment - 1;
	if ((alignment & mask) == 0) {	 // Check if alignment is a power of 2
		return get_var_bit(addr + mask, ~mask);
	} else
		return addr;
}

uintptr_t alignaddr_down(uintptr_t addr, size_t alignment)
{
	uintptr_t mask = alignment - 1;
	if ((alignment & mask) == 0) {	 // Check if alignment is a power of 2
		return get_var_bit(addr, ~mask);
	} else
		return addr;
}
