#!/usr/bin/env python3

from struct import unpack_from, calcsize
import sys

memory_type_names = {
     0: "EfiReservedMemoryType",
     1: "EfiLoaderCode",
     2: "EfiLoaderData",
     3: "EfiBootServicesCode",
     4: "EfiBootServicesData",
     5: "EfiRuntimeServicesCode",
     6: "EfiRuntimeServicesData",
     7: "EfiConventionalMemory",
     8: "EfiUnusableMemory",
     9: "EfiACPIReclaimMemory",
    10: "EfiACPIMemoryNVS",
    11: "EfiMemoryMappedIO",
    12: "EfiMemoryMappedIOPortSpace",
    13: "EfiPalCode",
    14: "EfiPersistentMemory",

    0x80000000: "Kernel Image",
    0x80000001: "Kernel Data",
}

EFI_MEMORY_UC = 0x0000000000000001
EFI_MEMORY_WC = 0x0000000000000002
EFI_MEMORY_WT = 0x0000000000000004
EFI_MEMORY_WB = 0x0000000000000008
EFI_MEMORY_UCE = 0x0000000000000010
EFI_MEMORY_WP = 0x0000000000001000
EFI_MEMORY_RP = 0x0000000000002000
EFI_MEMORY_XP = 0x0000000000004000
EFI_MEMORY_NV = 0x0000000000008000
EFI_MEMORY_MORE_RELIABLE = 0x0000000000010000
EFI_MEMORY_RO = 0x0000000000020000
EFI_MEMORY_RUNTIME = 0x8000000000000000

fmt = "LQQQQQ"
size = calcsize(fmt)

print("Descriptor size: {} bytes\n".format(size))
if size != 48:
    sys.exit(1)

data = open(sys.argv[1], 'rb').read()
length = len(data)
consumed = 0

while length - consumed > size:
    memtype, phys, virt, pages, attr, pad = unpack_from(fmt, data, consumed)
    consumed += size
    if pages == 0: break

    memtype = memory_type_names.get(memtype, "{:016x}".format(memtype))
    runtime = {EFI_MEMORY_RUNTIME: "*"}.get(attr & EFI_MEMORY_RUNTIME, " ")

    print("{:>23}{} {:016x} {:016x} [{:4d}]".format(memtype, runtime, phys, virt, pages))

