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

void priority_queue_push(struct priority_queue_meta_t *pq, void *push_data);

/**
 * @brief `HINT`: If the queue is empty, this function will perform a ub.
 * @param pq
 * @return struct pair_t
 */
struct pair_t priority_queue_top(struct priority_queue_meta_t *pq);

/**
 * @brief Pop up the element located at the top of the heap.
 * @param pq
 */
void priority_queue_pop(struct priority_queue_meta_t *pq);

/**
 * @brief Pop up all the elements that have the same key.
 * @param pq
 * @param key
 */
void priority_queue_pop_k(struct priority_queue_meta_t *pq, int64_t key);

/**
 * @brief Pop up all the elements that have the same value.
 * @param pq
 * @param value
 */
void priority_queue_pop_v(struct priority_queue_meta_t *pq, int64_t value);


#endif /* !__PRIORITY_QUEUE_H__ */