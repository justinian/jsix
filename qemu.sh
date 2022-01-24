#!/usr/bin/env bash

build="$(dirname $0)/build"
assets="$(dirname $0)/assets"
debug=""
isaexit='-device isa-debug-exit,iobase=0xf4,iosize=0x04'
debugtarget="${build}/jsix.elf"
gfx="-nographic"
vga="-vga none"
log=""
kvm=""
cpu="Broadwell,+pdpe1gb"
smp=4

while true; do
	case "$1" in
		-b | --debugboot)
			debug="-s -S"
			debugtarget="${build}/boot/boot.efi"
            shift
			;;
		-d | --debug)
			debug="-s -S"
            isaexit=""
            shift
			;;
		-g | --gfx)
			gfx=""
			vga=""
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
		i3-msg exec i3-sensible-terminal -- -e "gdb ${debugtarget}" &
	else
		i3-msg exec i3-sensible-terminal -- -e 'telnet localhost 45454' &
	fi
fi

exec qemu-system-x86_64 \
	-drive "if=pflash,format=raw,readonly,file=${assets}/ovmf/x64/ovmf_code.fd" \
	-drive "if=pflash,format=raw,file=${build}/ovmf_vars.fd" \
	-drive "format=raw,file=${build}/jsix.img" \
	-monitor telnet:localhost:45454,server,nowait \
	-serial stdio \
	-serial telnet:localhost:45455,server,nowait \
	-smp "${smp}" \
	-m 4096 \
	-cpu "${cpu}" \
	-M q35 \
	-no-reboot \
	$isaexit $log $gfx $vga $kvm $debug
