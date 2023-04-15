#ifndef __PRIORITY_QUEUE_H__
#define __PRIORITY_QUEUE_H__


#include <uniks/defs.h>
#include <uniks/kassert.h>

struct pair_t {
	int64_t key;
	int64_t value;
};

/**
 * @brief a priority queue of int32_t type
 */
struct priority_queue_meta_t {
	int64_t priority_queue_size;
	int64_t priority_queue_capacity;
	struct pair_t *priority_queue_heap;
};


void priority_queue_init(struct priority_queue_meta_t *pq, int64_t capacity,
			 void *heap_addr);

int32_t priority_queue_empty(struct priority_queue_meta_t *pq);

int32_t priority_queue_full(struct priority_queue_meta_t *pq);

int32_t less_than_pair_t(struct pair_t *o1, struct pair_t *o2);

void priority_queue_push(struct priority_queue_meta_t *pq, void *push_data);

/**
 * @brief if the priority queue is empty, this function will perform a ub
 *
 * @param pq
 * @return struct pair_t
 */
struct pair_t priority_queue_top(struct priority_queue_meta_t *pq);

void priority_queue_pop(struct priority_queue_meta_t *pq);

#endif /* !__PRIORITY_QUEUE_H__ */