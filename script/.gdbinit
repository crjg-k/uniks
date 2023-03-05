set confirm off
set architecture riscv:rv64
target remote 127.0.0.1:1234
symbol-file bin/kernel.elf
set disassemble-next-line on
set riscv use-compressed-breakpoints yes

break yield
break interrupt_handler
break scheduler
c
layout split
