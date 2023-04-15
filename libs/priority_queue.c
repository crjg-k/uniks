#include <uniks/priority_queue.h>


void priority_queue_init(struct priority_queue_meta_t *pq, int64_t capacity,
			 void *heap_addr)
{
	assert(capacity > 0);
	pq->priority_queue_size = 0;
	pq->priority_queue_capacity = capacity;
	pq->priority_queue_heap = (struct pair_t *)heap_addr;
}

int32_t priority_queue_empty(struct priority_queue_meta_t *pq)
{
	return pq->priority_queue_size == 0;
}

int32_t priority_queue_full(struct priority_queue_meta_t *pq)
{
	return pq->priority_queue_size == pq->priority_queue_capacity;
}

int32_t less_than_pair(struct pair_t *o1, struct pair_t *o2)
{
	if (o1->key < o2->key)
		return 1;
	else if (o1->key == o2->key and o1->value < o2->value)
		return 1;
	return 0;
}

void priority_queue_push(struct priority_queue_meta_t *pq, void *push_data)
{
	assert(pq->priority_queue_size < pq->priority_queue_capacity);
	pq->priority_queue_size++;
	int64_t now = pq->priority_queue_size;
	struct pair_t *temp = (struct pair_t *)push_data;
	while (now != 1) {
		if (less_than_pair(temp,
				     &pq->priority_queue_heap[now >> 1])) {
			pq->priority_queue_heap[now] =
				pq->priority_queue_heap[now >> 1];
			now >>= 1;
		} else
			break;
	}
	pq->priority_queue_heap[now] = *temp;
}

struct pair_t priority_queue_top(struct priority_queue_meta_t *pq)
{
	return pq->priority_queue_heap[1];
}

void priority_queue_pop(struct priority_queue_meta_t *pq)
{
	assert(!priority_queue_empty(pq));
	struct pair_t temp = pq->priority_queue_heap[pq->priority_queue_size];
	pq->priority_queue_size--;
	int64_t child = 2;
	while (child <= pq->priority_queue_size) {
		if (child < pq->priority_queue_size and
		    less_than_pair(&pq->priority_queue_heap[child + 1],
				     &pq->priority_queue_heap[child]))
			child++;
		if (less_than_pair(&pq->priority_queue_heap[child], &temp))
			pq->priority_queue_heap[child >> 1] =
				pq->priority_queue_heap[child];
		else
			break;
		child <<= 1;
	}
	pq->priority_queue_heap[child >> 1] = temp;
}