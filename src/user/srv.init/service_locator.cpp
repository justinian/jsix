#include <unordered_map>

#include <j6/errors.h>
#include <j6/flags.h>
#include <j6/types.h>
#include <j6/protocols/service_locator.h>
#include <j6/syscalls.h>
#include <util/node_map.h>

#include "service_locator.h"

struct handle_entry
{
    uint64_t protocol;
    j6_handle_t handle;
};

uint64_t & get_map_key(handle_entry &e) { return e.protocol; }

void
service_locator_start(j6_handle_t mb)
{
    // TODO: This should be a multimap
    util::node_map<uint64_t, handle_entry> services;

    uint64_t tag;
    uint64_t subtag;
    uint16_t reply_tag;

    j6_handle_t handles[10];
    size_t handles_count = sizeof(handles)/sizeof(j6_handle_t);

    j6_status_t s = j6_mailbox_receive(mb,
            &tag, &subtag,
            handles, &handles_count,
            &reply_tag,
            j6_mailbox_block);

    uint64_t proto_id;

    while (true) {
        handle_entry *found = nullptr;

        switch (tag) {
        case j6_proto_base_get_proto_id:
            tag = j6_proto_base_proto_id;
            subtag = j6_proto_sl_id;
            handles_count = 0;
            break;

        case j6_proto_sl_register:
            proto_id = subtag;
            if (handles_count != 1) {
                tag = j6_proto_base_status;
                subtag = j6_err_invalid_arg;
                handles_count = 0;
                break;
            }

            services.insert( {proto_id, handles[0]} );
            tag = j6_proto_base_status;
            subtag = j6_status_ok;
            handles_count = 0;
            break;

        case j6_proto_sl_find:
            proto_id = subtag;
            tag = j6_proto_sl_result;

            found = services.find(proto_id);
            if (found) {
                handles_count = 1;
                handles[0] = found->handle;
            } else {
                handles_count = 0;
            }
            break;

        default:
            tag = j6_proto_base_status;
            subtag = j6_err_invalid_arg;
            handles_count = 0;
            break;
        }

        size_t handles_in = handles_count;
        handles_count = sizeof(handles)/sizeof(j6_handle_t);

        s = j6_mailbox_respond_receive(mb,
                &tag, &subtag,
                handles, &handles_count, handles_in,
                &reply_tag,
                j6_mailbox_block);
    }
}
