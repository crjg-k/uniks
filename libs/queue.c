#include <uniks/queue.h>


void queue_init(struct queue_meta_t *q, int32_t capacity, void *heap_addr)
{
	assert(capacity > 0);
	q->queue_head = q->queue_size = 0;
	q->queue_tail = -1;
	q->queue_capacity = capacity;
	q->queue_array.queue_array_int32type = heap_addr;
}

int32_t queue_empty(struct queue_meta_t *q)
{
	return q->queue_size == 0;
}

int32_t queue_full(struct queue_meta_t *q)
{
	return q->queue_size == q->queue_capacity;
}

void *queue_front(struct queue_meta_t *q)
{
	return &q->queue_array.queue_array_int32type[q->queue_head];
}

void queue_push_int32type(struct queue_meta_t *q, int32_t push_data)
{
	assert(q->queue_size < q->queue_capacity);
	q->queue_tail++;
	if (q->queue_tail >= q->queue_capacity)
		q->queue_tail -= q->queue_capacity;
	q->queue_array.queue_array_int32type[q->queue_tail] = push_data;
	q->queue_size++;
}
void queue_push_chartype(struct queue_meta_t *q, char push_data)
{
	assert(q->queue_size < q->queue_capacity);
	q->queue_tail++;
	if (q->queue_tail >= q->queue_capacity)
		q->queue_tail -= q->queue_capacity;
	q->queue_array.queue_array_chartype[q->queue_tail] = push_data;
	q->queue_size++;
}

void queue_pop(struct queue_meta_t *q)
{
	assert(!queue_empty(q));
	q->queue_head++;
	if (q->queue_head >= q->queue_capacity)
		q->queue_head -= q->queue_capacity;
	q->queue_size--;
}