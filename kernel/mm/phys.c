#include "phys.h"
#include "memlay.h"
#include "mmu.h"
#include <uniks/defs.h>
#include <uniks/kassert.h>
#include <uniks/kstdlib.h>
#include <uniks/kstring.h>
#include <uniks/list.h>


void phymem_init()
{
	assert((char *)KERNEL_END_LIMIT >= (char *)end);

	// physical page allocator
	buddy_system_init(KERNEL_END_LIMIT, PHYSTOP);
	// kmalloc allocator referenced by slub in Linux
	kmem_cache_init();
}


// === buddy system ===

struct phys_page_record_t physical_page_record[PHYMEM_AVAILABLE >> PGSHIFT];
struct list_node_t orderarray[11];   // 0~10 order
struct spinlock_t buddy_lock;
uintptr_t mem_start, mem_end;
enum order {
	ORD_0 = 0,
	ORD_1,
	ORD_2,
	ORD_3,
	ORD_4,
	ORD_5,
	ORD_6,
	ORD_7,
	ORD_8,
	ORD_9,
	ORD_10,
};

// mount all available blocks of order power to the orderarray
static void mount_orderlist(uintptr_t current, int32_t order)
{
	while (current + (1 << order) * PGSIZE <= mem_end) {
		list_add_front((struct list_node_t *)current,
			       &orderarray[order]);
		current += (1 << order) * PGSIZE;
	}
	if (order != ORD_0 and current != mem_end)
		mount_orderlist(current, order - 1);
}

/**
 * @brief Returns the minimum power after aligning `n` up to the power of 2.
 * @param n
 * @return int16_t
 */
static int16_t get_power2(uint32_t n)
{
	for (int32_t i = 0; i <= 1024; i++) {
		if (n <= (1 << i))
			return i;
	}

	BUG();
}

// always allocate the left side after the split
static struct list_node_t *split_pages(struct list_node_t *list,
				       int16_t this_order, int16_t target_order)
{
	while (this_order != target_order) {
		this_order--;
		void *right_half = (void *)list + (1 << this_order) * PGSIZE;
		list_add_front(right_half, &orderarray[this_order]);
		int32_t right_index = ADDR2ARRAYINDEX(right_half);
		assert(physical_page_record[right_index].count == 0);
		physical_page_record[right_index].order = this_order;
	}
	return list;
}

static void do_record(void *ptr, int16_t order)
{
	int32_t index = ADDR2ARRAYINDEX(ptr);
	assert(physical_page_record[index].count == 0);
	physical_page_record[index].count++;
	physical_page_record[index].order = order;
	initlock(&physical_page_record[index].lk, "perpg_lock");
}

void buddy_system_init(uintptr_t start, uintptr_t end)
{
	mem_start = start, mem_end = end;
	initlock(&buddy_lock, "buddylock");
	for (int32_t i = 0; i < 11; i++)
		INIT_LIST_HEAD(&orderarray[i]);
	mount_orderlist(mem_start, ORD_10);
	memset(physical_page_record, 0, sizeof(physical_page_record));
}

void *pages_alloc(size_t npages)
{
	assert(npages > 0 and npages <= (1 << ORD_10));
	void *ptr = NULL;
	int16_t order = get_power2(npages);

	acquire(&buddy_lock);
	// find in order correspond list
	if (!list_empty(&orderarray[order])) {
		ptr = (void *)list_next_then_del(&orderarray[order]);
		do_record(ptr, order);
		goto out;
	}
	/**
	 * @brief order correspond list hasn't any free pages, so find a lager
	 * and split it
	 */
	int16_t i = order;
	while (++i < 11)
		if (!list_empty(&orderarray[i]))
			break;
	if (i == 11) {
		// no more free pages, then will return NULL
		goto out;
	}
	assert(i > order and i < 11);
	ptr = (void *)split_pages(list_next_then_del(&orderarray[i]), i, order);
	do_record(ptr, order);
out:
	// assert that prt is aligned to a page
	assert(OFFSETPAGE((uintptr_t)ptr) == 0);
	release(&buddy_lock);
	// tracef("buddy system: allocate %d page(s) start @%p,", npages, ptr);
	assert(ptr != NULL);
	return ptr;
}

