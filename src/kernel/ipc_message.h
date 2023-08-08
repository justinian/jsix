#pragma once
/// \file ipc_message.h
/// Definition of shared message structure

#include <stdint.h>
#include <j6/types.h>
#include <util/counted.h>

namespace ipc {

struct message
{
    uint64_t tag;
    util::buffer data;
    util::counted<j6_handle_t> handles;

    message();
    message(uint64_t in_tag, const util::buffer &in_data, const util::counted<j6_handle_t> &in_handles);
    message(message &&other);
    ~message();

    message & operator=(message &&other);
};

} // namespace ipc