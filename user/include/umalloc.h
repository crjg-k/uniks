#ifndef __USER_INCLUDE_UMALLOC_H__
#define __USER_INCLUDE_UMALLOC_H__


#define PGSIZE 4096


void *malloc(unsigned int nbytes);
void free(void *ap);


#endif /* !__USER_INCLUDE_UMALLOC_H__ */