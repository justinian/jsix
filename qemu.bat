call make.bat
del popcorn.log
qemu-system-x86_64.exe -bios .\assets\ovmf\x64\OVMF.fd -hda .\build\fs.img -m 512 -vga cirrus -d guest_errors,int,mmu -D popcorn.log -no-reboot
