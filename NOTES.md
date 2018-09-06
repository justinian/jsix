# Design / WIP notes

## TODO

- Better page-allocation model
- Allow for more than one IOAPIC in ACPI module
  - The objects get created, but GSI lookup only uses the one at index 0
- Slab allocator for kernel structures
- mark kernel memory pages global
- Serial out based on circular/bip biffer and interrupts, not spinning on
  `write_ready()`
- Split out more code into kutil for testing
- AHCI / MSI interrupts on Vbox break?
- FXSAVE to save XMM registers.
  - optimization using #NM (0x7) to detect SSE usage
- Clean up of process memory maps
- Better stack tracer


- Device Tree
  - Actual serial driver
  - Disk driver
    - File system
- Multiprocessing
  - Syscalls
