#include "memlay.h"
#include "mmu.h"
#include <defs.h>
#include <kassert.h>
#include <kstring.h>
#include <list.h>


LIST_HEAD(kmem);

/* each page is linked by a list whose node is located in the head of it */
void kfree(void *pa)
{
	assert(((uint64_t)pa % PGSIZE) == 0 and (char *) pa >= end and
	       (uint64_t) pa < PHYSTOP);

	struct list_node *r;
	r = (struct list_node *)pa;
	list_add_front(r, &kmem);
}
/**
 * Allocate one 4096-byte page of physical memory.
 * Returns a pointer that the kernel can use.
 * Returns 0 if the memory cannot be allocated.
 */
void *kalloc()
{
	if (list_empty(&kmem))
		return NULL;
	struct list_node *r = list_next(&kmem);
	list_del(kmem.next);
	return (void *)r;
}

static void freerange(void *pa_start, void *pa_end)
{
	char *p;
	p = (char *)PGROUNDUP((uint64_t)pa_start);
	for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
		kfree(p);
}

void phymem_init()
{
	freerange(end, (void *)PHYSTOP);
}
