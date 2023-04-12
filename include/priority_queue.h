#ifndef __PRIORITY_QUEUE_H__
#define __PRIORITY_QUEUE_H__


#include <defs.h>
#include <kassert.h>

/**
 * @brief a priority queue of int32_t type
 */
struct priority_queue_meta {
	uint32_t priority_queue_size;
	uint32_t priority_queue_capacity;
	int32_t *priority_queue_heap;
};

void priority_queue_init(struct priority_queue_meta *pq, uint32_t capacity,
			 int32_t *heap_addr)
{
	assert(capacity > 0);
	pq->priority_queue_size = 0;
	pq->priority_queue_capacity = capacity;
	pq->priority_queue_heap = heap_addr;
}

int32_t priority_queue_empty(struct priority_queue_meta *pq)
{
	return pq->priority_queue_size == 0;
}

int32_t priority_queue_full(struct priority_queue_meta *pq)
{
	return pq->priority_queue_size == pq->priority_queue_capacity;
}

void priority_queue_push(struct priority_queue_meta *pq, int32_t push_data)
{
	assert(pq->priority_queue_size < pq->priority_queue_capacity);
	pq->priority_queue_size++;
	int32_t now = pq->priority_queue_size;
	while (now != 1) {
		if (push_data < pq->priority_queue_heap[now >> 1]) {
			pq->priority_queue_heap[now] =
				pq->priority_queue_heap[now >> 1];
			now >>= 1;
		} else
			break;
	}
	pq->priority_queue_heap[now] = push_data;
}

/**
 * @brief if the priority queue is empty, this function will perform a ub
 *
 * @param pq
 * @return int32_t
 */
int32_t priority_queue_top(struct priority_queue_meta *pq)
{
	return pq->priority_queue_heap[1];
}

void priority_queue_pop(struct priority_queue_meta *pq)
{
	assert(!priority_queue_empty(pq));
	int32_t value = pq->priority_queue_heap[pq->priority_queue_size];
	pq->priority_queue_size--;
	int32_t child = 2;
	while (child <= pq->priority_queue_size) {
		if (child < pq->priority_queue_size and
		    pq->priority_queue_heap[child + 1] <
			    pq->priority_queue_heap[child])
			child++;
		if (pq->priority_queue_heap[child] < value)
			pq->priority_queue_heap[child >> 1] =
				pq->priority_queue_heap[child];
		else
			break;
		child <<= 1;
	}
	pq->priority_queue_heap[child >> 1] = value;
}


#endif /* !__PRIORITY_QUEUE_H__ */