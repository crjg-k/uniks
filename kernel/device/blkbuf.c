#include "blkbuf.h"
#include "virtio_disk.h"
#include <device/device.h>
#include <mm/mmu.h>
#include <mm/phys.h>
#include <platform/platform.h>
#include <process/proc.h>
#include <uniks/param.h>


volatile atomic_uint_least32_t syncing = ATOMIC_VAR_INIT(0),
			       rw_operating = ATOMIC_VAR_INIT(0);
struct blk_cache_t blk_cache;
char zero_blk[BLKSIZE] = {0};

#define _hashfn(dev, block) (((uint32_t)((dev) ^ (block))) % HASH_TABLE_PRIME)
#define hash(dev, block)    (blk_cache.hash_bucket_table[_hashfn((dev), (block))])

void blk_init()
{
	initlock(&blk_cache.lock, "blk_cache");

	for (int64_t i = 0; i < HASH_TABLE_PRIME; i++) {
		INIT_LIST_HEAD(&blk_cache.hash_bucket_table[i]);
	}
	INIT_LIST_HEAD(&blk_cache.free_list);
	INIT_LIST_HEAD(&blk_cache.wait_list);

	for (struct blkbuf_t *bb = blk_cache.blkbuf;
	     bb < &blk_cache.blkbuf[NBBUF]; bb++) {
		bb->b_count = bb->b_dirty = bb->b_hashed = 0;
		bb->b_data = NULL;
		INIT_LIST_HEAD(&bb->disk_wait_list);
		mutex_init(&bb->b_mtx, "blkbufmtx");
		list_add_front(&bb->free_node, &blk_cache.free_list);
	}
}

/**
 * @brief Find a cached block for a given device number and block number in the
 * cache. If found, returns a corresponding pointer. Otherwise return NULL.
 * @param dev
 * @param blkno
 * @return struct blkbuf_t*
 */
static struct blkbuf_t *find_buffer_inhash(dev_t dev, uint32_t blkno)
{
	struct list_node_t *l;
	struct list_node_t *bucket = &hash(dev, blkno);

	for (l = list_next(bucket); l != bucket; l = list_next(l)) {
		struct blkbuf_t *bb =
			element_entry(l, struct blkbuf_t, hash_node);
		if (bb->b_dev == dev and bb->b_blkno == blkno) {
			return bb;
		}
	}
	return NULL;
}

static struct blkbuf_t *get_LRU_blk()
{
	struct list_node_t *l;
	try:
		if (!list_empty(&blk_cache.free_list)) {
			l = list_prev_then_del(&blk_cache.free_list);
		} else {
			proc_block(&blk_cache.wait_list, &blk_cache.lock);
			goto try;
		}
	return element_entry(l, struct blkbuf_t, free_node);
}

static void rmold_then_insert_newhash(struct blkbuf_t *bb, dev_t dev,
				      uint32_t blkno)
{
	if (bb->b_hashed)
		list_del(&bb->hash_node);

	struct list_node_t *bucket = &hash(dev, blkno);
	list_add_front(&bb->hash_node, bucket);
	bb->b_hashed = 1;
}

/**
 * @brief Look through buffer for block on device `dev`. Make sure that return a
 * buffered block with mutex-lock holding, and also the buffer of a disk's block
 * is unique in global.
 * @param dev
 * @param blkno
 * @return struct blkbuf_t*
 */
struct blkbuf_t *getblk(dev_t dev, uint32_t blkno)
{
	struct blkbuf_t *bb;
	acquire(&blk_cache.lock);

	// Is the block already cached?
	if ((bb = find_buffer_inhash(dev, blkno)) != NULL) {
		if (bb->b_count++ == 0)
			list_del(&bb->free_node);
		goto ret;
	}

	/**
	 * @brief Don't find in hash table (namely not cached and need to
	 * get a new block and read from disk).
	 */
	bb = get_LRU_blk();
	rmold_then_insert_newhash(bb, dev, blkno);
	bb->b_dev = dev;
	bb->b_blkno = blkno;
	bb->b_count++;
	if (bb->b_data == NULL) {
		if ((bb->b_data = pages_alloc(1)) == NULL) {
			release(&blk_cache.lock);
			return NULL;
		}
	}

ret:
	release(&blk_cache.lock);
	mutex_acquire(&bb->b_mtx);
	return bb;   // return with mutex-lock holding
}

/**
 * @brief Release a locked buffer. And do LRU algorithm.
 * @param bb
 */
void blk_release(struct blkbuf_t *bb)
{
	mutex_release(&bb->b_mtx);
	acquire(&blk_cache.lock);
	if (--bb->b_count == 0) {   // no one is referring it
		// LRU strategy
		list_add_front(&bb->free_node, &blk_cache.free_list);
		proc_unblock_all(&blk_cache.wait_list);
	}
	release(&blk_cache.lock);
}

// return a locked buffered block with the contents of the indicated block
struct blkbuf_t *blk_read(dev_t dev, uint32_t blockno)
{
	struct blkbuf_t *bb = getblk(dev, blockno);
	if (bb == NULL)
		goto ret;

	if (bb->b_dirty and (bb->b_dev != dev and bb->b_blkno != blockno)) {
		// write-back strategy
		assert(bb->b_data != NULL);
		device_write(dev, 0, bb, PGSIZE);
		bb->b_dirty = 0;
	}
	if (bb->b_valid == 0)
		device_read(dev, 0, bb, PGSIZE);
	/**
	 * @brief If disk operation success, bb->b_valid will be set as 1 in
	 * do_virtio_disk_interrupt().
	 */
	if (bb->b_valid == 1)
		return bb;

	// indicate that disk operation failed
	blk_release(bb);

ret:
	return NULL;
}

// Write-back strategy, so just mark it dirty.
void blk_write_over(struct blkbuf_t *bb)
{
	assert(mutex_holding(&bb->b_mtx));
	bb->b_dirty = 1;
	blk_release(bb);
}

void blk_sync_all(int32_t still_block)
{
	atomic_fetch_add(&syncing, 1);
	while (atomic_load(&rw_operating) > 0)
		;

	for (struct blkbuf_t *bb = blk_cache.blkbuf;
	     bb < &blk_cache.blkbuf[NBBUF]; bb++) {
		mutex_acquire(&bb->b_mtx);
		if (bb->b_dirty) {
			assert(bb->b_data != NULL);
			device_write(bb->b_dev, 0, bb, PGSIZE);
			bb->b_dirty = 0;
		}
		mutex_release(&bb->b_mtx);
	}

	if (!still_block)
		atomic_fetch_sub(&syncing, 1);
}