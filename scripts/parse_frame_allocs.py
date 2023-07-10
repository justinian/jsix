#!/usr/bin/env python3

def add_maps(allocs, addr, count):
    for i in range(count):
        if addr+i in allocs:
            print(f"ERROR: frame {addr+i:012x} map collision.")
        else:
            #print(f"       frame {addr+i:012x} mapped")
            allocs.add(addr+i)

def remove_maps(allocs, addr, count):
    for i in range(count):
        if addr+i not in allocs:
            print(f" WARN: removing unmapped frame {addr+i:012x}")
        else:
            #print(f"       frame {addr+i:012x} unmapped")
            allocs.remove(addr+i)

def parse_line(allocs, line):
    args = line.strip().split()
    match args:
        case ['+', addr, count]:
            add_maps(allocs, int(addr, base=16), int(count))
        case ['-', addr, count]:
            remove_maps(allocs, int(addr, base=16), int(count))
        case _:
            pass

def parse_file(f):
    allocs = set()
    for line in f.readlines():
        parse_line(allocs, line)

if __name__ == "__main__":
    import sys
    if len(sys.argv) > 1:
        with open(sys.argv[1]) as f:
            parse_file(f)
    else:
        parse_file(sys.stdin)
