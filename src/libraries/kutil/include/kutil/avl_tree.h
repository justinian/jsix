#pragma once
/// \file avl_tree.h
/// Templated container class for an AVL tree

#include <stdint.h>
#include "kutil/assert.h"

namespace kutil {

template <typename T> class avl_tree;


/// A node in a `avl_tree<T>`
template <typename T>
class avl_node :
	public T
{
public:
	using item_type = T;
	using node_type = avl_node<T>;

	/// Dereference operator. Helper to cast this node to the contained type.
	/// \returns  A pointer to the node, cast to T*.
	inline item_type & operator*() { return *this; }

	/// Dereference operator. Helper to cast this node to the contained type.
	/// \returns  A pointer to the node, cast to T*.
	inline const item_type & operator*() const { return *this; }

	/// Cast operator. Helper to cast this node to the contained type.
	/// \returns  A reference to the node, cast to T&.
	inline operator item_type& () { return *this; }

	/// Cast operator. Helper to cast this node to the contained type.
	/// \returns  A reference to the node, cast to const T&.
	inline operator const item_type& () { return *this; }

	/// Helper to cast this node to the contained type.
	/// \returns  A reference to the node, cast to const T&.
	inline const item_type& item() const { return *this; }

	/// Helper to cast this node to the contained type.
	/// \returns  A reference to the node, cast to T&.
	inline item_type& item() { return *this; }

	/// Accessor for the left child.
	/// \returns  A pointer to the left child, or nullptr.
	inline node_type * left() { return m_left; }

	/// Accessor for the left child.
	/// \returns  A pointer to the left child, or nullptr.
	inline const node_type * left() const { return m_left; }

	/// Accessor for the right child.
	/// \returns  A pointer to the right child, or nullptr.
	inline node_type * right() { return m_right; }

	/// Accessor for the right child.
	/// \returns  A pointer to the right child, or nullptr.
	inline const node_type * right() const { return m_right; }

private:
	friend class avl_tree<T>;

	inline static int height(node_type *l) { return (l ? l->m_height : 0); }

	// Update this node's height and return its new balance factor
	inline int update_height()
	{
		int left = height(m_left);
		int right = height(m_right);
		m_height = std::max(left, right) + 1;
		return left - right;
	}

	int bias(node_type *addend)
	{
		const item_type &this_item = *this;
		const item_type &that_item = *addend;
		if (that_item < this_item)
			return -1;
		else if (that_item > this_item)
			return 1;

		kassert(false, "Equal items not allowed in AVL tree");
		return 0;
	}

	static node_type * rotate_right(node_type *existing)
	{
		node_type *root = existing->m_left;
		node_type *left = root->m_right;

		root->m_right = existing;
		existing->m_left = left;

		existing->update_height();
		root->update_height();

		return root;
	}

	static node_type * rotate_left(node_type *existing)
	{
		node_type *root = existing->m_right;
		node_type *right = root->m_left;

		root->m_left = existing;
		existing->m_right = right;

		existing->update_height();
		root->update_height();

		return root;
	}

	static node_type * insert(node_type *existing, node_type *addend)
	{
		if (existing == nullptr)
			return addend;

		if (existing->compare(addend) < 0)
			existing->m_left = insert(existing->m_left, addend);
		else
			existing->m_right = insert(existing->m_right, addend);

		int balance = existing->update_height();
		if (balance > 1) {
			// Left-heavy
			if (existing->m_left->compare(addend) < 0) {
				// Left Left
				return rotate_right(existing);
			} else {
				// Left Right
				existing->m_left = rotate_left(existing->m_left);
				return rotate_right(existing);
			}
		} else if (balance < -1) {
			// Right-heavy
			if (existing->m_right->compare(addend) > 0) {
				// Right Right
				return rotate_left(existing);
			} else {
				// Right Left
				existing->m_right = rotate_right(existing->m_right);
				return rotate_left(existing);
			}
		}

		return existing;
	}

	static node_type * remove(node_type *existing, node_type *subtrahend, allocator &alloc)
	{
		if (existing == nullptr)
			return existing;

		if (existing == subtrahend) {
			if (!existing->m_left || !existing->m_right) {
				// At least one child is null
				node_type *temp = existing->m_left ?
					existing->m_left : existing->m_right;

				if (temp == nullptr) {
					// Both were null
					temp = existing;
					existing = nullptr;
				} else {
					*existing = *temp;
				}

				alloc.free(temp);
			} else {
				// Both children exist, find next node
				node_type *temp = existing->m_right;
				while (temp->m_left)
					temp = temp->m_left;

				*existing = *temp;
				existing->m_right = remove(existing->m_right, temp, alloc);
			}
		} else if (existing->compare(subtrahend) < 0) {
			existing->m_left = remove(existing->m_left, subtrahend, alloc);
		} else {
			existing->m_right = remove(existing->m_right, subtrahend, alloc);
		}

		if (!existing)
			return nullptr;

		int balance = existing->update_height();
		if (balance > 1) {
			int left_balance = existing->m_left->update_height();

			if (left_balance < 0)
				existing->m_left = rotate_left(existing->m_left);

			return rotate_right(existing);
		} else if (balance < -1) {
			int right_balance = existing->m_right->update_height();

			if (right_balance > 0)
				existing->m_right = rotate_right(existing->m_right);

			return rotate_left(existing);
		}

		return existing;
	}

	int m_height;
	node_type *m_left;
	node_type *m_right;
};

template <typename T>
class avl_tree
{
public:
	using item_type = T;
	using node_type = avl_node<T>;

	inline node_type * root() { return m_root; }
	inline unsigned count() const { return m_count; }

	inline void remove(node_type *subtrahend, allocator &alloc) {
		m_root = node_type::remove(m_root, subtrahend, alloc);
		m_count--;
	}

	inline void insert(node_type *addend) {
		m_root = node_type::insert(m_root, addend);
		m_count++;
	}

private:
	unsigned m_count {0};
	node_type *m_root;
};

} // namespace kutil
