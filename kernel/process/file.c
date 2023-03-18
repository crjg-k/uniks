#include "file.h"
#include <param.h>
#include <sync/spinlock.h>

struct {
	struct spinlock lock;
	struct file file[NFILE];
} fcbtable;

void fileinit()
{
	initlock(&fcbtable.lock, "fcbtable");
}