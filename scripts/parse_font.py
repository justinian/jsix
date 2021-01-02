#!/usr/bin/env python3

from fontpsf import PSF2

def print_glyph(header, data):
    bw = (header.width + 7) // 8
    fmt = "{:08b}" * bw
    for y in range(header.height):
        b = data[y*bw:(y+1)*bw]
        s = fmt.format(*b).replace("0", " ").replace("1", "*")
        print(s)
    print("")


def display_font(filename):
    font = PSF2.load(filename)
    print(font.header)

    for glyph in font:
        if glyph.empty():
            print("{}: BLANK".format(glyph.description()))
        else:
            print("{}:".format(glyph.description()))
            print_glyph(font.header, glyph.data)


if __name__ == "__main__":
    import sys
    for filename in sys.argv[1:]:
        display_font(filename)
