#!/usr/bin/env bash

build=${1:-"$(dirname $0)/build"}

kvm=""
if [[ -f /dev/kvm ]]; then
	kvm="--enable-kvm"
fi

ninja -C $build && \
exec qemu-system-x86_64 \
	-drive "if=pflash,format=raw,file=${build}/flash.img" \
	-drive "format=raw,file=${build}/popcorn.img" \
	-smp 1 \
	-m 512 \
	-d mmu,int,guest_errors \
	-D popcorn.log \
	-cpu Broadwell \
	-M q35 \
	-no-reboot \
	-nographic \
	$kvm
