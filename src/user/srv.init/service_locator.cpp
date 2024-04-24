#include <stdint.h>
#include <stdlib.h>

#include <j6/errors.h>
#include <j6/flags.h>
#include <j6/types.h>
#include <j6/protocols/service_locator.h>
#include <j6/syscalls.h>
#include <j6/syslog.hh>
#include <util/counted.h>
#include <util/node_map.h>

#include "service_locator.h"

struct handle_entry
{
    uint64_t protocol;
    util::counted<j6_handle_t> handles;
};

uint64_t & get_map_key(handle_entry &e) { return e.protocol; }

void
service_locator_start(j6_handle_t mb)
{
    // TODO: This should be a multimap
    util::node_map<uint64_t, handle_entry> services;

    uint64_t tag = 0;
    uint64_t data = 0;
    uint64_t reply_tag = 0;

    static constexpr size_t max_handles = 16;
    uint64_t handle_count = 0;
    j6_handle_t give_handles[max_handles];
    j6_handle_t *save_handles = nullptr;
    uint64_t proto_id;

    j6::syslog(j6::logs::proto, j6::log_level::verbose, "SL> Starting service locator on mbx handle %x", mb);

    while (true) {
        uint64_t data_len = sizeof(uint64_t);
        j6_status_t s = j6_mailbox_respond(mb, &tag,
            &data, &data_len, data_len,
            give_handles, &handle_count, max_handles,
            &reply_tag, j6_flag_block);

        if (s != j6_status_ok) {
            j6::syslog(j6::logs::proto, j6::log_level::error, "SL> Syscall returned error %lx, dying", s);
            continue;
        }

        switch (tag) {
        case j6_proto_sl_register:
            proto_id = data;
            if (!handle_count) {
                tag = j6_proto_base_status;
                data = j6_err_invalid_arg;
                break;
            }

            j6::syslog(j6::logs::proto, j6::log_level::verbose, "SL> Registering %d handles for proto %x", handle_count, proto_id);

            save_handles = new j6_handle_t [handle_count];
            memcpy(save_handles, give_handles, sizeof(j6_handle_t) * handle_count);
            services.insert( {proto_id, {save_handles, handle_count}} );
            tag = j6_proto_base_status;
            data = j6_status_ok;
            save_handles = nullptr;
            handle_count = 0;
            break;

        case j6_proto_sl_find:
            proto_id = data;
            tag = j6_proto_sl_result;
            data = 0;

            {
                handle_entry *found = services.find(proto_id);
                if (found) {
                    handle_count = found->handles.count;
                    memcpy(give_handles, found->handles.pointer, sizeof(j6_handle_t) * handle_count);
                    j6::syslog(j6::logs::proto, j6::log_level::verbose, "SL> Found %d handles for proto %x", handle_count, proto_id);
                } else {
                    handle_count = 0;
                    j6::syslog(j6::logs::proto, j6::log_level::verbose, "SL> Found no handles for proto %x", proto_id);
                }
            }
            break;

        default:
            tag = j6_proto_base_status;
            data = j6_err_invalid_arg;
            handle_count = 0;
            break;
        }
    }
}
