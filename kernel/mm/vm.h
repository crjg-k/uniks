#ifndef __KERNEL_MM_VM_H__
#define __KERNEL_MM_VM_H__


#include "phys.h"
#include <platform/riscv.h>
#include <process/proc.h>
#include <uniks/defs.h>


// struct vm_area_struct {
// 	uintptr_t vm_start;   // our start address within vm_mm
// 	uintptr_t vm_end;

// 	struct list_node_t vm_area_list;
// 	// rb_node_t vm_rb;                   // red-black tree node

// 	uint64_t vm_flags;
// 	uint64_t vm_pgoff;
// 	struct file_t *vm_file;	  // if this segment map a file
// };
// struct mm_struct {
// 	struct list_node_t vm_area_list_head;	// list of VMA
// 	// rb_root_t mm_rb;	       // red-black tree root points to VMA
// 	struct spinlock_t mmap_lk;	     // mmap's lock
// 	struct vm_area_struct *mmap_cache;   // last find_vma result as a cache

// 	pagetable_t pagetable;		     // user page table
// 	uintptr_t kstack;    // always point to own kernel stack bottom

// 	uint32_t mm_count;   // how many reference to "struct mm_struct"
// 	int32_t map_count;   // number of VMA

// 	uintptr_t start_code, end_code, start_data, end_data;
// 	uintptr_t start_brk, brk,
// 		start_stack;   // start_stack means the high addr of stack
// 	uintptr_t arg_start, arg_end, env_start, env_end;
// };

extern char trampoline[];
extern int32_t mem_map[];


uintptr_t vaddr2paddr(pagetable_t pagetable, uintptr_t va);

/* === kernel vitual addr space related === */

void kvmenable();
int32_t mappages(pagetable_t pagetable, uintptr_t va, size_t size, uintptr_t pa,
		 int32_t perm, int8_t recordref);
void kvminit();

/* === user vitual addr space related === */

pagetable_t uvmcreate();
void uvmunmap(pagetable_t pagetable, uintptr_t va, uintptr_t npages,
	      int32_t do_free);
void uvmfree(pagetable_t pagetable, uint64_t sz);
int64_t uvmcopy(pagetable_t old, pagetable_t new, uint64_t sz);

/* === lazy loading mechanism === */

void do_inst_page_fault(uintptr_t fault_vaddr);
void do_ld_page_fault(uintptr_t fault_vaddr);
void do_sd_page_fault(uintptr_t fault_vaddr);
void verify_area(void *addr, int64_t size);

/* === transmit data between kernel and user process space === */

int64_t copyin(pagetable_t pagetable, void *dst, void *srcva, uint64_t len);
int32_t copyin_string(pagetable_t pagetable, char *dst, uintptr_t srcva,
		      uint64_t max);
int32_t copyout(pagetable_t pagetable, void *dstva, void *src, uint64_t len);


#endif /* ! __KERNEL_MM_VM_H__*/