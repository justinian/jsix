#!/usr/bin/env bash

build="$(dirname $0)/build"
assets="$(dirname $0)/assets"
debug=""
flash_name="ovmf_vars"
gfx="-nographic"
kvm=""

for arg in $@; do
	case "${arg}" in
		--debug)
			debug="-s"
			flash_name="ovmf_vars_d"
			;;
		--gfx)
			gfx=""
			;;
		--kvm)
			kvm="-enable-kvm"
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
		tmux split-window "gdb ${build}/popcorn.elf" &
	else
		tmux split-window -l 10 "sleep 1; telnet localhost 45454" &
	fi
elif [[ $DESKTOP_SESSION = "i3" ]]; then
	if [[ -n $debug ]]; then
		i3-msg exec i3-sensible-terminal -- -e "gdb ${PWD}/${build}/popcorn.elf" &
	else
		i3-msg exec i3-sensible-terminal -- -e 'telnet localhost 45454' &
	fi
fi

exec qemu-system-x86_64 \
	-drive "if=pflash,format=raw,readonly,file=${assets}/ovmf/x64/ovmf_code.fd" \
	-drive "if=pflash,format=raw,file=${build}/${flash_name}.fd" \
	-drive "format=raw,file=${build}/popcorn.img" \
	-monitor telnet:localhost:45454,server,nowait \
	-smp 4 \
	-m 512 \
	-d mmu,int,guest_errors \
	-D popcorn.log \
	-cpu Broadwell \
	-M q35 \
	-no-reboot \
	$gfx $kvm $debug
