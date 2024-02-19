#ifndef __LIST_H__
#define __LIST_H__

#include <uniks/defs.h>

/* this list template is excerpted from linux:include/linux/list.h with url:
 * [https://github.com/torvalds/linux/blob/master/include/linux/list.h] */


#define offsetof(type, member) ((size_t)(&((type *)0)->member))

#define element_entry(ptr, type, member) \
	({ \
		const typeof(((type *)0)->member) *__mptr = (ptr); \
		(type *)((char *)__mptr - offsetof(type, member)); \
	})


struct list_node_t {
	struct list_node_t *prev, *next;
};

#define LIST_HEAD_GEN(name) \
	{ \
		&(name), &(name) \
	}

#define LIST_HEAD(name) struct list_node_t name = LIST_HEAD_GEN(name)

__always_inline void __list_add(struct list_node_t *new,
				struct list_node_t *prev,
				struct list_node_t *next)
{
	prev->next = next->prev = new;
	new->next = next;
	new->prev = prev;
}
__always_inline void __list_del(struct list_node_t *prev,

				struct list_node_t *next)
{
	next->prev = prev;
	prev->next = next;
}

__always_inline int32_t list_empty(const struct list_node_t *head)
{
	return head->next == head;
}
__always_inline void INIT_LIST_HEAD(struct list_node_t *list)
{
	list->next = list;
	list->prev = list;
}
__always_inline void list_add_front(struct list_node_t *new,
				    struct list_node_t *head)
{
	__list_add(new, head, head->next);
}
__always_inline void list_add_tail(struct list_node_t *new,
				   struct list_node_t *head)
{
	__list_add(new, head->prev, head);
}
__always_inline void list_del(struct list_node_t *elem)
{
	__list_del(elem->prev, elem->next);
}
__always_inline void list_del_then_init(struct list_node_t *listelm)
{
	list_del(listelm);
	INIT_LIST_HEAD(listelm);
}
__always_inline struct list_node_t *list_next(struct list_node_t *listelm)
{
	return listelm->next;
}
__always_inline struct list_node_t *list_prev(struct list_node_t *listelm)
{
	return listelm->prev;
}

__always_inline struct list_node_t *
list_next_then_del(struct list_node_t *listelm)
{
	struct list_node_t *node = listelm->next;
	list_del(node);
	return node;
}

__always_inline struct list_node_t *
list_prev_then_del(struct list_node_t *listelm)
{
	struct list_node_t *node = listelm->prev;
	list_del(node);
	return node;
}

#endif /* !__LIST_H__ */