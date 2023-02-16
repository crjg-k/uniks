
LOG ?= debug

# basic tools
CROSS_PREFIX = riscv64-unknown-elf-
CC = ${CROSS_PREFIX}gcc
AS = $(TOOLPREFIX)as
LD = $(TOOLPREFIX)ld
GDB = ${CROSS_PREFIX}gdb
OBJCOPY = ${CROSS_PREFIX}objcopy
OBJDUMP = ${CROSS_PREFIX}objdump

SRC_KERNEL_DIR = kernel
INC_DIR = \
	./include \
	./${SRC_KERNEL_DIR}
SRC_FILES = \
	./${SRC_KERNEL_DIR}/init/* \
	./${SRC_KERNEL_DIR}/driver/* \
	./${SRC_KERNEL_DIR}/platform/* \
	./${SRC_KERNEL_DIR}/trap/* \
	./libs/*
LINKER = ./script/kernel.ld
TARGET = ./bin/kernel.elf
OSNAME = kos.bin
OSBIN = ./bin/${OSNAME}

CFLAGS = \
	-mcmodel=medany \
	-nostdlib \
	-fno-builtin \
	-ffreestanding \
	-Wall \
	-Werror \
	-fno-stack-protector \
	-fno-omit-frame-pointer

CFLAGS += -T ${LINKER}
CFLAGS += $(foreach dir, ${INC_DIR}, -I${dir})
ifeq ($(LOG), error)
	CFLAGS += -D LOG_LEVEL_ERROR
else ifeq ($(LOG), warn)
	CFLAGS += -D LOG_LEVEL_WARN
else ifeq ($(LOG), info)
	CFLAGS += -D LOG_LEVEL_INFO
else ifeq ($(LOG), debug)
	CFLAGS += -D LOG_LEVEL_DEBUG
	CFLAGS += -g
else ifeq ($(LOG), trace)
	CFLAGS += -D LOG_LEVEL_TRACE
endif

.PHONY: run
run: build qemu

.PHONY: build
build: 
	${CC} ${SRC_FILES} ${CFLAGS} -o ${TARGET}
	${OBJCOPY} ${TARGET} --strip-all -O binary ${OSBIN}


# qemu option
CPUS = 1
BOOTLOADER = default
QEMU = qemu-system-riscv64
QFLAGS = \
	-nographic \
	-smp ${CPUS} \
	-machine virt \
	-bios ${BOOTLOADER} \
	-kernel ${OSBIN}
QEMUGDB = \
	-gdb tcp::1234


.PHONY: qemu
qemu: build
	${QEMU} ${QFLAGS}

.PHONY: debug
debug: build
	${QEMU} ${QFLAGS} -S ${QEMUGDB}&
	gdb-multiarch bin/kernel.elf -q -x script/.gdbinit

.PHONY: hint
hint:
	bear -- make build
	mv compile_commands.json .vscode/

.PHONY: clean
clean:
	rm -r bin/*