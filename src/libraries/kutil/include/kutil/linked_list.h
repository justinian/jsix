#pragma once
/// \file linked_list.h
/// A generic templatized linked list.
#include <stddef.h>

namespace kutil {

template <typename T> class linked_list;


/// A list node in a `linked_list<T>` or `sortable_linked_list<T>`.
template <typename T>
class list_node :
	public T
{
public:
	using item_type = T;
	using node_type = list_node<T>;

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

	/// Accessor for the next pointer.
	/// \returns  The next node in the list
	inline node_type * next() { return m_next; }

	/// Accessor for the next pointer.
	/// \returns  The next node in the list
	inline const node_type * next() const { return m_next; }

	/// Accessor for the prev pointer.
	/// \returns  The prev node in the list
	inline node_type * prev() { return m_prev; }

	/// Accessor for the prev pointer.
	/// \returns  The prev node in the list
	inline const node_type * prev() const { return m_prev; }

private:
	friend class linked_list<T>;

	/// Insert an item after this one in the list.
	/// \arg item  The item to insert
	void insert_after(node_type *item)
	{
		if (m_next) m_next->m_prev = item;
		item->m_next = m_next;
		item->m_prev = this;
		m_next = item;
	}

	/// Insert an item before this one in the list.
	/// \arg item  The item to insert
	void insert_before(node_type *item)
	{
		if (m_prev) m_prev->m_next = item;
		item->m_prev = m_prev;
		item->m_next = this;
		m_prev = item;
	}

	/// Remove this item from its list.
	void remove()
	{
		if (m_next) m_next->m_prev = m_prev;
		if (m_prev) m_prev->m_next = m_next;
		m_next = m_prev = nullptr;
	}

	node_type *m_next;
	node_type *m_prev;
};


/// An iterator for linked lists
template <typename T>
class list_iterator
{
public:
	using item_type = list_node<T>;

	list_iterator(item_type *item) : m_item(item) {}

	inline item_type * operator*() { return m_item; }
	inline const item_type * operator*() const { return m_item; }
	inline list_iterator & operator++() { m_item = m_item ? m_item->next() : nullptr; return *this; }
	inline list_iterator operator++(int) { return list_iterator<T>(m_item ? m_item->next() : nullptr); }
	inline bool operator!=(const list_iterator<T> &other) { return m_item != other.m_item; }

private:
	item_type *m_item;
};


/// A templatized doubly-linked list container of `list_node<T>` items.
template <typename T>
class linked_list
{
public:
	using item_type = list_node<T>;
	using iterator = list_iterator<T>;

	/// Constructor. Creates an empty list.
	linked_list() :
		m_head(nullptr),
		m_tail(nullptr)
	{}

	/// Move constructor. Takes ownership of list elements.
	linked_list(linked_list<T> &&other) :
		m_head(other.m_head),
		m_tail(other.m_tail)
	{
		other.m_head = other.m_tail = nullptr;
	}

	/// Check if the list is empty.
	/// \returns  true if the list is empty
	bool empty() const { return m_head == nullptr; }

	/// Count the items in the list.
	/// \returns  The number of entries in the list.
	size_t length() const
	{
		size_t len = 0;
		for (item_type *cur = m_head; cur; cur = cur->m_next) ++len;
		return len;
	}

	/// Get the item at the front of the list, without removing it
	/// \returns  The first item in the list
	inline item_type * front() { return m_head; }

	/// Get the item at the back of the list, without removing it
	/// \returns  The last item in the list
	inline item_type * back() { return m_tail; }

	/// Prepend an item to the front of this list.
	/// \arg item  The node to insert.
	void push_front(item_type *item)
	{
		if (!item)
			return;

		if (!m_head) {
			m_head = m_tail = item;
			item->m_next = item->m_prev = nullptr;
		} else {
			m_head->m_prev = item;
			item->m_next = m_head;
			item->m_prev = nullptr;
			m_head = item;
		}
	}

	/// Append an item to the end of this list.
	/// \arg item  The node to append.
	void push_back(item_type *item)
	{
		if (!item)
			return;

		if (!m_tail) {
			m_head = m_tail = item;
			item->m_next = item->m_prev = nullptr;
		} else {
			m_tail->m_next = item;
			item->m_prev = m_tail;
			item->m_next = nullptr;
			m_tail = item;
		}
	}

	/// Remove an item from the front of this list.
	/// \returns  The node that was removed
	item_type * pop_front()
	{
		item_type *item = m_head;
		remove(item);
		return item;
	}

	/// Remove an item from the end of this list.
	/// \returns  The node that was removed
	item_type * pop_back()
	{
		item_type *item = m_tail;
		remove(item);
		return item;
	}

	/// Append the contents of another list to the end of this list. The other
	/// list is emptied, and this list takes ownership of its items.
	/// \arg list  The other list.
	void append(linked_list<T> &list)
	{
		if (!list.m_head) return;

		if (!m_tail) {
			m_head = list.m_head;
			m_tail = list.m_tail;
		} else {
			m_tail->m_next = list.m_head;
			m_tail = list.m_tail;
		}

		list.m_head = list.m_tail = nullptr;
	}

	/// Append the contents of another list to the end of this list. The other
	/// list is emptied, and this list takes ownership of its items.
	/// \arg list  The other list.
	void append(linked_list<T> &&list)
	{
		if (!list.m_head) return;

		if (!m_tail) {
			m_head = list.m_head;
			m_tail = list.m_tail;
		} else {
			m_tail->m_next = list.m_head;
			m_tail = list.m_tail;
		}

		list.m_head = list.m_tail = nullptr;
	}

	/// Remove an item from the list.
	/// \arg item  The item to remove
	void remove(item_type *item)
	{
		if (!item) return;
		if (item == m_head)
			m_head = item->m_next;
		if (item == m_tail)
			m_tail = item->m_prev;
		item->remove();
	}

	/// Inserts an item into the list before another given item.
	/// \arg existing The existing item to insert before
	/// \arg item     The new item to insert
	void insert_before(item_type *existing, item_type *item)
	{
		if (!item) return;

		if (!existing)
			push_back(item);
		else if (existing == m_head)
			push_front(item);
		else
			existing->insert_before(item);
	}

	/// Inserts an item into the list after another given item.
	/// \arg existing The existing item to insert after
	/// \arg item     The new item to insert
	void insert_after(item_type *existing, item_type *item)
	{
		if (!item) return;

		if (!existing)
			push_front(item);
		else if (existing == m_tail)
			push_back(item);
		else
			existing->insert_after(item);
	}

	/// Insert an item into the list in a sorted position. Depends on T
	/// having a method `int compare(const T *other)`.
	/// \arg item  The item to insert
	void sorted_insert(item_type *item)
	{
		if (!item) return;

		item_type *cur = m_head;
		while (cur && item->compare(cur) > 0)
			cur = cur->m_next;

		insert_before(cur, item);
	}

	/// Range-based for iterator generator.
	/// \returns  An iterator to the beginning of the list
	inline iterator begin() { return iterator(m_head); }

	/// Range-based for iterator generator.
	/// \returns  A const iterator to the beginning of the list
	inline const iterator begin() const { return iterator(m_head); }

	/// Range-based for end-iterator generator.
	/// \returns  An iterator to the end of the list
	inline const iterator end() const { return iterator(nullptr); }

private:
	item_type *m_head;
	item_type *m_tail;
};


} // namespace kutil
