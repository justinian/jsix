# Design / WIP notes

## TODO

- Paging manager
  - Copy-on-write pages
  - Better page-allocation model?
- Allow for more than one IOAPIC in ACPI module
  - The objects get created, but GSI lookup only uses the one at index 0
- mark kernel memory pages global
- Serial out based on circular/bip biffer and interrupts, not spinning on
  `write_ready()`
- Split out more code into kutil for testing
- AHCI / MSI interrupts on Vbox break?
- FXSAVE to save XMM registers.
  - optimization using #NM (0x7) to detect SSE usage
- Clean up of process memory maps
- Better stack tracer
- Bootloader rewrite
  - C++ and sharing library code for ELF, initrd, etc
  - Parse initrd and pre-load certain ELF images, eg the process loader process?
  - Do initial memory bootstrap?
- Calling global ctors
- Device Tree
  - Actual serial driver
  - Disk driver
    - File system
- Multiprocessing
  - Fast syscalls using syscall/sysret

### Build

- Clean up build generator and its templates
  - More robust objects to represent modules & targets to pass to templates
  - Read project setup from a simple JSON/TOML/etc
  - Better compartmentalizing when doing template inheritance
- Move to LLD as sysroot linker

