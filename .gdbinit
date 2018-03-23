set architecture i386:x86-64
symbol-file build/kernel.elf.sym
target remote localhost:1234
break kernel_main
continue
disconnect
set architecture i386:x86-64
target remote localhost:1234
