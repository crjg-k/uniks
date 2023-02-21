#ifndef __KERNEL_MM_MEMLAY_H__
#define __KERNEL_MM_MEMLAY_H__


#define KSTACKPAGE 4			   // number of pages in kernel stack
#define KSTACKSIZE (KSTACKPAGE * PGSIZE)   // sizeof kernel stack
#define KERNELBASE 0x80000000	// in now stage, this is actually OpenSBI
#define PHYSTOP	   (KERNELBASE + 128 * 1024 * 1024)   // 128MiB RAM main memory


#endif /* !__KERNEL_MM_MEMLAY_H__ */
