#pragma once
/// \file j6/protocols/service_locator.h
/// Definitions for the service locator protocol

#include <j6/protocols.h>

enum j6_proto_sl_tag
{
    j6_proto_sl_register = j6_proto_base_first_proto_id,
    j6_proto_sl_find,
    j6_proto_sl_result,
};
