/* kernel.ld */

OUTPUT_ARCH(riscv) /* specific the ISA of output files which run in RV */
ENTRY(kernel_entry)  /* specific entry of OS defined in asm file */

BASE_ADDRESS = 0x80200000; /* OS start addr */


SECTIONS
{
	/* Load the kernel at this address */
	. = BASE_ADDRESS;
	PROVIDE(stext = .);
	.text : {
		*(.text.kernel_entry)
		*(.text .text.*)
	}
	PROVIDE(etext = .);

	/*read only data */
	.rodata : {
		. = ALIGN(16);
		*(.srodata .srodata.*) /* do not need to distinguish this from .rodata */
		. = ALIGN(16);
		*(.rodata .rodata.*)
	}

	. = ALIGN(0x1000);	/* align the addr to next page */
	PROVIDE(sdata = .);
	.data : {
		. = ALIGN(16);
		*(.sdata .sdata.*) /* do not need to distinguish this from .data */
		. = ALIGN(16);
		*(.data .data.*)
	}
	PROVIDE(edata = .);

	PROVIDE(sbss = .);
	.bss : {
		. = ALIGN(16);
		*(.sbss .sbss.*) /* do not need to distinguish this from .bss */
		. = ALIGN(16);
		*(.bss .bss.*)
	}
	PROVIDE(ebss = .);

	/* /DISCARD/ means omitting and it will not be added in output file */
	/DISCARD/ : {
		*(.eh_frame .note.GNU-stack)
	}
}