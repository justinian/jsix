#include <stdlib.h>
#include <j6/syscalls.h>

#include "test_case.h"

extern "C"
int main()
{
    size_t failures = test::registry::run_all_tests();
    j6_test_finish(failures); // never actually returns
    return 0;
}
