#ifndef __KERNEL_LOADER_ELF_H__
#define __KERNEL_LOADER_ELF_H__

/**
 * @brief most of this file's content comes from <usr/include/elf.h>.
 *
 * The ELF standard specification:
 * https://uclibc.org/docs/elf-64-gen.pdf
 */

#include <uniks/defs.h>

#if 0
	#include <elf.h>
#endif
#define ELFMAGIC \
	("\x7f" \
	 "ELF")

typedef uint64_t Elf64_Addr_t;
typedef uint64_t Elf64_Off_t;
typedef uint16_t Elf64_Half_t;
typedef uint32_t Elf64_Word_t;
typedef int32_t Elf64_Sword_t;
typedef uint64_t Elf64_Xword_t;
typedef int64_t Elf64_Sxword_t;


/* === below is ELF ident === */

// ELF file verion
enum EI_version_t {
	EV_NONE = 0,	  // unavailable version
	EV_CURRENT = 1,	  // current version
};

// ELF class(distinguish 32bit/64bit)
enum Ei_class_t {
	ELFCLASSNONE = 0,   // invalid class
	ELFCLASS32 = 1,	    // 32-bit objects
	ELFCLASS64 = 2,	    // 64-bit objects
};

enum Ei_data_t {
	ELFDATANONE = 0,   // invalid data encoding
	ELFDATA2LSB = 1,   // LSB
	ELFDATA2MSB = 2,   // MSB
};

enum Ei_osabi_t {
	ELFOSABI_SYSV = 0,	     // System V ABI
	ELFOSABI_HPUX = 1,	     // HP-UX operating system
	ELFOSABI_STANDALONE = 255,   // Standalone (embedded) application
};

enum Elf_Ident_index_t {
	EI_MAG0 = 0,	     // 0x7F
	EI_MAG1 = 1,	     // 'E'
	EI_MAG2 = 2,	     // 'L'
	EI_MAG3 = 3,	     // 'F'
	EI_CLASS = 4,	     // Architecture (32/64)
	EI_DATA = 5,	     // Byte Order
	EI_VERSION = 6,	     // ELF Version
	EI_OSABI = 7,	     // OS Specific
	EI_ABIVERSION = 8,   // OS Specific
	EI_PAD = 9	     // Padding
};

// ELF file label
struct Elf_Ident_t {
	char ei_magic[4];	 // magic number: 0x7F, E, L, F
	uint8_t ei_class;	 // file class, 1=>32bit, 2=>64bit
	uint8_t ei_data;	 // mark endian, 1=>little endian, 2=>big endian
	uint8_t ei_version;	 // must be 1 as same as e_version
	uint8_t ei_osabi;	 // OS/ABI identification
	uint8_t ei_abiversion;	 // ABI version
	uint8_t ei_pad;		 // start of padding bytes
	uint8_t ei_nident[16 - 10];   // align to 16 bytes
} __packed;


/* === below is ELF header === */

// ELF file type
enum Elf_Type_t {
	ET_NONE = 0,	      // no file type
	ET_REL = 1,	      // relocatable
	ET_EXEC = 2,	      // executable
	ET_DYN = 3,	      // dynamic linked library
	ET_CORE = 4,	      // core file, seize seat
	ET_LOOS = 0xfe00,     // reserved for environment-specific use[start]
	ET_HIOS = 0xfeff,     // reserved for environment-specific use[end]
	ET_LOPROC = 0xff00,   // reserved for processor-specific use[start]
	ET_HIPROC = 0xffff,   // reserved for processor-specific use[end]
};

// ELF platform(ISA/CPU) type
enum Elf_Machine_t {
	EM_NONE = 0,
	EM_RISCV = 243,	  // RISCV
};

#define EI_NIDENT (16)
// ELF file header
struct Elf64_Ehdr_t {
	uint8_t e_ident[EI_NIDENT]; /* Magic number and other info */
	Elf64_Half_t e_type;	    /* Object file type */
	Elf64_Half_t e_machine;	    /* Architecture */
	Elf64_Word_t e_version;	    /* Object file version */
	Elf64_Addr_t e_entry;	    /* Entry point virtual address */
	Elf64_Off_t e_phoff;	    /* Program header table file offset */
	Elf64_Off_t e_shoff;	    /* Section header table file offset */
	Elf64_Word_t e_flags;	    /* Processor-specific flags */
	Elf64_Half_t e_ehsize;	    /* ELF header size in bytes */
	Elf64_Half_t e_phentsize;   /* Program header table entry size */
	Elf64_Half_t e_phnum;	    /* Program header table entry count */
	Elf64_Half_t e_shentsize;   /* Section header table entry size */
	Elf64_Half_t e_shnum;	    /* Section header table entry count */
	Elf64_Half_t e_shstrndx;    /* Section header string table index */
} __packed;


