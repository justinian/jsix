#include <j6/protocols.h>

extern "C" const uint64_t j6_proto_style_mailbox = (uint64_t)j6::proto::style::mailbox;
extern "C" const uint64_t j6_proto_style_channel = (uint64_t)j6::proto::style::channel;

#define PROTOCOL(name, desc, type) \
    extern "C" const uint64_t j6_proto_##name##_id = j6::proto::name::id; \
    extern "C" const uint64_t j6_proto_##name##_style = j6_proto_style_##type;

#include <j6/tables/protocols.inc>