#ifndef __KERNEL_LOADER_ELFLOADER_H__
#define __KERNEL_LOADER_ELFLOADER_H__


#include <fs/fs.h>
#include <uniks/defs.h>

#define INVALID_ELFFORMAT 1


uint64_t load_elf(struct m_inode_t *inode);


#endif