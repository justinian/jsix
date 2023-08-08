#include <j6/memutils.h>
#include <util/basic_types.h>

#include "ipc_message.h"

namespace ipc {

message::message() : tag {0}, data {nullptr, 0}, handles {nullptr, 0} {}

message::message(
    uint64_t in_tag,
    const util::buffer &in_data,
    const util::counted<j6_handle_t> &in_handles) :
        tag {in_tag}, data {nullptr, in_data.count}, handles {nullptr, in_handles.count}
{
    if (data.count) {
        data.pointer = new uint8_t [data.count];
        memcpy(data.pointer, in_data.pointer, data.count);
    }

    if (handles.count) {
        handles.pointer = new j6_handle_t [handles.count];
        memcpy(handles.pointer, in_handles.pointer, handles.count * sizeof(j6_handle_t));
    }
}

message::message(message &&other) { *this = util::move(other); }

message::~message()
{
    delete [] reinterpret_cast<uint8_t*>(data.pointer);
    delete [] handles.pointer;
}

message &
message::operator=(message &&other)
{
    tag = other.tag;
    other.tag = 0;

    data = other.data;
    other.data = {nullptr, 0};

    handles = other.handles;
    other.handles = {nullptr, 0};
    return *this;
}

} // namespace ipc