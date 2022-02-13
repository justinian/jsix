#!/usr/bin/env bash

build="$(dirname $0)/build"
assets="$(dirname $0)/assets"
vga="-vga none"
log=""
kvm=""
cpu="Broadwell,+pdpe1gb,-check"
smp=4
mem=2048
clean=""

while true; do
	case "$1" in
        -c | --clean)
            clean="yes"
            shift
            ;;
		-v | --vga)
			vga=""
            shift
			;;
		-k | --kvm)
			kvm="-enable-kvm"
			cpu="host"
            shift
			;;
        -c | --cpus)
            smp=$2
            shift 2
            ;;
        -m | --memory)
            mem=$2
            shift 2
            ;;
        -l | --log)
            log="-d mmu,int,guest_errors -D jsix.log"
            shift
            ;;
		*)
            if [ -d "$1" ]; then
                build="$1"
                shift
            fi
            break
			;;
	esac
done

if [[ ! -c /dev/kvm ]]; then
	kvm=""
fi

if [[ -n $clean ]]; then 
    ninja -C "${build}" -t clean
fi

if ! ninja -C "${build}"; then
	exit 255
fi

qemu-system-x86_64 \
	-drive "if=pflash,format=raw,readonly,file=${assets}/ovmf/x64/ovmf_code.fd" \
	-drive "if=pflash,format=raw,file=${build}/ovmf_vars.fd" \
	-drive "format=raw,file=${build}/jsix.img" \
	-serial "file:${build}/qemu_test_com1.txt" \
    -debugcon "file:${build}/qemu_debugcon.txt" \
	-smp "${smp}" \
	-m $mem \
	-cpu "${cpu}" \
	-M q35 \
	-no-reboot \
    -nographic \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
	$log $vga $kvm $debug > "${build}/qemu_test_out.txt"

result=$?
if [[ $result -eq 0 ]]; then
    echo "Somehow exited with status 0" > /dev/stderr
    exit 1
fi

if [[ $result -eq 1 ]]; then
    echo "QEMU returned an error status" > /dev/stderr
    exit 1
fi

((result = ($result >> 1) - 1))
if [[ $result -gt 0 ]]; then
    cat "${build}/qemu_test_com1.txt"
fi
exit $result
