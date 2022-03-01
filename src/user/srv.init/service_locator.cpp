#include <unordered_map>

#include <j6/errors.h>
#include <j6/flags.h>
#include <j6/types.h>
#include <j6/protocols/service_locator.h>
#include <j6/syscalls.h>
#include <util/map.h>

#include "service_locator.h"

void
service_locator_start(j6_handle_t mb)
{
    // TODO: This should be a multimap
    util::map<uint64_t, j6_handle_t> services;

    uint64_t tag;
    uint16_t reply_tag;

    uint8_t data[100];
    size_t data_len = sizeof(data);

    j6_handle_t handles[10];
    size_t handles_count = sizeof(handles)/sizeof(j6_handle_t);

    j6_status_t s = j6_mailbox_receive(mb, &tag,
            data, &data_len,
            handles, &handles_count,
            &reply_tag, nullptr,
            j6_mailbox_block);

    uint64_t proto_id;
    uint64_t *data_words = reinterpret_cast<uint64_t*>(data);

    while (true) {
        j6_handle_t *found = nullptr;

        switch (tag) {
        case j6_proto_base_get_proto_id:
            tag = j6_proto_base_proto_id;
            *data_words = j6_proto_sl_id;
            data_len = sizeof(j6_status_t);
            handles_count = 0;
            break;

        case j6_proto_sl_register:
            proto_id = *data_words;
            if (handles_count != 1) {
                tag = j6_proto_base_status;
                *data_words = j6_err_invalid_arg;
                data_len = sizeof(j6_status_t);
                handles_count = 0;
                break;
            }

            services.insert( proto_id, handles[0] );
            tag = j6_proto_base_status;
            *data_words = j6_status_ok;
            data_len = sizeof(j6_status_t);
            handles_count = 0;
            break;

        case j6_proto_sl_find:
            proto_id = *data_words;
            tag = j6_proto_sl_result;
            data_len = 0;

            found = services.find(proto_id);
            if (found) {
                handles_count = 1;
                handles[0] = *found;
            } else {
                handles_count = 0;
            }
            break;

        default:
            tag = j6_proto_base_status;
            *data_words = j6_err_invalid_arg;
            data_len = sizeof(j6_status_t);
            handles_count = 0;
            break;
        }

        size_t data_in = data_len;
        size_t handles_in = handles_count;
        data_len = sizeof(data);
        handles_count = sizeof(handles)/sizeof(j6_handle_t);

        s = j6_mailbox_respond_receive(mb, &tag,
                data, &data_len, data_in,
                handles, &handles_count, handles_in,
                &reply_tag, nullptr,
                j6_mailbox_block);
    }
}
