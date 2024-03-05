set confirm off
set architecture riscv:rv64
target remote 127.0.0.1:2952
symbol-file bin/kernel.elf
set disassemble-next-line on
set riscv use-compressed-breakpoints yes

break usertrapret
break *0x0000003ffffff540
c
layout split
