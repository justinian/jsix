#include <limits>
#include <vector>

#include <util/linked_list.h>

#include "container_helpers.h"
#include "test_case.h"
#include "test_rng.h"

const int test_list_size = 100;

struct linked_list_tests :
    public test::fixture
{
};

template <typename T>
class list_vector_comparator :
    public test::comparator<std::vector<util::list_node<T>> const&>
{
public:
    using item = util::list_node<T>;
    using vector = std::vector<item>;

    list_vector_comparator(const util::linked_list<T> &list, bool reversed) :
        m_list(list), m_reverse(reversed) {}

    virtual bool operator()(vector const& vec) const override
    {
        size_t index = m_reverse ? vec.size() - 1 : 0;
        for (const T *i : m_list) {
            if (i != &vec[index]) return false;
            index += m_reverse ? -1 : 1;
        }
        return true;
    }

private:
    const util::linked_list<T> &m_list;
    bool m_reverse;
};

template <typename T>
class is_sorted :
    public test::comparator<util::linked_list<T> const&>
{
public:
    using item = util::list_node<T>;
    using list = util::linked_list<T>;

    is_sorted() {}

    virtual bool operator()(list const& l) const override
    {
        int big = std::numeric_limits<int>::min();
        for (const T *i : l) {
            if (i->value < big) return false;
            big = i->value;
        }
        return true;
    }
};

template <typename T>
class list_contains_comparator :
    public test::comparator<util::linked_list<T>>
{
public:
    using item = util::list_node<T>;
    using list = util::linked_list<T>;

    list_contains_comparator(const item &needle) : m_needle(needle) {}

    virtual bool operator()(list const& l) const override
    {
        for (const T *i : l)
            if (i == &m_needle) return true;
        return false;
    }

    const item &m_needle;
};

template <typename T>
list_vector_comparator<T> same_as(const util::linked_list<T> &list, bool reversed = false)
{
    return list_vector_comparator<T>(list, reversed);
}

template <typename T>
list_contains_comparator<T> contains(const util::list_node<T> &item)
{
    return list_contains_comparator<T>(item);
}

TEST_CASE( linked_list_tests, unsorted )
{
    util::linked_list<unsortableT> ulist;

    int value = 0;
    std::vector<util::list_node<unsortableT>> unsortables(test_list_size);
    for (auto &i : unsortables) {
        i.value = value++;
        ulist.push_back(&i);
    }
    CHECK_BARE( ulist.length() == test_list_size );
    CHECK_THAT( unsortables, same_as(ulist) );

    util::linked_list<unsortableT> ulist_reversed;

    for (auto &i : unsortables) {
        ulist.remove(&i);
        ulist_reversed.push_front(&i);
    }

    CHECK_BARE( ulist_reversed.length() == test_list_size );

    //CHECK_THAT( unsortables, IsSameAsList(ulist_reversed, true) );

    auto &removed = unsortables[test_list_size / 2];
    ulist_reversed.remove(&removed);
    CHECK( ulist_reversed.length() == test_list_size - 1,
            "remove() did not make size 1 less" );

    CHECK_THAT( ulist_reversed, !contains(removed) );
}

TEST_CASE( linked_list_tests, sorted )
{
    test::rng rng {12345};
    std::uniform_int_distribution<int> gen(1, 1000);

    util::linked_list<sortableT> slist;
    CHECK( slist.length() == 0, "Newly constructed list should be empty" );

    std::vector<util::list_node<sortableT>> sortables(test_list_size);
    for (auto &i : sortables) {
        i.value = gen(rng);
        slist.sorted_insert(&i);
    }
    CHECK_BARE( slist.length() == test_list_size );
    // CHECK_THAT( slist, is_sorted<sortableT>() );
}
