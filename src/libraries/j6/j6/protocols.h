#pragma once
/// \file j6/protocols.h
/// Common definition for j6 user-mode IPC protocols

enum j6_proto_base_tag
{
    j6_proto_base_status,
    j6_proto_base_get_proto_id,
    j6_proto_base_proto_id,

    j6_proto_base_first_proto_id /// The first protocol-specific ID
};
