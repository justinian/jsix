# Design / WIP notes

## TODO

- Better page-allocation model
- Allow for more than one IOAPIC in ACPI module
  - The objects get created, but GSI lookup only uses the one at index 0
- Slab allocator for kernel structures
- mark kernel memory pages global
- lock `memory_manager` and `page_manager` structures
- Serial out based on circular/bip biffer and interrupts, not spinning on
  `write_ready()`
- Split out more code into kutil for testing


- Device Tree
  - Actual serial driver
  - Disk driver
    - File system
- Memory map swapping
  - Multiprocessing
    - Processes in Ring 3
- Stack tracer
- build system upgrade (CMake / Waf / etc)
