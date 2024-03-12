/* user.ld */

OUTPUT_ARCH(riscv)
ENTRY(_start)


SECTIONS
{
	. = 0x1000;

	.text : {
		*(.text .text.*)
	}

	.rodata : {
		. = ALIGN(16);
		*(.srodata .srodata.*)	/* do not need to distinguish this from .rodata */
		. = ALIGN(16);
		*(.rodata .rodata.*)
	}

	/* 
	 * This will result in a large gap between two parts,
	 * so that ld will divide them into two different segments 
	*/
	. = ALIGN(0x1000);

	.data : {
		. = ALIGN(16);
		*(.sdata .sdata.*)	/* do not need to distinguish this from .data */
		. = ALIGN(16);
		*(.data .data.*)
	}

	.bss : {
		. = ALIGN(16);
		*(.sbss .sbss.*)	/* do not need to distinguish this from .bss */
		. = ALIGN(16);
		*(.bss .bss.*)
	}

	PROVIDE(end = .);
}
