ARCH           ?= x86_64

include src/arch/$(ARCH)/config.mk

BUILD_D        := build
KERN_D         := src/kernel
ARCH_D         := src/arch/$(ARCH)
VERSION        ?= $(shell git describe --dirty --always)
GITSHA         ?= $(shell git rev-parse --short HEAD)

KERNEL_FILENAME:= popcorn.elf
KERNEL_FONT    := assets/fonts/tamsyn8x16r.psf

MODULES        := kutil


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
INCLUDES       += -I src/include
INCLUDES       += -isystem $(EFI_INCLUDES)
INCLUDES       += -isystem $(EFI_INCLUDES)/$(ARCH)
INCLUDES       += -isystem $(EFI_INCLUDES)/protocol

BASEFLAGS      := -ggdb -nostdlib
BASEFLAGS      += -ffreestanding -nodefaultlibs
BASEFLAGS      += -fno-builtin -fomit-frame-pointer
BASEFLAGS      += -mno-red-zone -fno-stack-protector 

ifdef CPU
BASEFLAGS      += -mcpu=$(CPU)
endif

# Removed Flags:: -Wcast-align
WARNFLAGS      += -Wformat=2 -Winit-self -Wfloat-equal -Winline 
WARNFLAGS      += -Winvalid-pch -Wmissing-format-attribute
WARNFLAGS      += -Wmissing-include-dirs -Wswitch -Wundef
WARNFLAGS      += -Wdisabled-optimization -Wpointer-arith

WARNFLAGS      += -Wno-attributes -Wno-sign-compare -Wno-multichar
WARNFLAGS      += -Wno-div-by-zero -Wno-endif-labels -Wno-pragmas
WARNFLAGS      += -Wno-format-extra-args -Wno-unused-result
WARNFLAGS      += -Wno-deprecated-declarations -Wno-unused-function

ASFLAGS        ?=
ASFLAGS        += -p $(BUILD_D)/versions.s

CFLAGS         := $(INCLUDES) $(DEPENDFLAGS) $(BASEFLAGS) $(WARNFLAGS)
CFLAGS         += -DGIT_VERSION="\"$(VERSION)\""
CFLAGS         += -std=c11 -mcmodel=large

CXXFLAGS       := $(INCLUDES) $(DEPENDFLAGS) $(BASEFLAGS) $(WARNFLAGS)
CXXFLAGS       += -DGIT_VERSION="\"$(VERSION)\""
CXXFLAGS       += -std=c++14 -mcmodel=large
CXXFLAGS       += -fno-exceptions -fno-rtti

BOOT_CFLAGS    := $(INCLUDES) $(DEPENDFLAGS) $(BASEFLAGS) $(WARNFLAGS)
BOOT_CFLAGS    += -std=c11 -I src/boot -fPIC -fshort-wchar
BOOT_CFLAGS    += -DKERNEL_FILENAME="L\"$(KERNEL_FILENAME)\""
BOOT_CFLAGS    += -DGIT_VERSION_WIDE="L\"$(VERSION)\""
BOOT_CFLAGS    += -DGNU_EFI_USE_MS_ABI -DHAVE_USE_MS_ABI
BOOT_CFLAGS    += -DEFI_DEBUG=0 -DEFI_DEBUG_CLEAR_MEMORY=0
#BOOT_CFLAGS   += -DEFI_FUNCTION_WRAPPER

ifdef MAX_HRES
BOOT_CFLAGS    += -DMAX_HRES=$(MAX_HRES)
endif

LDFLAGS        := -L $(BUILD_D) -g
LDFLAGS        += -nostdlib -znocombreloc -Bsymbolic -nostartfiles 

BOOT_LDFLAGS   := $(LDFLAGS) -shared
BOOT_LDFLAGS   += -L $(EFI_ARCH_DIR)/lib -L $(EFI_ARCH_DIR)/gnuefi

