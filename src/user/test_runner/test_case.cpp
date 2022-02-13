#include "test_case.h"

namespace test {

util::vector<fixture*> registry::m_tests;

void
fixture::_log_failure(const char *test_name, const char *message,
            const char *function, const char *file, uint64_t line)
{
    // TODO: output results
    ++_test_failure_count;
}

void
registry::register_test_case(fixture &test)
{
    m_tests.append(&test);
}

size_t
registry::run_all_tests()
{
    size_t failures = 0;
    for (auto *test : m_tests) {
        test->test_execute();
        failures += test->_test_failure_count;
    }
    return failures;
}

} // namespace test
