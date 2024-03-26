#ifndef __KERNEL_MM_MMU_H__
#define __KERNEL_MM_MMU_H__


#define PGSHIFT (12)		   // log2(PGSIZE)
#define PGSIZE	((1) << PGSHIFT)   // a page size

#define OFFSETPAGE(addr)  get_var_bit(addr, PGSIZE - 1)
#define PGROUNDUP(addr)	  get_var_bit((addr + PGSIZE - 1), ~(PGSIZE - 1))
#define PGROUNDDOWN(addr) get_var_bit((addr), ~(PGSIZE - 1))


#endif /* !__KERNEL_MM_MMU_H__ */