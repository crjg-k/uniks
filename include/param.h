#ifndef __PARAM_H__
#define __PARAM_H__


#define PROC_NAME_LEN 63		  // max count of process name length
#define NPROC	      64		  // max number of processes
#define NCPU	      1			  // max number of harts
#define MAXOPBLOCKS   10		  // max # of blocks any FS op writes
#define NBUF	      (MAXOPBLOCKS * 3)	  // size of disk block cache
#define NINODE	      64		  // max number of active inodes
#define NFILE	      255   // max number of opening files in system
#define TIMESPERSEC   100
#define CPUFREQ	      10000000


#endif /* !__PARAM_H__ */