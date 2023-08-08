#pragma once
/// \file j6/protocols.h
/// Common definition for j6 user-mode IPC protocols

enum j6_proto_base_tag
{
    j6_proto_base_status,
    j6_proto_base_get_proto_id,
    j6_proto_base_proto_id,
    j6_proto_base_open_channel,
    j6_proto_base_opened_channel,

    j6_proto_base_first_proto_id /// The first protocol-specific ID
};

#ifdef __cplusplus
#include <util/hash.h>
namespace j6::proto {

enum class style { mailbox, channel };

#define PROTOCOL(name, desc, type) namespace name { \
    inline constexpr uint64_t id = #desc##_id; \
    inline constexpr proto::style style = proto::style:: type; }

#include <j6/tables/protocols.inc>
#undef PROTOCOL

} // namespace j6::proto
#endif // __cplusplus

extern "C" const uint64_t j6_proto_style_mailbox;
extern "C" const uint64_t j6_proto_style_channel;

#define PROTOCOL(name, desc, type) \
    extern "C" const uint64_t j6_proto_ ## name ## _id; \
    extern "C" const uint64_t j6_proto_ ## name ## _style;

#include <j6/tables/protocols.inc>
#undef PROTOCOL