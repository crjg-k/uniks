#ifndef __KERNEL_MM_MMU_H__
#define __KERNEL_MM_MMU_H__


#define PGSIZE	4096   // a page size
#define PGSHIFT 12     // log2(PGSIZE)

#define OFFSETPAGE(addr) (addr & 0xfff)
#define PGROUNDUP(sz)	 (((sz) + PGSIZE - 1) & ~(PGSIZE - 1))
#define PGROUNDDOWN(a)	 (((a)) & ~(PGSIZE - 1))


#endif /* !__KERNEL_MM_MMU_H__ */