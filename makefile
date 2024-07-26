
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
run: clean qemu

.PHONY: build
build:
	$(shell if [ ! -e bin ]; then mkdir bin; fi)
	${CC} ${SRC_FILES} ${CFLAGS} -o ${TARGET}
	${OBJCOPY} ${TARGET} --strip-all -O binary ${OSBIN}
	${OBJDUMP} -S ${TARGET} > ./bin/kerneldump.asm


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
QEMUGDB1 = \
	-gdb tcp::2952
QEMUGDB2 = \
	-gdb tcp::8964


ifeq ($(QEMULOG), trace)
	QFLAGS += -D ${QEMULOGPATH} -d exec
endif


.PHONY: qemu
qemu: build
	${QEMU} ${QFLAGS}

.PHONY: debug
debug: build
	${QEMU} ${QFLAGS} -S ${QEMUGDB1}&
	${GDB} bin/kernel.elf -q -x script/.gdbinit

.PHONY: debug-back
debug-back: build
	${QEMU} ${QFLAGS} -S ${QEMUGDB1}

.PHONY: debug-front
debug-front:
	${GDB} bin/kernel.elf -q -x script/.gdbinit

.PHONY: debug-vscode
debug-vscode: build
	${QEMU} ${QFLAGS} -S ${QEMUGDB2}

.PHONY: ${DISKIMG}
${DISKIMG}:
	$(shell if [ -e ${DISKIMG} ]; then rm ${DISKIMG}; fi)
	touch ${DISKIMG}
	fallocate ${DISKIMG} -l ${DISKSIZE}M
	echo y | mkfs.ext2 -b ${BLKSIZE} -O ^has_journal -r 0 ${DISKIMG}

# below command acquire Linux's root user
.PHONY: fs
fs:
	make ${DISKIMG}
	$(shell if [ ! -e uniksfs ]; then mkdir uniksfs; fi)
	sudo mount ${DISKIMG} uniksfs
	mkdir ./uniksfs/dev ./uniksfs/bin  ./uniksfs/root
	mknod ./uniksfs/dev/tty0 c 00 36
	mknod ./uniksfs/dev/vda0 b 00 01
	mknod ./uniksfs/dev/null c 00 00
	cp README.md ./uniksfs/root
	cp LICENSE ./uniksfs/root
	make user
	find ./user/bin/ -type f -not -name 'initcode' -exec cp {} ./uniksfs/bin/ \;
	umount uniksfs

.PHONY: m
m:
	$(shell if [ ! -e uniksfs ]; then mkdir uniksfs; fi)
	sudo mount ${DISKIMG} uniksfs

.PHONY: um
um:
	sudo umount uniksfs

.PHONY: user
user:
	cd ./user && make


PROPERTIES = .vscode/c_cpp_properties.json
LAUNCH = .vscode/launch.json
TASKS = .vscode/tasks.json
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
	@echo Generating ${LAUNCH}...
	@echo '{' > ${LAUNCH}
	@echo '	"version": "0.1.0",' >> ${LAUNCH}
	@echo '	"configurations": [' >> ${LAUNCH}
	@echo '		{' >> ${LAUNCH}
	@echo '			"name": "uniks-debug",' >> ${LAUNCH}
	@echo '			"type": "cppdbg",' >> ${LAUNCH}
	@echo '			"request": "launch",' >> ${LAUNCH}
	@echo '			"program": "$${workspaceFolder}/bin/kernel.elf",' >> ${LAUNCH}
	@echo '			"cwd": "$${workspaceFolder}",' >> ${LAUNCH}
	@echo '			"stopAtEntry": true,' >> ${LAUNCH}
	@echo '			"miDebuggerServerAddress": "localhost:8964",' >> ${LAUNCH}
	@echo '			"miDebuggerPath": "gdb-multiarch",' >> ${LAUNCH}
	@echo '			"MIMode": "gdb",' >> ${LAUNCH}
	@echo '			"preLaunchTask": "make-debug",' >> ${LAUNCH}
	@echo '			"setupCommands": [' >> ${LAUNCH}
	@echo '				{' >> ${LAUNCH}
	@echo '					"description": "Enable pretty-printing for gdb",' >> ${LAUNCH}
	@echo '					"text": "-enable-pretty-printing",' >> ${LAUNCH}
	@echo '					"ignoreFailures": true' >> ${LAUNCH}
	@echo '				}' >> ${LAUNCH}
	@echo '			]' >> ${LAUNCH}
	@echo '		}' >> ${LAUNCH}
	@echo '	]' >> ${LAUNCH}
	@echo '}' >> ${LAUNCH}
	@echo Done.
	@echo Generating ${TASKS}...
	@echo '{' > ${TASKS}
	@echo '	"tasks": [' >> ${TASKS}
	@echo '		{' >> ${TASKS}
	@echo '			"label": "make-debug",' >> ${TASKS}
	@echo '			"command": "make",' >> ${TASKS}
	@echo '			"args": ["debug-vscode"],' >> ${TASKS}
	@echo '			"isBackground": true,' >> ${TASKS}
	@echo '			"problemMatcher": "$$gcc"' >> ${TASKS}
	@echo '		}' >> ${TASKS}
	@echo '	],' >> ${TASKS}
	@echo '	"version": "2.0.0"' >> ${TASKS}
	@echo '}' >> ${TASKS}
	@echo Done.

.PHONY: clean
clean:
	$(shell if [ -d bin -a "`ls -A bin`" != "" ]; then rm -rf bin/*; fi)