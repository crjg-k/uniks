#ifndef __KERNEL_MM_VM_H__
#define __KERNEL_MM_VM_H__


#include "phys.h"
#include <platform/riscv.h>
#include <sync/spinlock.h>
#include <uniks/defs.h>
#include <uniks/list.h>


struct vm_area_struct {
	/**
	 * @brief VMA covers [vm_start, vm_end) addresses, and these must be
	 * aligned to a page.
	 */
	uintptr_t vm_start, vm_end;

	struct mm_struct *vm_mm; /* The address space we belong to */

	struct list_node_t vm_area_list;

	// bit function: [D|A|G|U|X|W|R|V]
	uint32_t vm_flags;
	uint64_t _filesz;   // to cope with .bss and .data

	uint64_t vm_pgoff;   // Offset (within vm_inode) in PAGE_SIZE units
	struct m_inode_t *vm_inode;   // if this segment map a file
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

	uint32_t stack_maxsize;

	uintptr_t start_ustack;	  // means the high addr of user's stack
	uintptr_t mmap_base;	  // base of mmap area
	uintptr_t start_brk, brk;
	// uintptr_t start_code, end_code, start_data, end_data;
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
void free_pgtable(pagetable_t pagetable, int32_t layer);
int64_t uvm_space_copy(struct mm_struct *new_mm, struct mm_struct *old_mm);
struct mm_struct *new_mm_struct();
void free_mm_struct(struct mm_struct *mm);
struct vm_area_struct *new_vmarea_struct(uintptr_t _va_start, uintptr_t _va_end,
					 uint64_t flags, uint64_t pgoff,
					 struct m_inode_t *inode,
					 uint64_t _filesz);
int32_t add_vm_area(struct mm_struct *mm, uintptr_t va_start, uintptr_t va_end,
		    uint64_t flags, uint64_t pgoff, struct m_inode_t *vm_inode,
		    uint64_t _filesz);
struct vm_area_struct *search_vmareas(struct mm_struct *mm,
				      uintptr_t target_vaddr, size_t size,
				      uint32_t *flag_res);


/* === lazy loading mechanism === */

void do_inst_page_fault(uintptr_t fault_vaddr);
void do_ld_page_fault(uintptr_t fault_vaddr);
void do_sd_page_fault(uintptr_t fault_vaddr);
int32_t verify_area(struct mm_struct *mm, uintptr_t vaddr, size_t size,
		    int32_t targetflags);

/* === transmit data between kernel and user process space === */

int32_t copyin(pagetable_t pagetable, void *dst, void *srcva, uint64_t len);
int32_t copyin_string(pagetable_t pagetable, char *dst, uintptr_t srcva,
		      uint64_t max);
int32_t copyout(pagetable_t pagetable, void *dstva, void *src, uint64_t len);
int32_t either_copyin(int32_t user_src, void *dst, void *src, uint64_t len);
int32_t either_copyout(int32_t user_dst, void *dst, void *src, uint64_t len);

int64_t sys_brk();


#endif /* ! __KERNEL_MM_VM_H__*/