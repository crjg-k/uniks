#ifndef __PARAM_H__
#define __PARAM_H__


// platform  configurable parameters
#define PLATFORM_QEMU	  (1)
#define PLATFORM_DEVBOARD (1)
// CPU information
#define NCPU		  (1)	       // max number of harts
#define TIMESPERSEC	  (1000)
#define CPUFREQ		  (10000000)   // the frequency of qemu is 10MHz

// process relative configurable parameters
#define PROC_NAME_LEN (16)   // max length of process name
#define NPROC	      (64)   // max number of processes
#define MAXARG	      (32)   // max exec arguments and environment(they are same)

// file system configurable parameters
#define UNIKS_MAGIC	  (0x19890604)
#define NBUF		  (32)	  // size of disk block buffer cache
#define NFD		  (32)	  // number of fds of each process
#define NINODE		  (64)	  // max number of active inodes
#define NFILE		  (128)	  // max number of opening files in system
// device module configurable parameters
#define NDEVICE		  (64)	 // max major device number, according to platform's PLIC
#define DEV_NAME_LEN	  (8)	  // max length of dev name
#define BLKBUF_HASH_PRIME (307)	  // this num comes from Linux-0.11

// UART configurable parameters
#define UART_TX_BUF_SIZE (32)
// tty device configurable parameters
#define TTY_BUF_SIZE	 (32)	// ttys' read and write buffer size are equaled


#endif /* !__PARAM_H__ */