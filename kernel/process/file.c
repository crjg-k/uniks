#include "file.h"
#include <sync/spinlock.h>
#include <uniks/param.h>

struct {
	struct spinlock_t lock;
	struct file_t file[NFILE];
} fcbtable;

void fileinit()
{
	initlock(&fcbtable.lock, "fcbtable");
}