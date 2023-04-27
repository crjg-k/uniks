#include "blkbuf.h"
#include <device/virtio_disk.h>
#include <process/proc.h>
#include <uniks/param.h>


struct blkbuf_t blkbuf[NBUF];
struct free_list_head_t free_list_head;
struct hash_bucket_t hash_table[BLKBUF_HASH_PRIME];

void blkbuf_init()
{
	for (int32_t i = 0; i < BLKBUF_HASH_PRIME; i++) {
		INIT_LIST_HEAD(&hash_table[i].bucket_head);
		initlock(&hash_table[i].hash_lock, "hashbucket");
	}
	INIT_LIST_HEAD(&free_list_head.free_head);
	INIT_LIST_HEAD(&free_list_head.wait_list);
	initlock(&free_list_head.free_list_head_lock, "freelisthead");
	for (struct blkbuf_t *bb = blkbuf; bb < &blkbuf[NBUF]; bb++) {
		bb->b_count = bb->b_valid = bb->b_dirty = 0;
		mutex_init(&bb->b_mtx, "blkbufmtx");
		INIT_LIST_HEAD(&bb->hash_node);
		list_add_front(&bb->free_node, &free_list_head.free_head);
	}
}

/**
 * @brief Find a buffered block for a given device number and block number in
 * the buffer. If found, returns a corresponding pointer Otherwise, return NULL
 * @param dev
 * @param blkno
 * @return struct blkbuf_t*
 */
static struct blkbuf_t *find_buffer(dev_t dev, uint32_t blkno)
{
	struct list_node_t *l;
	struct hash_bucket_t *bucket = &hash(dev, blkno);

	acquire(&bucket->hash_lock);
	for (l = list_next(&bucket->bucket_head); l != &bucket->bucket_head;
	     l = list_next(l)) {
		struct blkbuf_t *bb =
			element_entry(l, struct blkbuf_t, free_node);
		if (bb->b_dev == dev and bb->b_blkno == blkno) {
			release(&bucket->hash_lock);
			return bb;
		}
	}
	release(&bucket->hash_lock);
	return NULL;
}

struct blkbuf_t *get_hash_table(dev_t dev, uint32_t blkno)
{
	struct blkbuf_t *bb;

	while (1) {
		/**
		 * @brief Find the buffer for the given device and specified
		 * block in the buffer, and return NULL if not found.
		 */
		if ((bb = find_buffer(dev, blkno)) == NULL)
			return NULL;

		/**
		 * @brief inc reference count, and wait for unlock (if it had
		 * been locked by other processes)
		 */
		mutex_acquire(&bb->b_mtx);
		bb->b_count++;

		/**
		 * @brief there is a need to verify the rightness of this
		 * buffered block again since experienced a blocked state
		 */
		if (bb->b_dev == dev and bb->b_blkno == blkno)
			return bb;
		/**
		 * @brief
		 * if the device number or block number of bb had been modified,
		 * then withdraw the reference count and refind
		 */
		bb->b_count--;
		mutex_release(&bb->b_mtx);
	}
}

void remove_then_insert_newhash(struct blkbuf_t *bb, dev_t dev, uint32_t blkno)
{
	struct hash_bucket_t *bucket = &hash(bb->b_dev, bb->b_blkno);
	acquire(&bucket->hash_lock);
	list_del(&bb->hash_node);
	release(&bucket->hash_lock);

	bucket = &hash(dev, blkno);
	acquire(&bucket->hash_lock);
	list_add_front(&bb->hash_node, &bucket->bucket_head);
	release(&bucket->hash_lock);
}

/**
 * @brief Look through buffer for block on device dev. Make sure that return a
 * buffered block with mutex-lock holding, and also the buffer of a disk's block
 * is unique in global. Furthermore, returned block will not on free_list.
 * note: I assume that this function is bug free
 * @param dev
 * @param blkno
 * @return struct blkbuf_t*
 */
