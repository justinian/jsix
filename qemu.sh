#!/usr/bin/env bash

build="$(dirname $0)/build"
debug=""
gfx="-nographic"

for arg in $@; do
	case "${arg}" in
		--debug)
			debug="-s"
			;;
		--gfx)
			gfx=""
			;;
		*)
			build="${arg}"
			;;
	esac
done

kvm=""
if [[ -c /dev/kvm ]]; then
	kvm="-enable-kvm"
fi

if ! ninja -C "${build}"; then
	exit 1
fi

if [[ -n $TMUX ]]; then
	tmux split-window "sleep 1; telnet localhost 45454" &
fi

exec qemu-system-x86_64 \
	-drive "if=pflash,format=raw,file=${build}/flash.img" \
	-drive "format=raw,file=${build}/popcorn.img" \
	-monitor telnet:localhost:45454,server,nowait \
	-smp 1 \
	-m 512 \
	-d mmu,int,guest_errors \
	-D popcorn.log \
	-cpu Broadwell \
	-M q35 \
	-no-reboot \
	$gfx $kvm $debug
