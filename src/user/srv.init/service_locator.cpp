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
    std::unordered_map<uint64_t, j6_handle_t> services;

    uint64_t tag = 0;
    uint64_t subtag = 0;
    uint64_t reply_tag = 0;

    j6_handle_t give_handle = j6_handle_invalid;
    uint64_t proto_id;

    while (true) {
        j6_status_t s = j6_mailbox_respond(mb, &tag, &subtag, &give_handle,
                &reply_tag, j6_flag_block);

        if (s != j6_status_ok)
            while (1);

        handle_entry *found = nullptr;

        switch (tag) {
        case j6_proto_base_get_proto_id:
            tag = j6_proto_base_proto_id;
            subtag = j6_proto_sl_id;
            give_handle = j6_handle_invalid;
            break;

        case j6_proto_sl_register:
            proto_id = subtag;
            if (give_handle == j6_handle_invalid) {
                tag = j6_proto_base_status;
                subtag = j6_err_invalid_arg;
                break;
            }

            services.insert( {proto_id, give_handle} );
            tag = j6_proto_base_status;
            subtag = j6_status_ok;
            give_handle = j6_handle_invalid;
            break;

        case j6_proto_sl_find:
            proto_id = subtag;
            tag = j6_proto_sl_result;

            {
                auto found = services.find(proto_id);
                if (found != services.end())
                    give_handle = found->second;
                else
                    give_handle = j6_handle_invalid;
            }
            break;

        default:
            tag = j6_proto_base_status;
            subtag = j6_err_invalid_arg;
            give_handle = j6_handle_invalid;
            break;
        }
    }
}
