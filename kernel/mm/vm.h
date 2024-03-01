#ifndef __KERNEL_MM_VM_H__
#define __KERNEL_MM_VM_H__


#include "phys.h"
#include <platform/riscv.h>
#include <sync/spinlock.h>
#include <uniks/defs.h>
#include <uniks/list.h>


struct vm_area_struct {
	/* VMA covers [vm_start, vm_end) addresses within mm */
	uintptr_t vm_start;
	uintptr_t vm_end;

	struct mm_struct *vm_mm; /* The address space we belong to */

	struct list_node_t vm_area_list;

	/**
	 * @brief bit function: [D|A|G|U|X|W|R|V]
	 */
	uint32_t vm_flags;

	// Offset (within vm_file) in PAGE_SIZE units
	uint64_t vm_pgoff;
	struct file_t *vm_file;	  // if this segment map a file
};

struct mm_struct {
	struct spinlock_t mmap_lk;   // mmap's lock

	struct list_node_t vm_area_list_head;	// list of VMA

	pagetable_t pagetable;	 // user page table
	uintptr_t kstack;	 // always point to own kernel stack bottom

	/**
	 * @brief: The number of references to &struct mm_struct.
	 *
	 * When this drops to 0, the &struct mm_struct is freed.
	 */
	uint32_t mm_count;
	int32_t map_count;   // number of VMA

	uintptr_t start_stack;	 // start_stack means the high addr of stack
	uintptr_t mmap_base;	 // base of mmap area
	uintptr_t start_brk, brk;
	uintptr_t start_code, end_code, start_data, end_data;
};

extern char trampoline[];


uintptr_t vaddr2paddr(pagetable_t pagetable, uintptr_t va);

/* === kernel vitual addr space related === */

void kvmenablehart();
int32_t mappages(pagetable_t pagetable, uintptr_t va, size_t size, uintptr_t pa,
		 int32_t perm);
void kvminit();

/* === user vitual addr space related === */

pagetable_t uvmcreate();
void uvmmap(pagetable_t pagetable, uintptr_t va_start, uintptr_t va_end,
	    int32_t perm);
void uvmunmap(pagetable_t pagetable, uintptr_t va_start, uintptr_t va_end,
	      int32_t perm);
int32_t init_mm(struct mm_struct *mm);
void free_mm(struct mm_struct *mm);
int32_t add_vm_area(struct mm_struct *mm, uintptr_t va_start, uintptr_t va_end,
		    uint64_t flags, uint64_t pgoff, struct file_t *file);
struct vm_area_struct *search_vmareas(struct mm_struct *mm,
				      uintptr_t target_vaddr, size_t size,
				      uint32_t *flag_res);

void uvmfree(struct mm_struct *mm);
int64_t uvmcopy(struct mm_struct *mm_new, struct mm_struct *mm_old);

/* === lazy loading mechanism === */

void do_inst_page_fault(struct mm_struct *mm, uintptr_t fault_vaddr);
void do_ld_page_fault(struct mm_struct *mm, uintptr_t fault_vaddr);
void do_sd_page_fault(struct mm_struct *mm, uintptr_t fault_vaddr);
int32_t verify_area(struct mm_struct *mm, uintptr_t vaddr, size_t size,
		    int32_t targetflags);

/* === transmit data between kernel and user process space === */

int32_t copyin(pagetable_t pagetable, void *dst, void *srcva, uint64_t len);
int32_t copyin_string(pagetable_t pagetable, char *dst, uintptr_t srcva,
		      uint64_t max);
int32_t copyout(pagetable_t pagetable, void *dstva, void *src, uint64_t len);
int32_t either_copyin(int32_t user_src, void *dst, void *src, uint64_t len);
int32_t either_copyout(int32_t user_dst, void *dst, void *src, uint64_t len);


#endif /* ! __KERNEL_MM_VM_H__*/