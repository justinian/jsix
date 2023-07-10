#!/usr/bin/env python3

from bisect import bisect_left, insort
from operator import attrgetter

by_start = attrgetter('start')

class alloc:
    def __init__(self, start, size):
        self.start = start
        self.size = size

    @property
    def end(self):
        return self.start + self.size

    def overlaps(self, other):
        return other.start < self.end and other.end > self.start

    def __str__(self):
        return f"[{self.start:012x} - {self.end:012x}]"

    def __gt__(self, other):
        return self.start > other.start

def add_alloc(allocs, addr, length):
    a = alloc(addr, length)
    for existing in allocs:
        if a.overlaps(existing):
            print(f"ERROR: allocation {a} overlaps existing {existing}")
    insort(allocs, a)

def remove_alloc(allocs, addr):
    a = alloc(addr, 0)
    i = bisect_left(allocs, a)
    if len(allocs) > i and allocs[i].start == addr:
        del allocs[i]
    else:
        print(f"ERROR: freeing unallocated {a}")

def parse_line(allocs, line):
    args = line.strip().split()
    match args:
        case ['+', addr, length]:
            add_alloc(allocs, int(addr, base=16), int(length))
        case ['-', addr]:
            remove_alloc(allocs, int(addr, base=16))
        case _:
            pass

def parse_file(f):
    allocs = []
    for line in f.readlines():
        parse_line(allocs, line)

if __name__ == "__main__":
    import sys
    if len(sys.argv) > 1:
        with open(sys.argv[1]) as f:
            parse_file(f)
    else:
        parse_file(sys.stdin)
