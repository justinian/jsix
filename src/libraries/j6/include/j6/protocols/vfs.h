#pragma once
/// \file j6/protocols/vfs.h
/// Definitions for the virtual file system protocol

#include <j6/protocols.h>

enum j6_proto_vfs_tag
{
    j6_proto_vfs_load = j6_proto_base_first_proto_id,
    j6_proto_vfs_file,
};