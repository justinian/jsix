#include <unordered_map>
#include <stdlib.h>

#include <j6/errors.h>
#include <j6/flags.h>
#include <j6/types.h>
#include <j6/protocols/service_locator.h>
#include <j6/syscalls.h>
#include <j6/syslog.hh>
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
    uint64_t data = 0;
    uint64_t handle_count = 1;
    uint64_t reply_tag = 0;

    j6_handle_t give_handle = j6_handle_invalid;
    uint64_t proto_id;

    j6::syslog("SL> Starting service locator on mbx handle %x", mb);

    while (true) {
        uint64_t data_len = sizeof(uint64_t);
        j6_status_t s = j6_mailbox_respond(mb, &tag,
            &data, &data_len, data_len,
            &give_handle, &handle_count,
            &reply_tag, j6_flag_block);

        if (s != j6_status_ok)
            exit(128);

        handle_entry *found = nullptr;

        switch (tag) {
        case j6_proto_sl_register:
            proto_id = data;
            if (give_handle == j6_handle_invalid) {
                tag = j6_proto_base_status;
                data = j6_err_invalid_arg;
                break;
            }

            j6::syslog("SL> Registering handle %x for proto %x", give_handle, proto_id);

            services.insert( {proto_id, give_handle} );
            tag = j6_proto_base_status;
            data = j6_status_ok;
            give_handle = j6_handle_invalid;
            break;

        case j6_proto_sl_find:
            proto_id = data;
            tag = j6_proto_sl_result;
            data = 0;

            {
                auto found = services.find(proto_id);
                if (found != services.end()) {
                    j6::syslog("SL> Found handle %x for proto %x", give_handle, proto_id);
                    give_handle = found->second;
                } else {
                    j6::syslog("SL> Found no handles for proto %x", proto_id);
                    give_handle = j6_handle_invalid;
                }
            }
            break;

        default:
            tag = j6_proto_base_status;
            data = j6_err_invalid_arg;
            give_handle = j6_handle_invalid;
            break;
        }
    }
}
