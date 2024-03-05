#ifndef __QUEUE_H__
#define __QUEUE_H__


#include <uniks/defs.h>
#include <uniks/kassert.h>


struct queue_meta_t {
	int32_t queue_size;
	int32_t queue_capacity;
	int32_t queue_head;
	int32_t queue_tail;
	union {
		int32_t *queue_array_int32type;
		char *queue_array_chartype;
	};
};

void queue_init(struct queue_meta_t *q, int32_t capacity, void *heap_addr);

int32_t queue_empty(struct queue_meta_t *q);

int32_t queue_full(struct queue_meta_t *q);

// if the queue is empty, this function will perform a ub
void *queue_front_int32type(struct queue_meta_t *q);
// if the queue is empty, this function will perform a ub
void *queue_front_chartype(struct queue_meta_t *q);

void queue_push_int32type(struct queue_meta_t *q, int32_t push_data);
void queue_push_chartype(struct queue_meta_t *q, char push_data);

void queue_front_pop(struct queue_meta_t *q);
void queue_back_pop(struct queue_meta_t *q);


#endif /* !__QUEUE_H__ */