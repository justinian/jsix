#include "test_case.h"

namespace test {

util::vector<fixture*> *registry::m_tests = nullptr;

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
    if (!m_tests)
        m_tests = new util::vector<fixture*>;
    m_tests->append(&test);
}

size_t
registry::run_all_tests()
{
    if (!m_tests)
        return 0;

    size_t failures = 0;
    for (auto *test : *m_tests) {
        test->test_execute();
        failures += test->_test_failure_count;
    }
    return failures;
}

} // namespace test
