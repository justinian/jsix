#!/usr/bin/env python3

class block:
    def __init__(self, start, order, free=False):
        self.start = start
        self.order = order
        self.free = free

    @property
    def end(self):
        return self.start + (1<<self.order)

    def overlaps(self, other):
        return other.start < self.end and other.end > self.start

    def __str__(self):
        return f"[{self.start:016x} {self.order:2} {self.free and 'free' or 'used'}]"

def get_block(blocks, addr, order, reason):
    b = blocks.get(addr)
    if b is None:
        print(f"ERROR: {reason} unknown block [{addr:016x}, {order}]")
    elif b.order != order:
        print(f"ERROR: {reason} block {b} for order {order}")
    else:
        return b
    return None
 
def new_block(blocks, addr, order):
    b = block(addr, order)
    for existing in blocks.values():
        if b.overlaps(existing):
            print(f"ERROR: new block {b} overlaps existing {existing}")
    blocks[addr] = b

def free_block(blocks, addr, order, free):
    s = free and "freeing" or "popping"
    b = get_block(blocks, addr, order, s)
    if b and b.free == free:
        print(f"ERROR: {s} block {b}")
    elif b:
        b.free = free

def split_block(blocks, addr, order):
    b = get_block(blocks, addr, order+1, "splitting")
    if b:
        b.order = order
        buddy = b.start ^ (1<<order)
        blocks[buddy] = block(buddy, order)

def merge_blocks(blocks, addr1, addr2, order):
    b1 = get_block(blocks, addr1, order, "merging")
    b2 = get_block(blocks, addr2, order, "merging")
    if b1.start > b2.start:
        b1, b2 = b2, b1
    del blocks[b2.start]
    b1.order = order + 1

def parse_line(blocks, line):
    args = line.strip().split()
    match args:
        case ['N', addr, order]:
            new_block(blocks, int(addr, base=16), int(order))
        case ['n', addr, order]:
            new_block(blocks, int(addr, base=16), int(order))
        case ['P', addr, order]:
            free_block(blocks, int(addr, base=16), int(order), False)
        case ['p', addr, order]:
            free_block(blocks, int(addr, base=16), int(order), False)
        case ['F', addr, order]:
            free_block(blocks, int(addr, base=16), int(order), True)
        case ['S', addr, order]:
            split_block(blocks, int(addr, base=16), int(order))
        case ['M', addr1, addr2, order]:
            merge_blocks(blocks, int(addr1, base=16), int(addr2, base=16), int(order))
        case _:
            pass

def parse_file(f):
    blocks = {}
    for line in f.readlines():
        parse_line(blocks, line)

    #for addr in sorted(blocks.keys()):
    #    print(f"{addr:09x}: {blocks[addr]}")

if __name__ == "__main__":
    import sys
    if len(sys.argv) > 1:
        with open(sys.argv[1]) as f:
            parse_file(f)
    else:
        parse_file(sys.stdin)
