#ifndef __KERNEL_PLATFORM_SBI_H__
#define __KERNEL_PLATFORM_SBI_H__


#include <uniks/defs.h>


// SBI EID number
#define SBI_SET_TIMER	    0
#define SBI_CONSOLE_PUTCHAR 1
#define SBI_CONSOLE_GETCHAR 2
#define SBI_CLEAR_IPI	    3
#define SBI_SEND_IPI	    4
#define SBI_SHUTDOWN	    8
#define SBI_HSM		    0x48534D

// SBI FID number belong SBI_HSM
#define HART_START 0


struct sbiretv_t {
	int64_t error;
	int64_t value;
};


int64_t sbi_set_timer(uint64_t stime_value);
int64_t sbi_console_putchar(char ch);
int64_t sbi_console_getchar();
void sbi_shutdown();
struct sbiretv_t sbi_hart_start(uint64_t hartid, uint64_t start_addr,
				uint64_t opaque);


#endif /* !__KERNEL_PLATFORM_SBI_H__ */