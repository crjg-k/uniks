
DEBUG ?= gdb

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
INITCODE = initcode

LINKER = ./user.ld

CFLAGS = \
	-c \
	-static \
	-march=rv64g \
	-nostdlib \
	-fno-builtin

CFLAGS += $(foreach dir, ${INC_DIR}, -I${dir})

ifeq ($(DEBUG), gdb)
	CFLAGS += -g3
endif

LDFLAGS = \
	-T ${LINKER}

LIBS := $(patsubst %.c, %.o, $(notdir $(wildcard $(LIBS_DIR)/*.c)))
OBJS := $(patsubst ./src/%.c, %, $(wildcard ./src/*.c))

.PHONY: build
build: init lib $(OBJS)
	rm *.o

.PHONY: init
init:
	$(shell if [ ! -e ${BIN_DIR} ]; then mkdir ${BIN_DIR}; fi)
	$(shell if [ ! -e ${ABSM_DIR} ]; then mkdir ${ABSM_DIR}; fi)

lib: ${LIBS_DIR}/*
	${CC} $^ ${CFLAGS}

%: src/%.c
	${CC} $< ${CFLAGS}
	${LD} ${LDFLAGS} ${LIBS} $@.o -o ${BIN_DIR}/$@
	${OBJDUMP} -S ${BIN_DIR}/$@ > ${ABSM_DIR}/$@.asm
	${OBJDUMP} -s ${BIN_DIR}/$@ > ${ABSM_DIR}/$@.bsm

.PHONY: ${INITCODE}
${INITCODE}: init src/${INITCODE}.S
	${CC} src/${INITCODE}.S ${CFLAGS}
	${LD} ${LDFLAGS} ${INITCODE}.o -o ${BIN_DIR}/${INITCODE}
	${OBJDUMP} -S ${BIN_DIR}/${INITCODE} > ${ABSM_DIR}/${INITCODE}.asm
	${OBJDUMP} -s ${BIN_DIR}/${INITCODE} > ${ABSM_DIR}/${INITCODE}.bsm
	${OBJCOPY} ${BIN_DIR}/${INITCODE} --strip-all -O binary ${BIN_DIR}/${INITCODE}
	hexdump -ve '4/4 "0x%08x, " "\n"' ${BIN_DIR}/${INITCODE}
	rm *.o

.PHONY: clean
clean:
	rm ${BIN_DIR}/*
	rm ${ABSM_DIR}/*
