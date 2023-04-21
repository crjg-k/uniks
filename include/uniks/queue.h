#ifndef __QUEUE_H__
#define __QUEUE_H__


#include <uniks/defs.h>
#include <uniks/kassert.h>

/**
 * @brief a circular queue of int32_t type
 */
struct queue_meta_t {
	int32_t queue_size;
	int32_t queue_capacity;
	int32_t queue_head;
	int32_t queue_tail;
	union {
		int32_t *queue_array_int32type;
		char *queue_array_chartype;
	} queue_array;
};

void queue_init(struct queue_meta_t *q, int32_t capacity, void *heap_addr);

int32_t queue_empty(struct queue_meta_t *q);

int32_t queue_full(struct queue_meta_t *q);

/**
 * @brief if the queue is empty, this function will perform a ub
 *
 * @param q
 * @return void*
 */
void *queue_front_int32type(struct queue_meta_t *q);
void *queue_front_chartype(struct queue_meta_t *q);

void queue_push_int32type(struct queue_meta_t *q, int32_t push_data);
void queue_push_chartype(struct queue_meta_t *q, char push_data);

void queue_pop(struct queue_meta_t *q);

#endif /* !__QUEUE_H__ */