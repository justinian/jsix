call make.bat
qemu-system-x86_64.exe -bios .\assets\ovmf\x64\OVMF.fd -hda .\build\fs.img -m 512 -vga cirrus
