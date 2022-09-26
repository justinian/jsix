#include <j6/cap_flags.h>
#include <j6/errors.h>
#include <j6/syscalls.h>
#include <j6/types.h>
#include "test_case.h"

extern j6_handle_t __handle_self;

struct handle_tests :
    public test::fixture
{
};

TEST_CASE( handle_tests, clone_cap )
{
    j6_status_t s;
    j6_handle_t self0 = __handle_self;

    j6_handle_t self1 = j6_handle_invalid;
    s = j6_handle_clone(self0, &self1, j6_handle_caps(self0) & ~j6_cap_object_clone);
    CHECK( s == j6_status_ok, "Cloning self handle" );

    j6_handle_t self2 = j6_handle_invalid;
    s = j6_handle_clone(self1, &self2, j6_handle_caps(self1));
    CHECK( s == j6_err_denied, "Cloning non-clonable handle" );
}
