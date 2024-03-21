#include "elfloader.h"
#include "elf.h"
#include <fs/ext2fs.h>
#include <mm/mmu.h>
#include <mm/phys.h>
#include <mm/vm.h>
#include <platform/riscv.h>
#include <process/proc.h>
#include <uniks/kassert.h>
#include <uniks/kstdlib.h>
#include <uniks/kstring.h>


static int32_t elf_validate(struct Elf64_Ehdr_t *elf_header)
{
	if (memcmp(&elf_header->e_ident, ELFMAGIC, 4))
		return -1;
	if (elf_header->e_ident[EI_CLASS] != ELFCLASS64)
		return -1;
	if (elf_header->e_ident[EI_DATA] != ELFDATA2LSB)
		return -1;
	if (elf_header->e_ident[EI_VERSION] != EV_CURRENT)
		return -1;
	if (elf_header->e_type != ET_EXEC)
		return -1;
	if (elf_header->e_machine != EM_RISCV)
		return -1;
	if (elf_header->e_version != EV_CURRENT)
		return -1;
	if (elf_header->e_phentsize != sizeof(struct Elf64_Phdr_t))
		return -1;
	return 0;
}

static int32_t pre_load_segment(struct mm_struct *mm, struct m_inode_t *inode,
				struct Elf64_Phdr_t *prgm_header)
{
	assert(prgm_header->p_align == PGSIZE);		 // align to page
	assert(OFFSETPAGE(prgm_header->p_vaddr) == 0);	 // align to page

	int32_t perm = PTE_U;
	if (get_var_bit(prgm_header->p_flags, PF_X))
		set_var_bit(perm, PTE_X);
	if (get_var_bit(prgm_header->p_flags, PF_R))
		set_var_bit(perm, PTE_R);
	if (get_var_bit(prgm_header->p_flags, PF_W))
		set_var_bit(perm, PTE_W);

	return add_vm_area(
		mm, prgm_header->p_vaddr,
		PGROUNDUP(prgm_header->p_vaddr + prgm_header->p_memsz), perm,
		prgm_header->p_offset, inode, prgm_header->p_filesz);
}

uint64_t load_elf(struct mm_struct *mm, struct m_inode_t *inode)
{
	int32_t res = 0;
	struct Elf64_Ehdr_t elf_header;

	ilock(inode);
	if (readi(inode, 0, (void *)&elf_header, 0, sizeof(elf_header)) !=
	    sizeof(elf_header))
		goto ret;
	if (elf_validate(&elf_header) != 0)
		goto ret;

	struct Elf64_Phdr_t prgm_header;
	for (int32_t i = 0; i < elf_header.e_phnum; i++) {
		if (readi(inode, 0, (void *)&prgm_header,
			  elf_header.e_phoff + i * sizeof(prgm_header),
			  sizeof(prgm_header)) != sizeof(prgm_header))
			goto ret;
		if (prgm_header.p_type == PT_LOAD) {
			if (pre_load_segment(mm, inode, &prgm_header) == -1)
				goto ret;
		}
	}
	res = elf_header.e_entry;

ret:
	iunlock(inode);
	return res;
}