
# basic tools
SHELL = /bin/bash
CROSS_PREFIX = riscv64-unknown-elf-
CC = ${CROSS_PREFIX}gcc
AS = ${CROSS_PREFIX}as
LD = ${CROSS_PREFIX}ld
GDB = gdb-multiarch
OBJCOPY = ${CROSS_PREFIX}objcopy
OBJDUMP = ${CROSS_PREFIX}objdump


INC_DIR = ./include
LIBS_DIR= ./libs
BIN_DIR = ./bin
ABSM_DIR = ./absm

LINKER = ./user.ld

CFLAGS = \
	-c \
	-g3 \
	-march=rv64g \
	-nostdlib \
	-fno-builtin \
	-ffreestanding

CFLAGS += $(foreach dir, ${INC_DIR}, -I${dir})

LDFLAGS = \
	-T ${LINKER}

SRCS := $(wildcard ./src/*.c)
OBJS := $(patsubst ./src/%.c, %, $(SRCS))

.PHONY: init
init:
	mkdir ${BIN_DIR}
	mkdir ${ABSM_DIR}

.PHONY: build
build: $(OBJS)

%: src/%.c ${LIBS_DIR}/ulib.c ${LIBS_DIR}/usyscall.c
	${CC} $^ ${CFLAGS}
	${LD} ${LDFLAGS} ulib.o $@.o usyscall.o -o ${BIN_DIR}/$@.o
	${OBJDUMP} -S ${BIN_DIR}/$@.o > ${ABSM_DIR}/$@.asm
	${OBJDUMP} -s ${BIN_DIR}/$@.o > ${ABSM_DIR}/$@.bsm
	${OBJCOPY} ${BIN_DIR}/$@.o --strip-all -O binary ${BIN_DIR}/$@
	od -t x ${BIN_DIR}/$@
	rm bin/$@
	rm *.o

.PHONY: initcode
initcode: src/initcode.S
	${CC} $< ${CFLAGS} -o ${BIN_DIR}/initcode.o
	${LD} ${LDFLAGS} ${BIN_DIR}/initcode.o -o ${BIN_DIR}/initcode.tmp
	${OBJDUMP} -S ${BIN_DIR}/initcode.tmp > ${ABSM_DIR}/initcode.asm
	${OBJDUMP} -s ${BIN_DIR}/initcode.tmp > ${ABSM_DIR}/initcode.bsm
	${OBJCOPY} ${BIN_DIR}/initcode.tmp --strip-all -O binary ${BIN_DIR}/initcode
	od -t x ${BIN_DIR}/initcode
	mv ${BIN_DIR}/initcode ${BIN_DIR}/initcode.o
	rm ${BIN_DIR}/initcode.tmp

.PHONY: clean
clean: 
	rm ${BIN_DIR}/*
	rm ${ABSM_DIR}/*