#include <chrono>
#include <iostream>
#include <limits>
#include <random>
#include <vector>
#include "kutil/linked_list.h"
#include "catch.hpp"

using namespace kutil;

const int test_list_size = 100;

struct unsortableT {
	int value;
};

struct sortableT {
	int value;
	int compare(const sortableT *other) {
		return value - other->value;
	}
};

template <typename T>
class ListVectorCompare :
	public Catch::MatcherBase<std::vector<list_node<T>>>
{
public:
	using item = list_node<T>;
	using vector = std::vector<item>;

	ListVectorCompare(const linked_list<T> &list, bool reversed) :
		m_list(list), m_reverse(reversed) {}

	virtual bool match (vector const& vec) const override
	{
		size_t index = m_reverse ? vec.size() - 1 : 0;
		for (const T &i : m_list) {
			if (&i != &vec[index]) return false;
			index += m_reverse ? -1 : 1;
		}
		return true;
	}

	virtual std::string describe() const override
	{
		return "is the same as the given linked list";
	}

private:
	const linked_list<T> &m_list;
	bool m_reverse;
};

template <typename T>
class IsSorted :
	public Catch::MatcherBase<linked_list<T>>
{
public:
	using item = list_node<T>;
	using list = linked_list<T>;

	IsSorted() {}

	virtual bool match (list const& l) const override
	{
		int big = std::numeric_limits<int>::min();
		for (const T &i : l) {
			if (i.value < big) return false;
			big = i.value;
		}
		return true;
	}

	virtual std::string describe() const override
	{
		return "is sorted";
	}
};

template <typename T>
ListVectorCompare<T> IsSameAsList(const linked_list<T> &list, bool reversed = false)
{
	return ListVectorCompare<T>(list, reversed);
}

TEST_CASE( "Linked list tests", "[containers list]" )
{
	using clock = std::chrono::system_clock;
	unsigned seed = clock::now().time_since_epoch().count();
	std::default_random_engine rng(seed);
	std::uniform_int_distribution<int> gen(1, 1000);

	linked_list<unsortableT> ulist;

	std::vector<list_node<unsortableT>> unsortables(test_list_size);
	for (auto &i : unsortables) {
		i.value = gen(rng);
		ulist.push_back(&i);
	}
	CHECK( ulist.length() == test_list_size );
	CHECK_THAT( unsortables, IsSameAsList(ulist) );

	linked_list<unsortableT> ulist_reversed;

	for (auto &i : unsortables) {
		i.remove();
		ulist_reversed.push_front(&i);
	}

	CHECK( ulist_reversed.length() == test_list_size );
	CHECK_THAT( unsortables, IsSameAsList(ulist_reversed, true) );

	linked_list<sortableT> slist;

	std::vector<list_node<sortableT>> sortables(test_list_size);
	for (auto &i : sortables) {
		i.value = gen(rng);
		slist.sorted_insert(&i);
	}
	CHECK( slist.length() == test_list_size );
	CHECK_THAT( slist, IsSorted<sortableT>() );
}
