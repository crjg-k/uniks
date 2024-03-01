
# LOG ?= trace
# QEMULOG ?= trace


# OS'S PARAMETER
CPUS = 2
DISKSIZE = 256
BLKSIZE = 4096


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
	./${SRC_KERNEL_DIR} \
	./ \
	/usr/include	# Linux C language enviroment include path: /usr/include/
SRC_FILES = \
	./${SRC_KERNEL_DIR}/init/* \
	./${SRC_KERNEL_DIR}/device/* \
	./${SRC_KERNEL_DIR}/file/* \
	./${SRC_KERNEL_DIR}/trap/* \
	./${SRC_KERNEL_DIR}/loader/* \
	./${SRC_KERNEL_DIR}/mm/* \
	./${SRC_KERNEL_DIR}/sys/* \
	./${SRC_KERNEL_DIR}/sync/* \
	./${SRC_KERNEL_DIR}/process/* \
	./platform/* \
	./fs/* \
	./libs/*
LINKER = ./script/kernel.ld
TARGET = ./bin/kernel.elf
OSNAME = uniks.bin
OSBIN = ./bin/${OSNAME}
DISKIMG = disk.img

CFLAGS = \
	-mcmodel=medany \
	-nostdlib \
	-fno-builtin \
	-ffreestanding \
	-Wall \
	-Werror \
	-Wno-error=unused-function \
	-march=rv64g \
	-fno-stack-protector \
	-fno-omit-frame-pointer \
	-fno-stack-protector \
	-DMAXNUM_HARTID=${CPUS} \
	-DDISKSIZE=${DISKSIZE} \
	-DBLKSIZE=${BLKSIZE}

CFLAGS += -T ${LINKER}
CFLAGS += -g3
CFLAGS += $(foreach dir, ${INC_DIR}, -I${dir})
ifeq ($(LOG), error)
	CFLAGS += -D LOG_LEVEL_ERROR
	LOG_LEVEL ?= LOG_LEVEL_ERROR
else ifeq ($(LOG), warn)
	CFLAGS += -D LOG_LEVEL_WARN
	LOG_LEVEL ?= LOG_LEVEL_WARN
else ifeq ($(LOG), info)
	CFLAGS += -D LOG_LEVEL_INFO
	LOG_LEVEL ?= LOG_LEVEL_INFO
else ifeq ($(LOG), debug)
	CFLAGS += -D LOG_LEVEL_DEBUG
	LOG_LEVEL ?= LOG_LEVEL_DEBUG
else ifeq ($(LOG), trace)
	CFLAGS += -D LOG_LEVEL_TRACE
	LOG_LEVEL ?= LOG_LEVEL_TRACE
endif

.PHONY: run
run: clean qemu dump

.PHONY: build
build:
	$(shell if [ ! -e bin ]; then mkdir bin; fi)
	${CC} ${SRC_FILES} ${CFLAGS} -o ${TARGET}
	${OBJCOPY} ${TARGET} --strip-all -O binary ${OSBIN}


# qemu option
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
	-drive file=${DISKIMG},if=none,format=raw,id=x0 \
	-device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
QEMUGDB = \
	-gdb tcp::8964

ifeq ($(QEMULOG), trace)
	QFLAGS += -D ${QEMULOGPATH} -d exec
endif


.PHONY: qemu
qemu: build
	${QEMU} ${QFLAGS}

.PHONY: debug
debug: build
	${QEMU} ${QFLAGS} -S ${QEMUGDB}&
	${GDB} bin/kernel.elf -q -x script/.gdbinit

.PHONY: debug-back
debug-back: build
	${QEMU} ${QFLAGS} -S ${QEMUGDB}

.PHONY: debug-front
debug-front:
	${GDB} bin/kernel.elf -q -x script/.gdbinit

.PHONY: debug-vscode
debug-vscode: build
	${QEMU} ${QFLAGS} -S ${QEMUGDB}

.PHONY: dump
dump: build
	${OBJDUMP} -S ${TARGET} > ./bin/kerneldump.asm
	# vim ./bin/kerneldump.txt

.PHONY: ${DISKIMG}
${DISKIMG}:
	$(shell if [ -e ${DISKIMG} ]; then rm ${DISKIMG}; fi)
	touch ${DISKIMG}
	fallocate ${DISKIMG} -l ${DISKSIZE}M
	echo y | mkfs.ext2 -b ${BLKSIZE} -O ^has_journal -r 0 ${DISKIMG}

# below command acquire Linux's root user
.PHONY: fs
fs:
	mkdir uniksfs
	sudo mount disk.img uniksfs
	cp ./user/src/rwblk.c ./uniksfs
	umount uniksfs
	rm -r uniksfs

PROPERTIES = .vscode/c_cpp_properties.json
.PHONY: vscode
vscode:
	@echo Generating ${PROPERTIES}...
	@echo "{" > ${PROPERTIES}
	@echo "	\"configurations\": [" >> ${PROPERTIES}
	@echo "		{" >> ${PROPERTIES}
	@echo "			\"name\": \"Linux\"," >> ${PROPERTIES}
	@echo "			\"includePath\": [" >> ${PROPERTIES}
	@echo "				\"\$$$ {workspaceFolder}/**\"" >> ${PROPERTIES}
	@echo "			]," >> ${PROPERTIES}
	@echo "			\"defines\": [" >> ${PROPERTIES}
	@echo "				\"MAXNUM_HARTID = ${CPUS}\"," >> ${PROPERTIES}
	@echo "				\"DISKSIZE = ${DISKSIZE}\"," >> ${PROPERTIES}
	@echo "				\"BLKSIZE = ${BLKSIZE}\"," >> ${PROPERTIES}
	@echo "				\"${LOG_LEVEL}\"," >> ${PROPERTIES}
	@echo "			]," >> ${PROPERTIES}
	@echo "			\"cStandard\": \"c11\"," >> ${PROPERTIES}
	@echo "			\"intelliSenseMode\": \"linux-gcc-x64\"," >> ${PROPERTIES}
	@echo "			\"configurationProvider\": \"ms-vscode.makefile-tools\"" >> ${PROPERTIES}
	@echo "		}" >> ${PROPERTIES}
	@echo "	]," >> ${PROPERTIES}
	@echo "	\"version\": 4" >> ${PROPERTIES}
	@echo "}" >> ${PROPERTIES}
	@echo Done.

.PHONY: clean
clean:
	$(shell if [ -d bin -a "`ls -A bin`" != "" ]; then rm -r bin/*; fi)