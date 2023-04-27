#include "file.h"
#include <process/proc.h>
#include <sync/spinlock.h>
#include <uniks/errno.h>


// system open files table
struct file_t fcbtable[NFILE];
struct idle_fcb_queue_t idle_fcb_queue;


void sysfile_init()
{
	for (int32_t i = 0; i < NFILE; i++) {
		fcbtable[i].f_count = 0;
	}
	initlock(&idle_fcb_queue.idle_fcb_lock, "idlefcb");
	queue_init(&idle_fcb_queue.qm, NFILE,
		   idle_fcb_queue.idle_fcb_queue_array);
	for (int32_t i = 0; i < NFILE; i++)
		queue_push_int32type(&idle_fcb_queue.qm, i);
}

// allocate an idle file structure entry in fcb table
struct file_t *file_alloc()
{
	acquire(&idle_fcb_queue.idle_fcb_lock);
	if (queue_empty(&idle_fcb_queue.qm)) {
		release(&idle_fcb_queue.idle_fcb_lock);
		return NULL;
	}
	int32_t idlefcb = *(int32_t *)queue_front_int32type(&idle_fcb_queue.qm);
	queue_pop(&idle_fcb_queue.qm);
	assert(fcbtable[idlefcb].f_count == 0);
	fcbtable[idlefcb].f_count = 1;
	release(&idle_fcb_queue.idle_fcb_lock);
	return &fcbtable[idlefcb];
}

// free a file structure entry appointed by pointer f
void file_free(struct file_t *f)
{
	assert(f->f_count > 0);
	--f->f_count;
	if(f->f_count==0){
		acquire(&idle_fcb_queue.idle_fcb_lock);
		queue_push_int32type(&idle_fcb_queue.qm, f-fcbtable);
		release(&idle_fcb_queue.idle_fcb_lock);
	}
}