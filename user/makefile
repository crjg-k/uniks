
# basic tools
CROSS_PREFIX = riscv64-unknown-elf-
CC = ${CROSS_PREFIX}gcc
AS = ${CROSS_PREFIX}as
LD = ${CROSS_PREFIX}ld
GDB = gdb-multiarch
OBJCOPY = ${CROSS_PREFIX}objcopy
OBJDUMP = ${CROSS_PREFIX}objdump


INC_DIR = ./include
TAG_DIR = ./bin
ABSM_DIR = ./absm

LINKER = ./user.ld

CFLAGS = \
	-c \
	-g3 \
	-march=rv64g

CFLAGS += $(foreach dir, ${INC_DIR}, -I${dir})

LDFLAGS = \
	-T ${LINKER}


.PHONY: build
build: src/forktest.c libs/ulib.c libs/usyscall.c
	${CC} $^ ${CFLAGS}
	${LD} ${LDFLAGS} ulib.o forktest.o usyscall.o -o ${TAG_DIR}/forktest.o
	${OBJDUMP} -d ${TAG_DIR}/forktest.o > ${ABSM_DIR}/forktest.asm
	${OBJDUMP} -s ${TAG_DIR}/forktest.o > ${ABSM_DIR}/forktest.bsm
	${OBJCOPY} ${TAG_DIR}/forktest.o --strip-all -O binary ${TAG_DIR}/forktest
	od -t x ${TAG_DIR}/forktest
	rm bin/forktest
	rm *.o

.PHONY: initasm
initasm: src/initcode.S
	${CC} $< ${CFLAGS} -o ${TAG_DIR}/initcode.o
	${LD} ${LDFLAGS} ${TAG_DIR}/initcode.o -o ${TAG_DIR}/initcode.out
	${OBJDUMP} -d ${TAG_DIR}/initcode.out > ${ABSM_DIR}/initcode.asm
	${OBJDUMP} -s ${TAG_DIR}/initcode.out > ${ABSM_DIR}/initcode.bsm
	${OBJCOPY} ${TAG_DIR}/initcode.out --strip-all -O binary ${TAG_DIR}/initcode
	od -t x ${TAG_DIR}/initcode
	rm ${TAG_DIR}/initcode.o

.PHONY: clean
clean: 
	rm ${TAG_DIR}/*
	rm ${ABSM_DIR}/*