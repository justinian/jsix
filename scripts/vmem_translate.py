#!/usr/bin/env python

def translate(i4 = 0, i3 = 0, i2 = 0, i1 = 0, offset = 0):
    addr = (i4 << 39) + (i3 << 30) + (i2 << 21) + (i1 << 12) + offset
    if addr & (1 << 47):
        addr |= 0xffff000000000000
    return addr

if __name__ == "__main__":
    import sys
    print("{:016x}".format(translate(*map(int, sys.argv[1:]))))

