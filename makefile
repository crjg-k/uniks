
# LOG ?= debug
# QEMULOG ?= trace

# basic tools
SHELL = /bin/bash
CROSS_PREFIX = riscv64-unknown-elf-
CC = ${CROSS_PREFIX}gcc
AS = ${CROSS_PREFIX}as
LD = ${CROSS_PREFIX}ld
GDB = gdb-multiarch
OBJCOPY = ${CROSS_PREFIX}objcopy
OBJDUMP = ${CROSS_PREFIX}objdump

SRC_KERNEL_DIR = kernel
INC_DIR = \
	./include \
	./${SRC_KERNEL_DIR}
SRC_FILES = \
	./${SRC_KERNEL_DIR}/init/* \
	./${SRC_KERNEL_DIR}/device/* \
	./${SRC_KERNEL_DIR}/platform/* \
	./${SRC_KERNEL_DIR}/trap/* \
	./${SRC_KERNEL_DIR}/mm/* \
	./${SRC_KERNEL_DIR}/sys/* \
	./${SRC_KERNEL_DIR}/fs/* \
	./${SRC_KERNEL_DIR}/sync/* \
	./${SRC_KERNEL_DIR}/process/* \
	./libs/*
LINKER = ./script/kernel.ld
TARGET = ./bin/kernel.elf
OSNAME = uniks.bin
OSBIN = ./bin/${OSNAME}

CFLAGS = \
	-mcmodel=medany \
	-nostdlib \
	-fno-builtin \
	-ffreestanding \
	-Wall \
	-Werror \
	-march=rv64g \
	-fno-stack-protector \
	-fno-omit-frame-pointer

CFLAGS += -T ${LINKER}
CFLAGS += -g3
CFLAGS += $(foreach dir, ${INC_DIR}, -I${dir})
ifeq ($(LOG), error)
	CFLAGS += -D LOG_LEVEL_ERROR
else ifeq ($(LOG), warn)
	CFLAGS += -D LOG_LEVEL_WARN
else ifeq ($(LOG), info)
	CFLAGS += -D LOG_LEVEL_INFO
else ifeq ($(LOG), debug)
	CFLAGS += -D LOG_LEVEL_DEBUG
else ifeq ($(LOG), trace)
	CFLAGS += -D LOG_LEVEL_TRACE
endif

.PHONY: run
run: clean build qemu dump

.PHONY: build
build:
	$(shell if [ ! -e bin ]; then mkdir bin; fi)
	${CC} ${SRC_FILES} ${CFLAGS} -o ${TARGET}
	${OBJCOPY} ${TARGET} --strip-all -O binary ${OSBIN}

.PHONY: user
user:
	cd ./user ; make


# qemu option
CPUS = 1
BOOTLOADER = default
QEMU = qemu-system-riscv64
QEMULOGPATH = ./qemu-log
QFLAGS = \
	-nographic \
	-smp ${CPUS} \
	-m 128M \
	-machine virt \
	-bios ${BOOTLOADER} \
	-kernel ${OSBIN} \
	-global virtio-mmio.force-legacy=false \
	-drive file=fs.img,if=none,format=raw,id=x0 \
	-device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
QEMUGDB = \
	-gdb tcp::1234

ifeq ($(LOG), trace)
	QFLAGS += -D ${QEMULOGPATH} -d exec
endif


.PHONY: qemu
qemu: build
	${QEMU} ${QFLAGS}

.PHONY: debug
debug: build
	${QEMU} ${QFLAGS} -S ${QEMUGDB}&
	${GDB} bin/kernel.elf -q -x script/.gdbinit

.PHONY: debug-vscode
debug-vscode: build
	${QEMU} ${QFLAGS} -S ${QEMUGDB}

.PHONY: dump
dump: build
	${OBJDUMP} -d ${TARGET} > ./bin/kerneldump.asm
	# vim ./bin/kerneldump.txt

.PHONY: hint
hint:
	bear -- make build
	mv compile_commands.json .vscode/

.PHONY: clean
clean:
	$(shell if [ -d bin -a "`ls -A bin`" != "" ]; then rm -r bin/*; fi)