AS             ?= $(CROSS)nasm
AR             ?= $(CROSS)ar
CC             ?= $(CROSS)gcc
CXX            ?= $(CROSS)g++
LD             ?= $(CROSS)ld
OBJC           := $(CROSS)objcopy
OBJD           := $(CROSS)objdump

INIT_DEP       := $(BUILD_D)/.builddir

BOOT_SRCS      := $(wildcard src/boot/*.c)
BOBJS          += $(patsubst src/boot/%,$(BUILD_D)/boot/%,$(patsubst %,%.o,$(BOOT_SRCS)))

KERN_SRCS      := $(wildcard $(KERN_D)/*.s)
KERN_SRCS      += $(wildcard $(KERN_D)/*.c)
KERN_SRCS      += $(wildcard $(KERN_D)/*.cpp)
KERN_SRCS      += $(wildcard $(ARCH_D)/*.s)
KERN_SRCS      += $(wildcard $(ARCH_D)/*.c)

KERN_OBJS      := $(patsubst src/%, $(BUILD_D)/%, $(patsubst %,%.o,$(KERN_SRCS)))

MOD_TARGETS    :=

PARTED         ?= /sbin/parted
QEMU           ?= qemu-system-x86_64
GDBPORT        ?= 27006
CPUS           ?= 1
OVMF           ?= assets/ovmf/x64/OVMF.fd 

QEMUOPTS       := -pflash $(BUILD_D)/flash.img
QEMUOPTS       += -drive file=$(BUILD_D)/fs.img,format=raw
QEMUOPTS       += -smp $(CPUS) -m 512
QEMUOPTS       += -d mmu,guest_errors,int -D popcorn.log 
QEMUOPTS       += -no-reboot
QEMUOPTS       += -cpu Broadwell
QEMUOPTS       += $(QEMUEXTRA)


all: $(BUILD_D)/fs.img
init: $(INIT_DEP)

$(INIT_DEP):
	mkdir -p $(BUILD_D) $(patsubst %,$(BUILD_D)/d.%,$(MODULES))
	mkdir -p $(BUILD_D)/boot
	mkdir -p $(patsubst src/%,$(BUILD_D)/%,$(ARCH_D))
	mkdir -p $(patsubst src/%,$(BUILD_D)/%,$(KERN_D))
	touch $(INIT_DEP)

clean:
	rm -rf $(BUILD_D)/* $(BUILD_D)/.version $(BUILD_D)/.builddir

vars:
	@echo "KERN_SRCS: " $(KERN_SRCS)
	@echo "KERN_OBJS: " $(KERN_OBJS)

dist-clean: clean
	make -C external/gnu-efi clean

.PHONY: all clean dist-clean init vars

$(BUILD_D)/.version:
	echo '$(VERSION)' | cmp -s - $@ || echo '$(VERSION)' > $@

$(BUILD_D)/versions.s:
	./parse_version.py "$(VERSION)" "$(GITSHA)" > $@

-include x $(patsubst %,src/modules/%/module.mk,$(MODULES))
-include x $(shell find $(BUILD_D) -type f -name '*.d')

$(EFI_LIB):
	make -C external/gnu-efi all

$(BUILD_D)/boot.elf: $(BOBJS) $(EFI_LIB)
	$(LD) $(BOOT_LDFLAGS) -T $(EFI_LDS) -o $@ \
		$(EFI_CRT_OBJ) $(BOBJS) -lefi -lgnuefi

$(BUILD_D)/boot.efi: $(BUILD_D)/boot.elf
	$(OBJC) -j .text -j .sdata -j .data -j .dynamic \
	-j .dynsym  -j .rel -j .rela -j .reloc \
	--target=efi-app-$(ARCH) $^ $@

$(BUILD_D)/boot.debug.efi: $(BUILD_D)/boot.elf
	$(OBJC) -j .text -j .sdata -j .data -j .dynamic \
	-j .dynsym  -j .rel -j .rela -j .reloc \
	-j .debug_info -j .debug_abbrev -j .debug_loc -j .debug_str \
	-j .debug_aranges -j .debug_line -j .debug_macinfo \
	--target=efi-app-$(ARCH) $^ $@

$(BUILD_D)/boot.dump: $(BUILD_D)/boot.efi
	$(OBJD) -D -S $< > $@

$(BUILD_D)/boot/%.s.o: src/boot/%.s $(BUILD_D)/versions.s $(INIT_DEP)
	$(AS) $(ASFLAGS) -i src/boot/ -o $@ $<

$(BUILD_D)/boot/%.c.o: src/boot/%.c $(INIT_DEP)
	$(CC) $(BOOT_CFLAGS) -c -o $@ $<

$(BUILD_D)/kernel.elf: $(KERN_OBJS) $(MOD_TARGETS) $(ARCH_D)/kernel.ld
	$(LD) $(LDFLAGS) -u _header -T $(ARCH_D)/kernel.ld -o $@ $(KERN_OBJS) $(patsubst %,-l%,$(MODULES))
	$(OBJC) --only-keep-debug $@ $@.sym
	$(OBJD) -D -S $@ > $@.dump

$(BUILD_D)/%.s.o: src/%.s $(BUILD_D)/versions.s $(INIT_DEP)
	$(AS) $(ASFLAGS) -i $(ARCH_D)/ -i $(KERN_D)/ -o $@ $<

$(BUILD_D)/%.c.o: src/%.c $(INIT_DEP)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_D)/%.cpp.o: src/%.cpp $(INIT_DEP)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD_D)/flash.img: $(OVMF)
	cp $^ $@

$(BUILD_D)/screenfont.psf: $(KERNEL_FONT)
	cp $^ $@

$(BUILD_D)/fs.img: $(BUILD_D)/boot.efi $(BUILD_D)/kernel.elf $(BUILD_D)/screenfont.psf
	$(eval TEMPFILE := $(shell mktemp --suffix=.img))
	dd if=/dev/zero of=$@.tmp bs=512 count=93750
	$(PARTED) $@.tmp -s -a minimal mklabel gpt
	$(PARTED) $@.tmp -s -a minimal mkpart EFI FAT16 2048s 93716s
	$(PARTED) $@.tmp -s -a minimal toggle 1 boot
	dd if=/dev/zero of=$(TEMPFILE) bs=512 count=91669
	mformat -i $(TEMPFILE) -h 32 -t 32 -n 64 -c 1
	mmd -i $(TEMPFILE) ::/EFI
	mmd -i $(TEMPFILE) ::/EFI/BOOT
	mcopy -i $(TEMPFILE) $(BUILD_D)/boot.efi ::/EFI/BOOT/BOOTX64.efi
	mcopy -i $(TEMPFILE) $(BUILD_D)/kernel.elf ::$(KERNEL_FILENAME)
	mcopy -i $(TEMPFILE) $(BUILD_D)/screenfont.psf ::screenfont.psf
	mlabel -i $(TEMPFILE) ::Popcorn_OS
	dd if=$(TEMPFILE) of=$@.tmp bs=512 count=91669 seek=2048 conv=notrunc
	rm $(TEMPFILE)
	mv $@.tmp $@

$(BUILD_D)/fs.iso: $(BUILD_D)/fs.img
	mkdir -p $(BUILD_D)/iso
	cp $< $(BUILD_D)/iso/
	xorriso -as mkisofs -R -f -e fs.img -no-emul-boot -o $@ $(BUILD_D)/iso

qemu: $(BUILD_D)/fs.img $(BUILD_D)/flash.img
	-rm popcorn.log
	"$(QEMU)" $(QEMUOPTS) -nographic

qemu-window: $(BUILD_D)/fs.img $(BUILD_D)/flash.img
	-rm popcorn.log
	"$(QEMU)" $(QEMUOPTS)

qemu-gdb: $(BUILD_D)/fs.img $(BUILD_D)/boot.debug.efi $(BUILD_D)/flash.img $(BUILD_D)/kernel.elf
	-rm popcorn.log
	"$(QEMU)" $(QEMUOPTS) -s -nographic

# vim: ft=make ts=4
