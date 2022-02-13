#pragma once
/// \file test_case.h
/// Test case definition and helpers

#include <stddef.h>
#include <util/vector.h>

namespace test {

class fixture
{
public:
    virtual void setup() {}
    virtual void teardown() {}
    virtual void test_execute() = 0;

protected:
    void _log_failure(
            const char *test_name,
            const char *message,
            const char *function = __builtin_FUNCTION(),
            const char *file = __builtin_FILE(),
            uint64_t line = __builtin_LINE());

private:
    friend class registry;
    size_t _test_failure_count = 0;
};

class registry
{
public:
    static void register_test_case(fixture &test);
    static size_t run_all_tests();

private:
    static util::vector<fixture*> m_tests;
};

template <typename T>
class registrar
{
public:
    registrar() { registry::register_test_case(m_test); }

private:
    T m_test;
};

template <typename... Args>
struct comparator_negate;

template <typename... Args>
struct comparator
{
    virtual bool operator()(const Args&... opts) const = 0;
    virtual comparator_negate<Args...> operator!() const { return comparator_negate<Args...> {*this}; }
};

template <typename... Args>
struct comparator_negate :
    public comparator<Args...>
{
    comparator_negate(const comparator<Args...> &in) : inner {in} {}
    virtual bool operator()(const Args&... opts) const override { return !inner(opts...); }
    const comparator<Args...> &inner;
};

} // namespace test

#define TEST_CASE(fixture, name) namespace {                        \
    struct test_case_ ## name : fixture {                           \
        static constexpr const char *test_name = #fixture ":" #name;\
        void test_execute();                                        \
    };                                                              \
    test::registrar<test_case_ ## name> name ## _auto_registrar;    \
}                                                                   \
void test_case_ ## name::test_execute()

#define CHECK(expr, message) do { if (!(expr)) {_log_failure(test_name,message);} } while (0)
#define REQUIRE(expr, message) do { if (!(expr)) {_log_failure(test_name,message); return;} } while (0)
#define CHECK_BARE(expr) do { if (!(expr)) {_log_failure(test_name,#expr);} } while (0)
#define CHECK_THAT(subject, checker) do { if (!(checker)(subject)) {_log_failure(test_name,#subject #checker);} } while (0)
