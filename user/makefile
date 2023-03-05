
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

LINKER = ./user.ld

CFLAGS = \
	-c \
	-g3 \
	-march=rv64g \
	-I ${INC_DIR}

LDFLAGS = \
	-T ${LINKER}


.PHONY: build
build: initcode.S
	${CC} $< ${CFLAGS} -o ${TAG_DIR}/initcode.o
	${LD} ${LDFLAGS} ${TAG_DIR}/initcode.o -o ${TAG_DIR}/initcode
	${OBJDUMP} -d ${TAG_DIR}/initcode > initcode.asm
	${OBJDUMP} -s ${TAG_DIR}/initcode > initcode.bsm

.PHONY: clean
clean: 
	rm ${TAG_DIR}/*