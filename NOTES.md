# Design / WIP notes

## Bootloader / UEFI

* What is the interface between the UEFI boot portion and the kernel?
  * Allocate pages, use UEFI reserved mapping types (0x70000000 - 0x7fffffff)
  * Passing memory map: Allocate earliest pages and fill with UEFI's memory map
    stuctures?


