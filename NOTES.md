# Design / WIP notes

## TODO

- Better page-allocation model
- Allow for more than one IOAPIC in ACPI module
  - The objects get created, but GSI lookup only uses the one at index 0
- Slab allocator for kernel structures
- mark kernel memory pages global
- kernel allocator `free()`
- lock `memory_manager` and `page_manager` structures
- Serial out based on circular/bip biffer and interrupts, not spinning on
  `write_ready()`
- Get framebuffer screen working again
