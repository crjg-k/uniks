#include "memlay.h"
#include "mmu.h"
#include <uniks/defs.h>
#include <uniks/kassert.h>
#include <uniks/kstring.h>

#define USED   1
#define UNUSED 0

int16_t mem_map[PHYSIZE >> PGSHIFT];
int32_t freepagenum;


void phymem_free_page(void *pa)
{
	assert(((uint64_t)pa % PGSIZE) == 0 and (char *) pa >= end and
	       (uint64_t) pa < PHYSTOP);

	mem_map[PA2ARRAYINDEX((uint64_t)pa)]--;
	if (mem_map[PA2ARRAYINDEX((uint64_t)pa)] == 0)
		freepagenum++;
}

/**
 * @brief Allocate one 4096-byte page of physical memory. Returns a pointer that
 * the kernel can use. Returns NULL if the memory cannot be allocated (namely
 * all pages are used now).
 * @return void*
 */
void *phymem_alloc_page()
{
	for (int32_t i = PA2ARRAYINDEX(PGROUNDUP((uint64_t)end));
	     i < PA2ARRAYINDEX(PHYSTOP); i++)
		if (mem_map[i] == UNUSED) {
			mem_map[i]++;
			freepagenum--;
			return (void *)ARRAYINDEX2PA((uint64_t)i);
		}
	return NULL;
}

static void freerange(void *pa_start, void *pa_end)
{
	for (int32_t i = PA2ARRAYINDEX((uint64_t)pa_start);
	     i < PA2ARRAYINDEX((uint64_t)pa_end); i++) {
		mem_map[i] = UNUSED;
		freepagenum++;
	}
}

void phymem_init()
{
	freepagenum = 0;
	// the front is used by OpenSBI and kernel code and data
	for (int32_t i = 0; i <= PA2ARRAYINDEX((uint64_t)(end)); i++)
		mem_map[i] = USED;
	freerange((void *)PGROUNDUP((uint64_t)end), (void *)PHYSTOP);
}