/* === below is segment === */

// program header
struct Elf64_Phdr_t {
	Elf64_Word_t p_type;	/* Segment type */
	Elf64_Word_t p_flags;	/* Segment flags */
	Elf64_Off_t p_offset;	/* Segment file offset */
	Elf64_Addr_t p_vaddr;	/* Segment virtual address */
	Elf64_Addr_t p_paddr;	/* Segment physical address */
	Elf64_Xword_t p_filesz; /* Segment size in file */
	Elf64_Xword_t p_memsz;	/* Segment size in memory */
	Elf64_Xword_t p_align;	/* Segment alignment */
} __packed;

#define a sizeof(struct Elf64_Phdr_t)

// segment type
enum segmentype_t {
	PT_NULL = 0,		// Unused entry
	PT_LOAD = 1,		// Loadable segment
	PT_DYNAMIC = 2,		// Dynamic linking tables
	PT_INTERP = 3,		// Program interpreter path name
	PT_NOTE = 4,		// Note sections
	PT_SHLIB = 5,		// Reserved
	PT_PHDR = 6,		// Program header table
	PT_LOOS = 0x60000000,	// reserved for environment-specific use[start]
	PT_HIOS = 0xfffffff,	// reserved for environment-specific use[end]
	PT_LOPROC = 0x70000000,	  // reserved for processor-specific use[start]
	PT_HIPROC = 0x7fffffff,	  // reserved for processor-specific use[end]
};

enum SegmentFlag_t {
	PF_X = 0x1,		    // executable
	PF_W = 0x2,		    // writable
	PF_R = 0x4,		    // readable
	PF_MASKOS = 0x00ff0000,	    // reserved for environment-specific use
	PF_MASKPROC = 0xff000000,   // reserved for processor-specific use
};


/* === below is section === */

enum SectionType_t {
	SHT_NULL = 0,		  // Section header table entry unused
	SHT_PROGBITS = 1,	  // Program data
	SHT_SYMTAB = 2,		  // Symbol table
	SHT_STRTAB = 3,		  // String table
	SHT_RELA = 4,		  // Relocation entries with addends
	SHT_HASH = 5,		  // Symbol hash table
	SHT_DYNAMIC = 6,	  // Dynamic linking information
	SHT_NOTE = 7,		  // Contains note information
	SHT_NOBITS = 8,		  // Program space with no data (bss)
	SHT_REL = 9,		  // Relocation entries, no addends
	SHT_SHLIB = 10,		  // Reserved
	SHT_DYNSYM = 11,	  // Dynamic linker symbol table
	SHT_INIT_ARRAY = 14,	  // Array of constructors
	SHT_FINI_ARRAY = 15,	  // Array of destructors
	SHT_PREINIT_ARRAY = 16,	  // Array of pre-constructors
	SHT_GROUP = 17,		  // Section group
	SHT_SYMTAB_SHNDX = 18,	  // Extended section indices
	SHT_NUM = 19,		  // Number of defined types.
	SHT_LOOS = 0x60000000,	 // reserved for environment-specific use[start]
	SHT_HIOS = 0x6fffffff,	 // reserved for environment-specific use[end]
	SHT_LOPROC = 0x70000000,   // reserved for processor-specific use[start]
	SHT_HIPROC = 0x7fffffff,   // reserved for processor-specific use[end]
};

enum SectionFlag_t {
	SHF_WRITE = 0x1,   // section contains writable data
	SHF_ALLOC = 0x2,   // section is allocated in memory image of program
	SHF_EXECINSTR = 0x4,	     // section contains executable instructions
	SHF_MASKOS = 0x0f000000,     // reserved for environment-specific use
	SHF_MASKPROC = 0xf0000000,   // reserved for processor-specific use
};

struct Elf64_Shdr_t {
	Elf64_Word_t sh_name;	    /* Section name (string tbl index) */
	Elf64_Word_t sh_type;	    /* Section type */
	Elf64_Xword_t sh_flags;	    /* Section flags */
	Elf64_Addr_t sh_addr;	    /* Section virtual addr at execution */
	Elf64_Off_t sh_offset;	    /* Section file offset */
	Elf64_Xword_t sh_size;	    /* Section size in bytes */
	Elf64_Word_t sh_link;	    /* Link to another section */
	Elf64_Word_t sh_info;	    /* Additional section information */
	Elf64_Xword_t sh_addralign; /* Section alignment */
	Elf64_Xword_t sh_entsize;   /* Entry size if section holds table */
} __packed;

#endif