#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <j6/errors.h>
#include <j6/flags.h>
#include <j6/types.h>
#include <j6/syscalls.h>

#include "test_case.h"
#include "test_rng.h"

struct mailbox_tests :
    public test::fixture
{
};

TEST_CASE( mailbox_tests, would_block )
{
    j6_handle_t mb = j6_handle_invalid;
    j6_status_t s;

    s = j6_mailbox_create(&mb);
    CHECK( s == j6_status_ok, "Could not create a mailbox" );

    uint64_t tag = 12345;
    uint8_t buffer[128];
    size_t buffer_size = 128;
    j6_handle_t handles[10];
    size_t handle_count = 10;
    uint16_t reply_tag = 0;
    uint64_t badge = 0;
    uint64_t flags = 0;

    s = j6_mailbox_receive( mb, &tag,
            buffer, &buffer_size,
            handles, &handle_count,
            &reply_tag, &badge,
            flags );
    CHECK( s == j6_status_would_block, "Should have gotten would block error" );

    j6_mailbox_close(mb);
}

TEST_CASE( mailbox_tests, send_receive )
{
    j6_handle_t mb = j6_handle_invalid;
    j6_status_t s;

    s = j6_mailbox_create(&mb);
    CHECK( s == j6_status_ok, "Could not create a mailbox" );

    uint64_t out_tag = 12345;
    uint8_t out_buffer[128] = {2, 4, 6, 8, 10, 12, 14};
    size_t out_buffer_size = 7;
    j6_handle_t out_handles[10];
    size_t out_handle_count = 0;

    s = j6_mailbox_send( mb, out_tag,
            out_buffer, out_buffer_size,
            out_handles, out_handle_count );
    CHECK( s == j6_status_ok, "Did not send successfully" );

    uint64_t in_tag = 0;
    uint8_t in_buffer[128];
    size_t in_buffer_size = 128;
    j6_handle_t in_handles[10];
    size_t in_handle_count = 10;
    uint16_t in_reply_tag = 0;
    uint64_t in_badge = 0;
    uint64_t in_flags = 0;

    s = j6_mailbox_receive( mb, &in_tag,
            in_buffer, &in_buffer_size,
            in_handles, &in_handle_count,
            &in_reply_tag, &in_badge,
            in_flags );
    CHECK( s == j6_status_ok, "Did not receive successfully" );

    CHECK_BARE( in_tag == out_tag );
    CHECK_BARE( in_buffer_size == out_buffer_size );
    CHECK_BARE( in_handle_count == out_handle_count );

    CHECK_BARE( memcmp(out_buffer, in_buffer, in_buffer_size) == 0 );

    j6_mailbox_close(mb);
}
