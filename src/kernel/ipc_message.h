#pragma once
/// \file ipc_message.h
/// Definition of shared message structure

#include <stdint.h>
#include <j6/types.h>
#include <util/counted.h>
#include <util/pointers.h>

namespace ipc {

static constexpr size_t message_size = 64;

struct message
{
    uint64_t tag;
    uint16_t data_size;

    uint16_t handle_count : 4;
    uint16_t out_of_band : 1;

    uint32_t _reserved;

    uint8_t content[ message_size - 8 ];

    util::buffer data();
    util::const_buffer data() const;

    util::counted<j6_handle_t> handles();
    util::counted<const j6_handle_t> handles() const;

    message();
    message(uint64_t in_tag, const util::buffer &in_data, const util::counted<j6_handle_t> &in_handles);
    message(message &&other);
    ~message();

    message & operator=(message &&other);

    void set(uint64_t in_tag, const util::buffer &in_data, const util::counted<j6_handle_t> &in_handles);

private:
    void clear_oob();
};

using message_ptr = util::unique_ptr<message>;

} // namespace ipc
