#ifndef __KERNEL_MM_PHYS_H__
#define __KERNEL_MM_PHYS_H__


#include <sync/spinlock.h>
#include <uniks/defs.h>


// temporarily assume that the size of kernel will not exceed KERNEL_END_LIMIT
#define KERNEL_END_LIMIT      0x80400000
#define PHYMEM_AVAILABLE      (PHYSTOP - KERNEL_END_LIMIT)
#define ADDR2ARRAYINDEX(addr) (((char *)(addr) - (char *)mem_start) >> PGSHIFT)


void phymem_init();

// === buddy system ===
struct phys_page_record_t {
	int16_t order;
	uint16_t count;
	struct spinlock_t lk;
};
void buddy_system_init(uintptr_t start, uintptr_t end);
void *pages_alloc(size_t npages);
void *pages_zalloc(size_t npages);
void *pages_dup(void *ptr);
int32_t pages_undup(void *ptr);
void release_pglock(void *ptr);
void pages_free(void *pa);


// === kmalloc implemented by slub allocator ===
void kmem_cache_init();
void *kmalloc(size_t size);
void *kzalloc(size_t size);
void kfree(void *ptr);


#endif /* !__KERNEL_MM_PHYS_H__ */