void *pages_zalloc(size_t npages)
{
	void *ptr = pages_alloc(npages);
	if (ptr != NULL)
		memset(ptr, 0, PGSIZE);
	return ptr;
}

void *pages_dup(void *ptr)
{
	int32_t index = ADDR2ARRAYINDEX(ptr);
	acquire(&physical_page_record[index].lk);
	assert(physical_page_record[index].count >= 1);
	physical_page_record[index].count++;
	release(&physical_page_record[index].lk);
	return ptr;
}

// This may be designed specifically for do_wp_page() function in COW scene.
int32_t pages_undup(void *ptr)
{
	int32_t index = ADDR2ARRAYINDEX(ptr), ret;
	acquire(&physical_page_record[index].lk);
	assert(physical_page_record[index].count >= 1);
	ret = physical_page_record[index].count;
	if (ret > 1)
		physical_page_record[index].count--;
	return ret;
}

void release_pglock(void *ptr)
{
	int32_t index = ADDR2ARRAYINDEX(ptr);
	assert(holding(&physical_page_record[index].lk));
	assert(physical_page_record[index].count >= 1);
	release(&physical_page_record[index].lk);
}

static void *whois_buddy(void *ptr, int16_t order)
{
	/**
	 * who is the buddy?
	 * if ptr % pow(2, k + 1) == 0:
	 *   the buddy is ptr + pow(2, k)
	 * else if ptr % pow(2, k + 1) == pow(2, k):
	 *   the buddy is ptr - pow(2, k)
	 * else BUG()
	 */
	if (((((uintptr_t)ptr - mem_start) >> PGSHIFT) &
	     ((1 << (order + 1)) - 1)) == 0)
		return ptr + ((1 << order) << PGSHIFT);
	else if (((((uintptr_t)ptr - mem_start) >> PGSHIFT) &
		  ((1 << (order + 1)) - 1)) == (1 << order))
		return ptr - ((1 << order) << PGSHIFT);
	else
		BUG();
}

// free npages and merge it if possible
void pages_free(void *ptr)
{
	assert(ptr != NULL);
	// assert that prt is aligned to a page
	assert(OFFSETPAGE((uintptr_t)ptr) == 0);

	acquire(&buddy_lock);
	int32_t index = ADDR2ARRAYINDEX(ptr);
	assert(physical_page_record[index].count > 0);
	if (--physical_page_record[index].count == 0) {
		// do free operation and merge operation
		int16_t order = physical_page_record[index].order;
		/* merge_pages() { */
		while (1) {
			void *buddyptr = whois_buddy(ptr, order);
			int32_t buddyindex = ADDR2ARRAYINDEX(buddyptr);
			if (physical_page_record[buddyindex].count == 0 and
			    physical_page_record[buddyindex].order == order) {
				/**
				 * @brief means that the buddy npages is in
				 * free, so we could do merge operation
				 */
				list_del(buddyptr);
				ptr = MIN(ptr, buddyptr);
				order++;
			} else {
				list_add_front(ptr, &orderarray[order]);
				break;
			}
		}
		/*}*/
	}
	release(&buddy_lock);
}


// === kmalloc implemented by slub allocator ===

int16_t slub_size[] = {
	16, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024, 1536,
};
#define SLUB_NODE_START(addr) (PGROUNDDOWN((uintptr_t)(addr)))
#define SLUBNUM		      (sizeof(slub_size) / sizeof(typeof(slub_size[0])))

struct kmem_cache_t {
	int32_t obj_size;
	struct spinlock_t kmem_cache_lock;
	struct list_node_t fulllist;
	struct list_node_t partiallist;
} kmem_cache_array[SLUBNUM];

struct slub_pages_node_t {
	struct kmem_cache_t *kmem_cache_linked;	  // used for finding back
	struct list_node_t slub_node_list;
	struct list_node_t obj_freelist;
};

