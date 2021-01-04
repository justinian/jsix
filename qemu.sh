#!/usr/bin/env bash

build="$(dirname $0)/build"
assets="$(dirname $0)/assets"
debug=""
debugtarget="${build}/jsix.elf"
flash_name="ovmf_vars"
gfx="-nographic"
vga="-vga none"
kvm=""
cpu="Broadwell,+pdpe1gb"

for arg in $@; do
	case "${arg}" in
		--debugboot)
			debug="-s -S"
			debugtarget="${build}/boot/boot.efi"
			flash_name="ovmf_vars_d"
			;;
		--debug)
			debug="-s -S"
			flash_name="ovmf_vars_d"
			;;
		--gfx)
			gfx=""
			vga=""
			;;
		--vga)
			vga=""
			;;
		--kvm)
			kvm="-enable-kvm"
			cpu="host"
			;;
		*)
			build="${arg}"
			;;
	esac
done

if [[ ! -c /dev/kvm ]]; then
	kvm=""
fi

if ! ninja -C "${build}"; then
	exit 1
fi

if [[ -n $TMUX ]]; then
	if [[ -n $debug ]]; then
		tmux split-window -h "gdb ${debugtarget}" &
	else
		tmux split-window -h -l 80 "sleep 1; telnet localhost 45455" &
		tmux last-pane
		tmux split-window -l 10 "sleep 1; telnet localhost 45454" &
	fi
elif [[ $DESKTOP_SESSION = "i3" ]]; then
	if [[ -n $debug ]]; then
		i3-msg exec i3-sensible-terminal -- -e "gdb ${PWD}/${build}/jsix.elf" &
	else
		i3-msg exec i3-sensible-terminal -- -e 'telnet localhost 45454' &
	fi
fi

exec qemu-system-x86_64 \
	-drive "if=pflash,format=raw,readonly,file=${assets}/ovmf/x64/ovmf_code.fd" \
	-drive "if=pflash,format=raw,file=${build}/${flash_name}.fd" \
	-drive "format=raw,file=${build}/jsix.img" \
	-device "isa-debug-exit,iobase=0xf4,iosize=0x04" \
	-monitor telnet:localhost:45454,server,nowait \
	-serial stdio \
	-serial telnet:localhost:45455,server,nowait \
	-smp 4 \
	-m 512 \
	-d mmu,int,guest_errors \
	-D jsix.log \
	-cpu "${cpu}" \
	-M q35 \
	-no-reboot \
	$gfx $vga $kvm $debug
