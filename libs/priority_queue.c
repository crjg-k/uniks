#include <uniks/kstdlib.h>
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

__always_inline int32_t __pair_less_than(struct pair_t *o1, struct pair_t *o2)
{
	if (o1->key < o2->key)
		return 1;
	else if (o1->key == o2->key and o1->value < o2->value)
		return 1;
	return 0;
}

__always_inline static void up(struct priority_queue_meta_t *pq,
			       int64_t root_no)
{
	int64_t half;
	while ((half = (root_no >> 1)) and
	       __pair_less_than(&pq->priority_queue_heap[root_no],
				&pq->priority_queue_heap[half])) {
		SWAP(pq->priority_queue_heap[root_no],
		     pq->priority_queue_heap[half], struct pair_t);
		root_no = half;
	}
}

__always_inline static void down(struct priority_queue_meta_t *pq,
				 int64_t root_no)
{
	int64_t doub;
	while ((doub = (root_no << 1)) <= pq->priority_queue_size) {
		if (root_no < pq->priority_queue_size and
		    __pair_less_than(&pq->priority_queue_heap[doub + 1],
				     &pq->priority_queue_heap[doub]))
			doub++;
		if (__pair_less_than(&pq->priority_queue_heap[doub],
				     &pq->priority_queue_heap[root_no]))
			SWAP(pq->priority_queue_heap[doub],
			     pq->priority_queue_heap[root_no], struct pair_t);
		else
			break;
		root_no = doub;
	}
}

void priority_queue_push(struct priority_queue_meta_t *pq, void *push_data)
{
	assert(pq->priority_queue_size < pq->priority_queue_capacity);
	struct pair_t *temp = (struct pair_t *)push_data;
	pq->priority_queue_heap[++pq->priority_queue_size] = *temp;
	up(pq, pq->priority_queue_size);
}

struct pair_t priority_queue_top(struct priority_queue_meta_t *pq)
{
	return pq->priority_queue_heap[1];
}

void priority_queue_pop(struct priority_queue_meta_t *pq)
{
	assert(!priority_queue_empty(pq));
	pq->priority_queue_heap[1] =
		pq->priority_queue_heap[pq->priority_queue_size--];
	down(pq, 1);
}

void priority_queue_pop_k(struct priority_queue_meta_t *pq, int64_t key)
{
	for (int64_t i = 1; i < pq->priority_queue_size; i++) {
		while (pq->priority_queue_heap[i].key == key) {
			pq->priority_queue_heap[i] =
				pq->priority_queue_heap
					[pq->priority_queue_size--];
			up(pq, i);
			down(pq, i);
		}
	}
}

void priority_queue_pop_v(struct priority_queue_meta_t *pq, int64_t value)
{
	for (int64_t i = 1; i < pq->priority_queue_size; i++) {
		while (pq->priority_queue_heap[i].value == value) {
			pq->priority_queue_heap[i] =
				pq->priority_queue_heap
					[pq->priority_queue_size--];
			up(pq, i);
			down(pq, i);
		}
	}
}