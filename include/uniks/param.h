#ifndef __PARAM_H__
#define __PARAM_H__


#define UNIKS_MAGIC (0x19890604)


// platform configurable parameters
#define PLATFORM_QEMU	  (1)
#define PLATFORM_DEVBOARD (1)
// CPU information
#define TIMESPERSEC	  (1000)
#define CPUFREQ		  (10000000)   // the frequency of qemu is 10MHz

// process relative configurable parameters
#define INITPROGRAM "initcode"
#define NPROC	    (64)   // max number of processes
#define MAXARG	    (64)   // max exec arguments and environment(they are same)
#define MAXARGLEN   (64)
#define MAXSTACK    (32)   // max number of processes' stack size(pages)

// file system configurable parameters
#define KiB		 (1024)
#define MiB		 (1024 * KiB)
#define GiB		 (1024 * MiB)
#define NBBUF		 (8192)	  // size of disk block buffer cache
#define NFD		 (64)	  // number of fds of each process
#define NINODE		 (512)	  // max number of active inodes
#define NFILE		 (256)	  // max number of opening files in system
#define PATH_MAX	 (1024)
#define ROOTPATH	 "/"
// device module configurable parameters
#define NDEVICE		 (64)	// max major device number, according to platform's PLIC
#define DEV_NAME_LEN	 (8)	 // max length of dev name
#define HASH_TABLE_PRIME (307)	 // this number comes from Linux-0.11

// UART configurable parameters
#define UART_TX_BUF_SIZE (1024)
// tty device configurable parameters
#define TTY_BUF_SIZE	 (1024)	  // ttys' read and write buffer size are equaled
#define LINE_MAXN	 (1024)


#endif /* !__PARAM_H__ */