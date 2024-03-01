#ifndef __KERNEL_LOADER_ELFLOADER_H__
#define __KERNEL_LOADER_ELFLOADER_H__


#include <fs/ext2fs.h>
#include <file/file.h>
#include <process/proc.h>
#include <uniks/defs.h>

#define INVALID_ELFFORMAT 1


int32_t load_elf(struct proc_t *p, struct m_inode_info_t *inode);


#endif