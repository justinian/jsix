#include <j6/memutils.h>
#include <util/basic_types.h>

#include "kassert.h"
#include "ipc_message.h"
#include "j6/types.h"

namespace ipc {

message::message() : tag {0}, data_size {0}, handle_count {0}, out_of_band {0} {}


message::message(
    uint64_t in_tag,
    const util::buffer &in_data,
    const util::counted<j6_handle_t> &in_handles) :
        out_of_band {0}
{
    set(in_tag, in_data, in_handles);
}


message::message(message &&other) {
    *this = util::move(other);
}


message::~message()
{
    clear_oob();
}


util::buffer
message::data()
{
    uint8_t *buf = content + (handle_count * sizeof(j6_handle_t));
    if (out_of_band)
        buf = reinterpret_cast<uint8_t**>(content)[handle_count];

    return {
        .pointer = buf,
        .count = data_size,
    };
}


util::const_buffer
message::data() const
{
    const uint8_t *buf = content + (handle_count * sizeof(j6_handle_t));
    if (out_of_band)
        buf = reinterpret_cast<uint8_t *const *const>(content)[handle_count];

    return {
        .pointer = buf,
        .count = data_size,
    };
}


util::counted<j6_handle_t>
message::handles()
{
    return {
        .pointer = reinterpret_cast<j6_handle_t*>(content),
        .count = handle_count,
    };
}


util::counted<const j6_handle_t>
message::handles() const
{
    return {
        .pointer = reinterpret_cast<const j6_handle_t*>(content),
        .count = data_size,
    };
}


message &
message::operator=(message &&other)
{
    clear_oob();

    tag = other.tag;
    other.tag = 0;

    data_size = other.data_size;
    other.data_size = 0;

    handle_count = other.handle_count;
    other.handle_count = 0;

    out_of_band = other.out_of_band;
    other.out_of_band = 0;
    
    memcpy(content, other.content, sizeof(content));
    return *this;
}


void
message::set(
    uint64_t in_tag,
    const util::buffer &in_data,
    const util::counted<j6_handle_t> &in_handles)
{
    clear_oob();
    tag = in_tag;
    handle_count = in_handles.count;
    data_size = in_data.count;

    if (in_handles.count) {
        kassert(in_handles.count < (sizeof(content) / sizeof(j6_handle_t)) - sizeof(void*));
        util::counted<j6_handle_t> handlebuf = handles();
        memcpy(handlebuf.pointer, in_handles.pointer, handlebuf.count * sizeof(j6_handle_t));
    }

    if (in_data.count) {
        if (in_data.count > sizeof(content) - (handle_count * sizeof(j6_handle_t))) {
            out_of_band = 1;
            auto *buf = new uint8_t [in_data.count];
            reinterpret_cast<uint8_t**>(content)[handle_count] = buf;
        }
        util::buffer databuf = data();
        memcpy(databuf.pointer, in_data.pointer, databuf.count);
    }
}


void
message::clear_oob()
{
    if (out_of_band) {
        uint8_t *buf = reinterpret_cast<uint8_t**>(content)[handle_count];
        delete [] buf;
    }
}

} // namespace ipc