struct blkbuf_t *getblk(dev_t dev, uint32_t blkno)
{
	/**
	 * note: when acquire a mutex lock for a free block, must goto repeat to
	 * note: check if any other process had added requesting block into
	 * note: hash_table
	 */
	struct blkbuf_t *bb;
repeat:
	if ((bb = get_hash_table(dev, blkno)) != NULL) {
		acquire(&free_list_head.free_list_head_lock);
		list_del(&bb->free_node);
		release(&free_list_head.free_list_head_lock);
		return bb;
	}

	/**
	 * @brief don't find in hash table (namely not buffered and need to
	 * read from disk)
	 */
	acquire(&free_list_head.free_list_head_lock);
	if (list_empty(&free_list_head.free_head)) {
		// if there are no free block in free list, then block
		proc_block(myproc(), &free_list_head.wait_list, TASK_BLOCK);
		release(&free_list_head.free_list_head_lock);
		sched();
		// nothing to do but have to repeat the above procedure
		goto repeat;
	}
	assert(!list_empty(&free_list_head.free_head));

	bb = element_entry(list_prev(&free_list_head.free_head),
			   struct blkbuf_t, free_node);
	mutex_acquire(&bb->b_mtx);
	if (bb->b_count) {
		release(&free_list_head.free_list_head_lock);
		mutex_release(&bb->b_mtx);
		goto repeat;
	}

	/**
	 * @brief if this block is dirty, we are supposed to write it back to
	 * disk and wait it unblock. And we have nothing to do but repeat the
	 * above procedure as this block had been used by another processes.
	 */
	while (bb->b_dirty) {
		blk_write(bb);
		if (bb->b_count) {
			release(&free_list_head.free_list_head_lock);
			mutex_release(&bb->b_mtx);
			goto repeat;
		}
	}

	/**
	 * note: when this process blocked because of waiting this block, other
	 * note: process may have add this block to the hash table, so check
	 * note: hash table again and repeat above procedure if it is true.
	 */
	if (find_buffer(dev, blkno)) {
		release(&free_list_head.free_list_head_lock);
		mutex_release(&bb->b_mtx);
		goto repeat;
	}

	/**
	 * @brief OK! Now, we have a unused(b_count=0), unlocked and clean
	 * buffered block which is one2one mapping to the argument <dev, blkno>,
	 * so we occupy this by set b_count as 1 and reset another flags.
	 */
	assert(bb->b_count == 0);
	assert(bb->b_dirty == 0);
	bb->b_count = 1;
	// push it into corresponding hash table
	remove_then_insert_newhash(bb, dev, blkno);
	bb->b_dev = dev;
	bb->b_blkno = blkno;
	list_del(&bb->free_node);
	release(&free_list_head.free_list_head_lock);

	return bb;   // return with mutex-lock holding
}

/**
 * @brief Release a locked buffer. And do LRU algorithm.
 * @param bb
 */
void brelease(struct blkbuf_t *bb)
{
	assert(mutex_holding(&bb->b_mtx));
	assert(bb->b_count == 1);
	bb->b_count--;

	acquire(&free_list_head.free_list_head_lock);
	// LRU strategy, insert into the head of free_list
	list_add_front(&bb->free_node, &free_list_head.free_head);
	proc_unblock_all(&free_list_head.wait_list);
	release(&free_list_head.free_list_head_lock);
	mutex_release(&bb->b_mtx);
}

// return a locked buffered block with the contents of the indicated block
struct blkbuf_t *blk_read(dev_t dev, uint32_t blockno)
{
	struct blkbuf_t *bb = getblk(dev, blockno);
	// if bb is NULL, kernerl or getblk() implementation oops!
	assert(bb != NULL);
	if (bb->b_valid == 0)
		virtio_disk_read(bb);
	if (bb->b_valid == 1)
		return bb;
	// indicate that disk operation failed
	brelease(bb);
	return NULL;
}

// write bb's contents to disk and must be locked before called this function
void blk_write(struct blkbuf_t *bb)
{
	assert(mutex_holding(&bb->b_mtx));
	virtio_disk_write(bb);
	bb->b_dirty = 0;
}