CXX ?= g++
AS   = nasm

ARCH ?= x86_64

MKDIR = mkdir -p

BIN  := /tmp/aex2/kernel/
BOOT := boot/
DEP_DEST := $(BIN)dep/
OBJ_DEST := $(BIN)obj/

CXXFILES  := $(shell find . -type f -name '*.cpp'  -not -path './arch/*' -not -path './mod/*') $(shell find './arch/$(ARCH)/.' -type f -name '*.cpp'  -not -path './arch/$(ARCH)/./mod/*')
HXXFILES  := $(shell find . -type f -name '*.hpp'  -not -path './arch/*' -not -path './mod/*') $(shell find './arch/$(ARCH)/.' -type f -name '*.hpp'  -not -path './arch/$(ARCH)/./mod/*')
ASMFILES  := $(shell find . -type f -name '*.asm'  -not -path './arch/*' -not -path './mod/*') $(shell find './arch/$(ARCH)/.' -type f -name '*.asm'  -not -path './arch/$(ARCH)/./mod/*')
PSFFILES  := $(shell find . -type f -name '*.psf'  -not -path './arch/*' -not -path './mod/*') $(shell find './arch/$(ARCH)/.' -type f -name '*.psf'  -not -path './arch/$(ARCH)/./mod/*')
ASMRFILES := $(shell find . -type f -name '*.asmr' -not -path './arch/*' -not -path './mod/*') $(shell find './arch/$(ARCH)/.' -type f -name '*.asmr' -not -path './arch/$(ARCH)/./mod/*')

OBJS    := $(patsubst %.o, $(OBJ_DEST)%.o, $(CXXFILES:.cpp=.cpp.o) $(ASMFILES:.asm=.asm.o) $(PSFFILES:.psf=.psf.o) $(ASMRFILES:.asmr=.asmr.o))
VERSION := $(shell date -u '+%d.%m.%Y').$(shell printf "%05d" $(shell date -d "1970-01-01 UTC $$(date -u +%T)" +%s))

ISO  = $(BIN)grubiso/
SYS  = $(ISO)sys/

GFLAGS = -O3 -Wall -Wextra -Werror -nostdlib -pipe -lgcc

INCLUDES := -I. -Iinclude/ -Iarch/$(ARCH)/ -Iarch/$(ARCH)/include/

KERNEL_SRC := $(shell pwd)/

CXXFLAGS := $(GFLAGS)		   \
	-std=c++17				   \
	-fno-rtti				   \
	-fno-exceptions			   \
	-ffreestanding			   \
	-masm=intel				   \
	-mcmodel=kernel			   \
	-fno-pic			   	   \
	-fno-stack-protector       \
	-fno-omit-frame-pointer    \
	-mno-red-zone		       \
	-fvisibility=hidden		   \
	-DARCH="\"$(ARCH)\""       \
	-DVERSION="\"$(VERSION)\"" \
	$(INCLUDES)

ASFLAGS := -felf64

LDFLAGS := $(GFLAGS)		\
	-ffreestanding			\
	-z max-page-size=0x1000 \
	-no-pie

format:
	@$(MKDIR) $(ISO) $(SYS)
	clang-format -style=file -i ${CXXFILES} ${HXXFILES}

all: $(OBJS)	
	@$(MKDIR) $(ISO) $(SYS) $(SYS)core/ $(SYS)init/

	cd mod/core && $(MAKE) all ROOT_DIR="$(ROOT_DIR)" KERNEL_SRC="$(KERNEL_SRC)" && cd ../..
	cd mod/init && $(MAKE) all ROOT_DIR="$(ROOT_DIR)" KERNEL_SRC="$(KERNEL_SRC)" && cd ../..
	cd arch/$(ARCH)/mod/core && $(MAKE) all ROOT_DIR="$(ROOT_DIR)" KERNEL_SRC="$(KERNEL_SRC)" && cd ../../../..
	cd arch/$(ARCH)/mod/init && $(MAKE) all ROOT_DIR="$(ROOT_DIR)" KERNEL_SRC="$(KERNEL_SRC)" && cd ../../../..

	@$(CXX) $(OBJS) $(LDFLAGS) -T linker.ld -o $(SYS)aexkrnl
	objcopy --localize-hidden -K __dso_handle $(SYS)aexkrnl
	nm $(SYS)aexkrnl | grep -o 'W \w*$$' | sed 's/W /-L/g' | xargs objcopy $(SYS)aexkrnl
	objcopy -x -g -K __dso_handle $(SYS)aexkrnl
	objcopy --extract-symbol $(SYS)aexkrnl $(SYS)aexkrnl.sf

copy:
	@cp $(SYS)aexkrnl    "$(ROOT_DIR)sys/"
	@cp $(SYS)aexkrnl.sf "$(ROOT_DIR)sys/"

include $(shell find $(DEP_DEST) -type f -name *.d)

clean:
	cd mod/core && $(MAKE) clean && cd ../..
	cd mod/init && $(MAKE) clean && cd ../..

	cd arch/$(ARCH)/mod/core && $(MAKE) clean && cd ../../../..
	cd arch/$(ARCH)/mod/init && $(MAKE) clean && cd ../../../..

	rm -rf $(DEP_DEST)
	rm -rf $(OBJ_DEST)

$(OBJ_DEST)%.cpp.o : %.cpp
	@$(MKDIR) ${@D}
	@$(MKDIR) $(dir $(DEP_DEST)$*)
	$(CXX) $(CXXFLAGS) -c $< -o $@ -MMD -MT $@ -MF $(DEP_DEST)$*.cpp.d

$(OBJ_DEST)%.asm.o : %.asm
	@$(MKDIR) ${@D}
	$(AS) $(ASFLAGS) $< -o $@
	objcopy --weaken $@

$(OBJ_DEST)%.psf.o : %.psf
	@$(MKDIR) ${@D}
	objcopy -B i386:x86-64 -O elf64-x86-64 -I binary $< $@
	objcopy --weaken $@

$(OBJ_DEST)%.asmr.o : %.asmr
	@$(MKDIR) ${@D}
	$(AS) -f bin -o $@ $<
	objcopy -B i386:x86-64 -O elf64-x86-64 -I binary $@ $@
	objcopy --weaken $@

iso:
	@$(MKDIR) $(ISO)bin/ $(ISO)dev/ $(ISO)mnt/
	@grub-mkrescue -o $(BIN)aex.iso $(ISO) 2> /dev/null

qemunet:
	qemu-system-x86_64 -monitor stdio -machine type=q35 -smp 4 -m 32M -cdrom $(BIN)aex.iso \
	-netdev tap,id=net0,ifname=TAP -device rtl8139,netdev=net0,mac=00:01:e3:00:00:00 	   \
	--enable-kvm
	
qemu:
	qemu-system-x86_64 -monitor stdio -machine type=q35 -smp 4 -m 32M -cdrom $(BIN)aex.iso