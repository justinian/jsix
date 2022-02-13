#pragma once

struct unsortableT {
    unsigned long value;
};

struct sortableT {
    unsigned long value;
    unsigned long compare(const sortableT &other) const {
        return value > other.value ? 1 : value == other.value ? 0 : -1;
    }
};

