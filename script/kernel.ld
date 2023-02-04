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
		*(.text .stub .text.* .gnu.linkonce.t.*)
	}
	PROVIDE(etext = .);

	/*read only data */
	.rodata : {
		*(.rodata .rodata.* .gnu.linkonce.r.*)
	}

	. = ALIGN(0x1000);	/* align the addr to next page */
	PROVIDE(sdata = .);
	.data : {
		*(.data)
		*(.data.*)
	}
	PROVIDE(edata = .);

	/* small data section, generally is char, short etc. */
	.sdata : {
		*(.sdata)
		*(.sdata.*)
	}

	/* initial bss seg with 0 */
	PROVIDE(sbss = .);
	.bss : {
		*(.bss)
		*(.bss.*)
		*(.sbss*)
	}
	PROVIDE(ebss = .);

	/* /DISCARD/ means omitting and it will not be added in output file */
	/DISCARD/ : {
		*(.eh_frame .note.GNU-stack)
	}
}