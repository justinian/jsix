ARCH           ?= x86_64

include src/arch/$(ARCH)/config.mk

BUILD_D        := build
ARCH_D         := src/arch/$(ARCH)
VERSION        ?= $(shell git describe --dirty --always)


EFI_DIR        := external/gnu-efi
EFI_DATA       := $(EFI_DIR)/gnuefi
EFI_LDS        := $(EFI_DATA)/elf_$(ARCH)_efi.lds
EFI_ARCH_DIR   := $(EFI_DIR)/$(ARCH)
EFI_ARCH_DATA  := $(EFI_ARCH_DIR)/gnuefi
EFI_CRT_OBJ    := $(EFI_ARCH_DATA)/crt0-efi-$(ARCH).o
EFI_LIB        := $(EFI_ARCH_DIR)/lib/libefi.a
EFI_INCLUDES   := $(EFI_DIR)/inc

DEPENDFLAGS    := -MMD

INCLUDES       := -I $(ARCH_D)
INCLUDES       += -I src/modules
INCLUDES       += -isystem $(EFI_INCLUDES) -isystem $(EFI_INCLUDES)/$(ARCH) -isystem $(EFI_INCLUDES)/protocol

BASEFLAGS      := -O2 -fpic -nostdlib
BASEFLAGS      += -ffreestanding -nodefaultlibs
BASEFLAGS      += -fno-builtin -fomit-frame-pointer

ifdef CPU
BASEFLAGS      += -mcpu=$(CPU)
endif

WARNFLAGS      := -Wall -Wextra -Wshadow -Wcast-align -Wwrite-strings
WARNFLAGS      += -Winline -Wshadow
WARNFLAGS      += -Wno-attributes -Wno-deprecated-declarations
WARNFLAGS      += -Wno-div-by-zero -Wno-endif-labels -Wfloat-equal
WARNFLAGS      += -Wformat=2 -Wno-format-extra-args -Winit-self
WARNFLAGS      += -Winvalid-pch -Wmissing-format-attribute
WARNFLAGS      += -Wmissing-include-dirs -Wno-multichar
WARNFLAGS      += -Wno-sign-compare -Wswitch -Wundef
WARNFLAGS      += -Wno-pragmas #-Wno-unused-but-set-parameter
WARNFLAGS      += -Wno-unused-result #-Wno-unused-but-set-variable 
WARNFLAGS      += -Wwrite-strings -Wdisabled-optimization -Wpointer-arith
WARNFLAGS      += -Werror

ASFLAGS        ?= 

CFLAGS         ?= 
CFLAGS         += $(INCLUDES) $(DEPENDFLAGS) $(BASEFLAGS) $(WARNFLAGS)
CFLAGS         += -DGIT_VERSION="\"$(VERSION)\""
CFLAGS         += -std=c11 -fno-stack-protector -fpic -fshort-wchar -mno-red-zone
CFLAGS         += -DEFI_DEBUG=0 -DEFI_DEBUG_CLEAR_MEMORY=0 -DGNU_EFI_USE_MS_ABI -DHAVE_USE_MS_ABI #-DEFI_FUNCTION_WRAPPER 

LDFLAGS        ?= 
LDFLAGS        += -L $(BUILD_D) -ggdb
LDFLAGS        += -nostdlib -znocombreloc -shared -Bsymbolic -fPIC -nostartfiles 
LDFLAGS        += -L $(EFI_ARCH_DIR)/lib -L $(EFI_ARCH_DIR)/gnuefi

AS             ?= $(CROSS)as
AR             ?= $(CROSS)ar
CC             ?= $(CROSS)gcc
CXX            ?= $(CROSS)g++
LD             ?= $(CROSS)ld
OBJC           := $(CROSS)objcopy
OBJD           := $(CROSS)objdump

