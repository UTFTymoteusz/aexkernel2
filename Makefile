CC = g++
AS = nasm

MKDIR = mkdir -p

BIN  := bin/
BOOT := boot/
DEP_DEST := $(BIN)dep/
OBJ_DEST := $(BIN)obj/

ARCH = arch/x64/

CFILES    := $(shell find . -type f -name '*.cpp'  -not -path './arch/*') $(shell find './$(ARCH).' -type f -name '*.cpp')
HFILES    := $(shell find . -type f -name '*.hpp'  -not -path './arch/*') $(shell find './$(ARCH).' -type f -name '*.hpp')
ASMFILES  := $(shell find . -type f -name '*.asm'  -not -path './arch/*') $(shell find './$(ARCH).' -type f -name '*.asm')
PSFFILES  := $(shell find . -type f -name '*.psf'  -not -path './arch/*') $(shell find './$(ARCH).' -type f -name '*.psf')
ASMRFILES := $(shell find . -type f -name '*.asmr' -not -path './arch/*') $(shell find './$(ARCH).' -type f -name '*.asmr')

OBJS := $(patsubst %.o, $(OBJ_DEST)%.o, $(CFILES:.cpp=.cpp.o) $(ASMFILES:.asm=.asm.o) $(PSFFILES:.psf=.psf.o) $(ASMRFILES:.asmr=.asmr.o))

ISO  = $(BIN)grubiso/
SYS  = $(ISO)sys/

GFLAGS = -O3 -Wall -Wextra -nostdlib -pipe

INCLUDES := -I. -I$(ARCH) -I$(ARCH)include/ -Iinclude/ -Iinclude/libc/ -I../lai/include/

CCFLAGS := $(GFLAGS)     \
	-std=c++17           \
	-fno-rtti            \
	-fno-exceptions      \
	-ffreestanding       \
	-masm=intel          \
	-mcmodel=kernel      \
	-fno-pic             \
	-fno-stack-protector \
	-mno-red-zone        \
	-fno-omit-frame-pointer \
	$(INCLUDES)

ASFLAGS := -felf64 \
	-wall

LDFLAGS := $(GFLAGS) \
	-ffreestanding   \
	-z max-page-size=0x1000 \
	-no-pie

format:
	@$(MKDIR) $(ISO) $(ISO)bin/ $(ISO)dev/ $(ISO)mnt/ $(ISO)sys/
	clang-format -style=file -i ${CFILES} ${HFILES}

all: $(OBJS)
	@$(MKDIR) $(ISO) $(ISO)bin/ $(ISO)dev/ $(ISO)mnt/ $(ISO)sys/
	@$(CC) $(OBJS) $(LDFLAGS) -T linker.ld -o $(SYS)aexkrnl.elf

include $(shell find $(DEP_DEST) -type f -name *.d)

clean:
	rm -rf $(DEP_DEST)
	rm -rf $(OBJ_DEST)
	rm -rf $(SYS)core/

$(OBJ_DEST)%.cpp.o : %.cpp
	@$(MKDIR) ${@D}
	@$(MKDIR) $(dir $(DEP_DEST)$*)
	$(CC) $(CCFLAGS) -c $< -o $@ -MMD -MT $@ -MF $(DEP_DEST)$*.cpp.d

$(OBJ_DEST)%.asm.o : %.asm
	@$(MKDIR) ${@D}
	$(AS) $(ASFLAGS) $< -o $@

$(OBJ_DEST)%.psf.o : %.psf
	@$(MKDIR) ${@D}
	@objcopy -B i386:x86-64 -O elf64-x86-64 -I binary $< $@

$(OBJ_DEST)%.asmr.o : %.asmr
	@$(MKDIR) ${@D}
	$(AS) -f bin -o $@ $<
	@objcopy -B i386:x86-64 -O elf64-x86-64 -I binary $@ $@

iso:
	@$(MKDIR) $(ISO)bin/ $(ISO)dev/ $(ISO)mnt/
	@grub-mkrescue -o $(BIN)aex.iso $(ISO) 2> /dev/null

qemunet:
	qemu-system-x86_64 -monitor stdio -machine type=q35 -smp 4 -m 32M -cdrom $(BIN)aex.iso \
	-netdev tap,id=net0,ifname=TAP -device rtl8139,netdev=net0,mac=00:01:e3:00:00:00 	   \
	--enable-kvm
	
qemu:
	qemu-system-x86_64 -monitor stdio -machine type=q35 -smp 1 -m 32M -cdrom $(BIN)aex.iso