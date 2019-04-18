#!/usr/bin/env python3

def parse_elf(filename):
    import struct
    with open(filename, 'rb') as elf:

if __name__ == "__main__":
    import sys
    for arg in sys.argv[1:]:
        parse_elf(arg)