INIT_DEP       := $(BUILD_D)/.builddir
ARCH_SRCS      := $(wildcard $(ARCH_D)/*.s)
ARCH_SRCS      += $(wildcard $(ARCH_D)/*.c)
KOBJS          += $(patsubst $(ARCH_D)/%,$(BUILD_D)/arch/%,$(patsubst %,%.o,$(ARCH_SRCS)))
DEPS           :=
MOD_TARGETS    :=

PARTED         ?= /sbin/parted
QEMU           ?= qemu-system-x86_64
GDBPORT        ?= 27006
CPUS           ?= 2
OVMF           ?= assets/ovmf/x64/OVMF.fd 
QEMUOPTS       := -bios $(OVMF) -hda $(BUILD_D)/fs.img -smp $(CPUS) -m 512 -nographic $(QEMUEXTRA)


all: $(BUILD_D)/fs.img
init: $(INIT_DEP)

$(INIT_DEP):
	mkdir -p $(BUILD_D) $(patsubst %,$(BUILD_D)/d.%,$(MODULES))
	mkdir -p $(BUILD_D)/board $(BUILD_D)/arch
	touch $(INIT_DEP)

clean:
	rm -rf $(BUILD_D)/* $(BUILD_D)/.version $(BUILD_D)/.builddir

dist-clean: clean
	make -C external/gnu-efi clean

.PHONY: all clean dist-clean init

$(BUILD_D)/.version:
	echo '$(VERSION)' | cmp -s - $@ || echo '$(VERSION)' > $@

-include x $(patsubst %,src/modules/%/module.mk,$(MODULES))
-include x $(DEPS)

$(EFI_LIB):
	make -C external/gnu-efi all

$(BUILD_D)/kernel.elf: $(KOBJS) $(MOD_TARGETS) $(EFI_LIB)
	$(LD) $(LDFLAGS) -T $(EFI_LDS) -o $@ \
		$(EFI_CRT_OBJ) $(KOBJS) $(patsubst %,-l%,$(MODULES)) -lefi -lgnuefi

$(BUILD_D)/kernel.efi: $(BUILD_D)/kernel.elf
	objcopy -j .text -j .sdata -j .data -j .dynamic \
	-j .dynsym  -j .rel -j .rela -j .reloc \
	--target=efi-app-$(ARCH) $^ $@

$(BUILD_D)/kernel.debug.efi: $(BUILD_D)/kernel.elf
	objcopy -j .text -j .sdata -j .data -j .dynamic \
	-j .dynsym  -j .rel -j .rela -j .reloc \
	-j .debug_info -j .debug_abbrev -j .debug_loc -j .debug_str \
	-j .debug_aranges -j .debug_line -j .debug_macinfo \
	--target=efi-app-$(ARCH) $^ $@

$(BUILD_D)/%.dump: $(BUILD_D)/%.efi
	$(OBJD) -D -S $< > $@

$(BUILD_D)/arch/%.s.o: $(ARCH_D)/%.s $(INIT_DEP)
	$(AS) $(ASFLAGS) -o $@ $<

$(BUILD_D)/arch/%.c.o: $(ARCH_D)/%.c $(INIT_DEP)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_D)/fs.img: $(BUILD_D)/kernel.efi
	$(eval TEMPFILE := $(shell mktemp --suffix=.img))
	dd if=/dev/zero of=$@.tmp bs=512 count=93750
	$(PARTED) $@.tmp -s -a minimal mklabel gpt
	$(PARTED) $@.tmp -s -a minimal mkpart EFI FAT16 2048s 93716s
	$(PARTED) $@.tmp -s -a minimal toggle 1 boot
	dd if=/dev/zero of=$(TEMPFILE) bs=512 count=91669
	mformat -i $(TEMPFILE) -h 32 -t 32 -n 64 -c 1
	mmd -i $(TEMPFILE) ::/EFI
	mmd -i $(TEMPFILE) ::/EFI/BOOT
	mcopy -i $(TEMPFILE) $^ ::/EFI/BOOT/BOOTX64.efi
	dd if=$(TEMPFILE) of=$@.tmp bs=512 count=91669 seek=2048 conv=notrunc
	rm $(TEMPFILE)
	mv $@.tmp $@

qemu: $(BUILD_D)/fs.img
	"$(QEMU)" $(QEMUOPTS)

qemu-gdb: $(BUILD_D)/fs.img $(BUILD_D)/kernel.debug.efi
	"$(QEMU)" $(QEMUOPTS) -S -D popcorn-qemu.log -s

# vim: ft=make ts=4
