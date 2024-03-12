#include "blk_dev.h"
#include "blkbuf.h"
#include <mm/mmu.h>
#include <mm/vm.h>
#include <process/proc.h>
#include <uniks/errno.h>


#define blkdev_rw_common1() \
	int64_t blk_no = pos / BLKSIZE, offset = OFFSETPAGE(pos), chars, \
		n = 0; \
	struct blkbuf_t *bb; \
	struct proc_t *p = myproc();

#define blkdev_rw_common2() \
	chars = BLKSIZE - offset; \
	if (chars > cnt) \
		chars = cnt;

#define blkdev_rw_common3() \
	offset = 0; \
	n += chars; \
	cnt -= chars;

int64_t blkdev_read(dev_t dev, char *buf, int64_t pos, size_t cnt)
{
	blkdev_rw_common1();

	while (cnt > 0) {
		blkdev_rw_common2();

		if ((bb = blk_read(dev, blk_no++)) == NULL)
			return n ? n : -EIO;
		assert(copyout(p->mm->pagetable, buf, bb->b_data + offset,
			       chars) != -1);

		blkdev_rw_common3();

		blk_release(bb);
	}

	return n;
}

int64_t blkdev_write(dev_t dev, char *buf, int64_t pos, size_t cnt)
{
	blkdev_rw_common1();

	while (cnt > 0) {
		blkdev_rw_common2();

		/**
		 * @brief If there is exactly one block to be written, apply for
		 * one block buffer block directly. Otherwise, the data block
		 * that will be modified needs to be read in. Then, increase the
		 * block number by one.
		 */
		if (chars == BLKSIZE)
			bb = getblk(dev, blk_no);
		else
			bb = blk_read(dev, blk_no);
		if (bb == NULL)
			return n ? n : -EIO;
		blk_no++;
		assert(copyin(p->mm->pagetable, bb->b_data + offset, buf,
			      chars) != -1);

		blkdev_rw_common3();

		blk_write_over(bb);
	}

	return n;
}