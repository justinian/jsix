#pragma once

struct unsortableT {
	int value;
};

struct sortableT {
	int value;
	int compare(const sortableT *other) const {
		return value - other->value;
	}
};

