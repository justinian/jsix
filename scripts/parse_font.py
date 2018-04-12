#!/usr/bin/env python3

MAGIC = (0x72, 0xb5, 0x4a, 0x86)

from collections import namedtuple
PSF2 = namedtuple("PSF2", ["version", "offset", "flags", "count", "charsize", "height", "width"])

def read_header(data):
    from struct import unpack_from, calcsize

    fmt = "BBBBIIIIIII"

    values = unpack_from(fmt, data)
    if values[:len(MAGIC)] != MAGIC:
        raise Exception("Bad magic number in header")

    return PSF2(*values[len(MAGIC):])


def print_glyph(header, data):
    bw = (header.width + 7) // 8
    fmt = "{:08b}" * bw
    for y in range(header.height):
        b = data[y*bw:(y+1)*bw]
        s = fmt.format(*b).replace("0", " ").replace("1", "*")
        print(s)
    print("")


def display_font(filename):
    data = open(filename, 'rb').read()

    header = read_header(data)
    print(header)

    c = header.charsize
    for i in range(0, header.count):
        n = i * c + header.offset
        print("Glyph {}:".format(i))
        print_glyph(header, data[n:n+c])


if __name__ == "__main__":
    import sys
    for filename in sys.argv[1:]:
        display_font(filename)
