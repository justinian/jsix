#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <j6/errors.h>
#include <j6/flags.h>
#include <j6/thread.hh>
#include <j6/types.h>
#include <j6/syscalls.h>

#include "test_case.h"
#include "test_rng.h"

struct mailbox_tests :
    public test::fixture
{
};

static constexpr uintptr_t caller_stack = 0x4000000;
static j6_handle_t test_mailbox = j6_handle_invalid;

TEST_CASE( mailbox_tests, would_block )
{
    return;
    j6_handle_t mb = j6_handle_invalid;
    j6_status_t s;

    s = j6_mailbox_create(&mb);
    CHECK( s == j6_status_ok, "Could not create a mailbox" );

    uint64_t tag = 12345;
    uint64_t subtag = 67890;
    j6_handle_t handle;
    uint64_t reply_tag = 0;
    uint64_t flags = 0;

    s = j6_mailbox_respond( mb, &tag, &subtag, &handle, &reply_tag, flags );
    CHECK( s == j6_status_would_block, "Should have gotten would block error" );

    j6_mailbox_close(mb);
}

void
caller_proc(void *)
{
    j6_status_t s;

    uint64_t tag = 12345;
    uint64_t subtag = 67890;
    j6_handle_t no_handle = j6_handle_invalid;

    j6_mailbox_call( test_mailbox, &tag, &subtag, &no_handle );

    volatile int i = 0;
    while (i != 0);
}

TEST_CASE( mailbox_tests, send_receive )
{
    j6_status_t s;

    s = j6_mailbox_create(&test_mailbox);
    CHECK( s == j6_status_ok, "Could not create a mailbox" );

    j6::thread caller {caller_proc, caller_stack};
    s = caller.start();
    CHECK( s == j6_status_ok, "Could not start mailbox caller thread" );

    uint64_t tag = 0;
    uint64_t subtag = 0;
    uint64_t reply_tag = 0;
    j6_handle_t mb_handle = j6_handle_invalid;

    s = j6_mailbox_respond( test_mailbox, &tag, &subtag, &mb_handle, &reply_tag, j6_mailbox_block );
    CHECK( s == j6_status_ok, "Did not respond successfully" );

    CHECK_BARE( tag == 12345 );
    CHECK_BARE( subtag == 67890 );

    j6_mailbox_close(test_mailbox);
    caller.join();
}
