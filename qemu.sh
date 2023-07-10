#!/usr/bin/env bash

root=$(dirname $0)
build="${root}/build"
assets="${root}/assets"

no_build=""
debug=""
isaexit='-device isa-debug-exit,iobase=0xf4,iosize=0x04'
debugtarget="${build}/jsix.elf"
gfx="-nographic"
vga="-vga none"
log=""
kvm=""
debugcon_cmd=""
cpu_features=",+pdpe1gb,+invtsc,+hypervisor,+x2apic,+xsavec,+xsaves,+xsaveopt"
cpu="Broadwell"
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
        -r | --remote)
            shift
            vnchost="${1:-${VNCHOST:-"localhost:5500"}}"
            gfx="-vnc ${vnchost},reverse"
            vga=""
            shift
            ;;
        -k | --kvm)
            kvm="-enable-kvm"
            cpu="max"
            shift
            ;;
        -c | --cpus)
            smp=$2
            shift 2
            ;;
        -l | --log)
            log="-d mmu,int,guest_errors -D ${root}/jsix.log"
            shift
            ;;
        -x | --debugcon)
            debugcon_cmd="less --follow-name -R +F debugcon.log"
            shift
            ;;
        --no-build)
            no_build="yes"
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

[[ -z "${no_build}" ]] && if ! ninja -C "${build}"; then
    exit 1
fi

if [[ -n $TMUX ]]; then
    cols=$(tput cols)
    log_width=100

    if [[ -n $debug ]]; then
        log_cols=1
        if [[ $debugcon_cmd ]]; then
            log_cols=2
        fi

        gdb_width=$(($cols - $log_cols * $log_width))

        if (($gdb_width < 150)); then
            stack=1
            gdb_width=$(($cols - $log_width))
            tmux split-window -h -l $gdb_width "gdb ${debugtarget}"
            if [[ $debugcon_cmd ]]; then
                tmux select-pane -t .left
                tmux split-window -v "$debugcon_cmd"
            fi
        else
            if [[ $debugcon_cmd ]]; then
                tmux split-window -h -l $(($log_width + $gdb_width)) "$debugcon_cmd"
            fi
            tmux split-window -h -l $gdb_width "gdb ${debugtarget}"
            tmux select-pane -t .left
            tmux select-pane -t .right
        fi
    else
        if [[ $debugcon_cmd ]]; then
            tmux split-window -h -l $log_width "$debugcon_cmd"
            tmux last-pane
        fi
        tmux split-window -l 10 "sleep 1; telnet localhost 45454"
    fi
elif [[ $DESKTOP_SESSION = "i3" ]]; then
    if [[ -n $debug ]]; then
        i3-msg exec i3-sensible-terminal -- -e "gdb ${debugtarget}" &
    else
        i3-msg exec i3-sensible-terminal -- -e 'telnet localhost 45454' &
    fi
fi

if [[ -n "${debug}" ]]; then
    (sleep 1; echo "Debugging ready.") &
fi

qemu-system-x86_64 \
    -drive "if=pflash,format=raw,readonly=on,file=${assets}/ovmf/x64/ovmf_code.fd" \
    -drive "if=pflash,format=raw,file=${build}/ovmf_vars.fd" \
    -drive "format=raw,file=${build}/jsix.img" \
    -chardev "file,path=${root}/debugcon.log,id=jsix_debug" \
    -device "isa-debugcon,iobase=0x6600,chardev=jsix_debug" \
    -monitor telnet:localhost:45454,server,nowait \
    -serial stdio \
    -smp "${smp}" \
    -m 4096 \
    -cpu "${cpu}${cpu_features}" \
    -M q35 \
    -no-reboot \
    -name jsix \
    $isaexit $log $gfx $vga $kvm $debug

((result = ($? >> 1) - 1))
exit $result
