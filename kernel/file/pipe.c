#include "pipe.h"
#include <fs/ext2fs.h>
#include <mm/mmu.h>
#include <mm/phys.h>
#include <process/proc.h>


struct m_inode_t *pipealloc()
{
	struct m_inode_t *ip;
	char *pipe_pg = pages_alloc(1);
	if (pipe_pg == NULL)
		return NULL;

	acquire(&inode_table.lock);
	while (1) {
		for (ip = inode_table.m_inodes;
		     ip < &inode_table.m_inodes[NINODE]; ip++) {
			if (ip->i_count == 0) {
				ip->i_count++;
				ip->i_dev = 0;
				release(&inode_table.lock);
				goto found;
			}
		}
		proc_block(&inode_table.wait_list, &inode_table.lock);
	}
found:
	ip->d_inode_ctnt.i_mode = EXT2_S_IFIFO;

	struct pipe_t *pi = &ip->pipe_node;
	// Initialize pipe related data.
	pi->readopen = pi->writeopen = 1;
	initlock(&pi->lk, "pipelk");
	INIT_LIST_HEAD(&pi->read_wait);
	INIT_LIST_HEAD(&pi->write_wait);

	queue_init(&pi->pipe, PGSIZE, pipe_pg);

	return ip;
}

void pipeclose(struct pipe_t *pi, int32_t W)
{
	acquire(&pi->lk);
	if (W) {
		pi->writeopen = 0;
		proc_unblock_all(&pi->read_wait);
	} else {
		pi->readopen = 0;
		proc_unblock_all(&pi->write_wait);
	}

	if (pi->readopen == 0 and pi->writeopen == 0) {
		pages_free(pi->pipe.queue_array_chartype);
	}
	release(&pi->lk);
}

int64_t pipewrite(struct pipe_t *pi, void *addr, size_t n)
{
	int64_t i = 0;
	struct proc_t *p = myproc();

	acquire(&pi->lk);
	while (i < n) {
		if (pi->readopen == 0 or killed(p)) {
			release(&pi->lk);
			return 0;
		}
		if (queue_full(&pi->pipe)) {
			// pipewrite-full
			proc_unblock_all(&pi->read_wait);
			proc_block(&pi->write_wait, &pi->lk);
		} else {
			char ch;
			assert(copyin(p->mm->pagetable, &ch, addr + i, 1) !=
			       -1);
			queue_push_chartype(&pi->pipe, ch);
			i++;
		}
	}
	proc_unblock_all(&pi->read_wait);
	release(&pi->lk);

	return i;
}

int64_t piperead(struct pipe_t *pi, void *addr, size_t n)
{
	int64_t i;
	struct proc_t *p = myproc();

	acquire(&pi->lk);
	while (queue_empty(&pi->pipe) and pi->writeopen) {
		// pipe-empty
		if (killed(p)) {
			release(&pi->lk);
			return 0;
		}
		proc_block(&pi->read_wait, &pi->lk);   // piperead-block
	}
	for (i = 0; i < n; i++) {
		if (queue_empty(&pi->pipe))
			break;
		char ch = *(char *)queue_front_chartype(&pi->pipe);
		queue_front_pop(&pi->pipe);
		assert(copyout(p->mm->pagetable, addr + i, &ch, 1) != -1);
	}
	proc_unblock_all(&pi->write_wait);   // piperead-unblock
	release(&pi->lk);

	return i;
}
