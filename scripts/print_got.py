#!/usr/bin/env python3


def section_header(name):
    print()
    print(name)
    print("-" * 40)


def print_dyn(elf):
    section = elf.get_section_by_name(".dynamic")
    if section is None:
        return

    section_header(".dynamic")
    for tag in section.iter_tags():
        print(tag)


def print_got(elf, name):
    import struct

    section = elf.get_section_by_name(name)
    if section is None:
        return

    section_header(name)
    base_ip = section['sh_addr']

    data = section.data()
    n = section.data_size // 8
    for i in range(n):
        addr = struct.unpack_from("Q", data, i*8)[0]
        print(f"[{i:2x}]: {base_ip+i*8:6x} {addr:16x}")


def print_plt(elf):
    from iced_x86 import Decoder, Formatter, FormatterSyntax

    section_header(".plt")

    section = elf.get_section_by_name(".plt")
    n = section.data_size // 16
    data = section.data()

    frm = Formatter(FormatterSyntax.NASM)
    frm.digit_separator = "'"
    frm.first_operand_char_index = 8
    frm.hex_prefix = "0x"
    frm.hex_suffix = ""
    frm.leading_zeros = False
    frm.rip_relative_addresses = False
    frm.small_hex_numbers_in_decimal = False
    frm.uppercase_hex = False

    base_ip = section['sh_addr']

    for i in range(n):
        entry = data[ i*16 : (i+1)*16 ]
        d = Decoder(64, entry, ip=base_ip + i*16)

        indent = f"[{i:2x}]:"
        for instr in d:
            disasm = frm.format(instr)
            print(f"{indent:6} {instr.ip:6x} {disasm}")
            indent = ""

        print()


def print_gnu_hash(elf):
    hash_section = elf.get_section_by_name(".gnu.hash")
    data = hash_section.data()

    import struct
    (nbuckets, symoff, bloom_sz, bloom_sh) = struct.unpack_from("IIII", data, 0)
    blooms = struct.unpack_from(f"{bloom_sz}Q", data, 16)
    buckets = struct.unpack_from(f"{nbuckets}I", data, 16+(bloom_sz*8))

    p = 16 + (bloom_sz*8) + (nbuckets*4)
    n = (len(data) - p) // 4
    chains = struct.unpack_from(f"{n}I", data, p)

    section_header(".gnu.hash")
    print(f" Bucket Count: {nbuckets}")
    print(f"Symbol Offset: {symoff}")
    print(f"      Buckets: {buckets}")

    print("\n  Bloom words:")
    for i in range(len(blooms)):
        print(f"          [{i:2}]: {blooms[i]:016x}")

    print("\n       Hashes:")
    for i in range(len(chains)):
        h = chains[i]
        end = ""
        if (h & 1) == 1:
            end = "END"
        bloom_idx = (h>>6) % bloom_sz
        bloom_msk = ((1<<(h%64)) | (1<<((h>>bloom_sh)%64)))
        print(f"          [{i+symoff:2}]: {h:08x} {end:5} {bloom_idx:2}/{bloom_msk:016x}")


def print_tables(filename):
    from elftools.elf.elffile import ELFFile

    print(filename)
    print("=" * 50)

    with open(filename, 'rb') as f:
        elf = ELFFile(f)
        print_got(elf, ".got")
        print_got(elf, ".got.plt")
        print_plt(elf)
        print_dyn(elf)
        print_gnu_hash(elf)

    print()


if __name__ == "__main__":
    import sys
    for filename in sys.argv[1:]:
        print_tables(filename)
