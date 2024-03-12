#ifndef __KERNEL_LOADER_ELFLOADER_H__
#define __KERNEL_LOADER_ELFLOADER_H__


#include <file/file.h>
#include <fs/ext2fs.h>
#include <process/proc.h>
#include <uniks/defs.h>


int32_t load_elf(struct proc_t *p, struct mm_struct *mm,
		 struct m_inode_t *inode);


#endif