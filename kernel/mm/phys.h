#ifndef __KERNEL_MM_PHYS_H__
#define __KERNEL_MM_PHYS_H__


#include <uniks/defs.h>

void phymem_init();

// === buddy system ===
void buddy_system_init(uintptr_t start, uintptr_t end);
void *pages_alloc(size_t npages, int32_t wait_until_free);
void pages_free(void *pa);


// === kmalloc implemented by slub allocator ===
void kmem_cache_init();
void *kmalloc(size_t size);
void kfree(void *ptr);


#endif /* !__KERNEL_MM_PHYS_H__ */