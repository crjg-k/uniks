#ifndef __KERNEL_LOADER_ELFLOADER_H__
#define __KERNEL_LOADER_ELFLOADER_H__


#include <fs/fs.h>
#include <process/proc.h>
#include <uniks/defs.h>

#define INVALID_ELFFORMAT 1


int32_t load_elf(struct proc_t *p, struct m_inode_t *inode);


#endif