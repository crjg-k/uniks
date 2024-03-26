#include <udefs.h>
#include <ulib.h>
#include <umalloc.h>


// Memory allocator by Kernighan and Ritchie,
// The C programming Language, 2nd ed.  Section 8.7.

typedef long Align;

union header_t {
	struct {
		union header_t *ptr;
		unsigned int size;
	} s;
	Align x;
};

static union header_t base;
static union header_t *freep;

static union header_t *morecore(unsigned int nu)
{
	char *p;
	union header_t *hp;

	if (nu < PGSIZE)
		nu = PGSIZE;
	p = sbrk(nu * sizeof(union header_t));
	if (p == (char *)-1)
		return 0;
	hp = (union header_t *)p;
	hp->s.size = nu;
	free((void *)(hp + 1));
	return freep;
}

void *malloc(unsigned int nbytes)
{
	union header_t *p, *prevp;
	unsigned int nunits;

	nunits =
		(nbytes + sizeof(union header_t) - 1) / sizeof(union header_t) +
		1;
	if ((prevp = freep) == 0) {
		base.s.ptr = freep = prevp = &base;
		base.s.size = 0;
	}
	for (p = prevp->s.ptr;; prevp = p, p = p->s.ptr) {
		if (p->s.size >= nunits) {
			if (p->s.size == nunits)
				prevp->s.ptr = p->s.ptr;
			else {
				p->s.size -= nunits;
				p += p->s.size;
				p->s.size = nunits;
			}
			freep = prevp;
			return (void *)(p + 1);
		}
		if (p == freep)
			if ((p = morecore(nunits)) == 0)
				return 0;
	}
}

void free(void *ap)
{
	union header_t *bp, *p;

	bp = (union header_t *)ap - 1;
	for (p = freep; !(bp > p and bp < p->s.ptr); p = p->s.ptr)
		if (p >= p->s.ptr and (bp > p or bp < p->s.ptr))
			break;
	if (bp + bp->s.size == p->s.ptr) {
		bp->s.size += p->s.ptr->s.size;
		bp->s.ptr = p->s.ptr->s.ptr;
	} else
		bp->s.ptr = p->s.ptr;
	if (p + p->s.size == bp) {
		p->s.size += bp->s.size;
		p->s.ptr = bp->s.ptr;
	} else
		p->s.ptr = bp;
	freep = p;
}
