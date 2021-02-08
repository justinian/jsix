/// \file spinlock.h
/// Spinlock types and related defintions

#pragma once

namespace kutil {
namespace spinlock {

/// An MCS based spinlock node
struct node
{
	bool locked;
	node *next;
};

void aquire(node *lock, node *waiter);
void release(node *lock, node *waiter);

} // namespace spinlock
} // namespace kutil