void kmem_cache_init()
{
	for (int32_t i = 0; i < SLUBNUM; i++) {
		kmem_cache_array[i].obj_size = slub_size[i];
		initlock(&kmem_cache_array[i].kmem_cache_lock, "slublock");
		INIT_LIST_HEAD(&kmem_cache_array[i].fulllist);
		INIT_LIST_HEAD(&kmem_cache_array[i].partiallist);
	}
}

static struct slub_pages_node_t *new_slub_pages_node(int32_t idx)
{
	struct slub_pages_node_t *slub_pages_node = pages_alloc(1);
	if (slub_pages_node == NULL)
		goto ret;
	INIT_LIST_HEAD(&slub_pages_node->slub_node_list);
	INIT_LIST_HEAD(&slub_pages_node->obj_freelist);
	slub_pages_node->kmem_cache_linked = &kmem_cache_array[idx];
	// mount free object in this new page
	char *node = (char *)(slub_pages_node + 1);
	while (node + kmem_cache_array[idx].obj_size - (char *)slub_pages_node <
	       PGSIZE) {
		list_add_front((struct list_node_t *)node,
			       &slub_pages_node->obj_freelist);
		node += kmem_cache_array[idx].obj_size;
	}

ret:
	return slub_pages_node;
}

// 'ge' means greater than and equal
static int32_t binary_search_ge(const int16_t array[], int32_t array_size,
				size_t target)
{
	int32_t mid, l = 0, r = array_size;
	while (r - l > 0) {
		mid = (l + r) >> 1;
		if (array[mid] > target)
			r = mid;
		else if (array[mid] == target)
			return mid;
		else
			l = mid + 1;
	}
	return l;
}

// this couldn't allocate more than a page size memory area
void *kmalloc(size_t size)
{
	assert(size > 0 and size <= slub_size[SLUBNUM - 1]);
	int32_t idx = binary_search_ge(slub_size, SLUBNUM, size);

	void *ptr = NULL;
	acquire(&kmem_cache_array[idx].kmem_cache_lock);
	if (list_empty(&kmem_cache_array[idx].partiallist)) {
		struct slub_pages_node_t *slub_pages_node =
			new_slub_pages_node(idx);
		if (slub_pages_node == NULL)   // todo: add wait-list
			goto ret;
		list_add_front(
			&slub_pages_node->slub_node_list,
			&slub_pages_node->kmem_cache_linked->partiallist);
	}
	assert(!list_empty(&kmem_cache_array[idx].partiallist));
	struct slub_pages_node_t *slub_pages_node =
		element_entry(list_next(&kmem_cache_array[idx].partiallist),
			      struct slub_pages_node_t, slub_node_list);
	assert(!list_empty(&slub_pages_node->obj_freelist));
	ptr = list_next_then_del(&slub_pages_node->obj_freelist);
	if (list_empty(&slub_pages_node->obj_freelist)) {
		// remove from partial list and add to full list
		list_del(&slub_pages_node->slub_node_list);
		list_add_front(&slub_pages_node->slub_node_list,
			       &slub_pages_node->kmem_cache_linked->fulllist);
	}

ret:
	release(&kmem_cache_array[idx].kmem_cache_lock);
	return ptr;
}

void *kzalloc(size_t size)
{
	void *ptr = kmalloc(size);
	memset(ptr, 0, size);
	return ptr;
}

void kfree(void *ptr)
{
	if (ptr == NULL)
		return;
	struct slub_pages_node_t *slub_pages_node =
		(struct slub_pages_node_t *)SLUB_NODE_START(ptr);
	acquire(&slub_pages_node->kmem_cache_linked->kmem_cache_lock);
	if (list_empty(&slub_pages_node->obj_freelist)) {
		// remove from full list and add to partial list
		list_del(&slub_pages_node->slub_node_list);
		list_add_front(
			&slub_pages_node->slub_node_list,
			&slub_pages_node->kmem_cache_linked->partiallist);
	}
	list_add_front(ptr, &slub_pages_node->obj_freelist);
	release(&slub_pages_node->kmem_cache_linked->kmem_cache_lock);
}