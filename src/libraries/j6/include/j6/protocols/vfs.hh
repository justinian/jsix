#pragma once
/// \file vfs.hh
/// C++ client interface for VFS protocol

#include <j6/protocols/vfs.h>
#include <j6/types.h>
#include <util/api.h>

namespace j6::proto::vfs {

class API client
{
public:
    /// Constructor.
    /// \arg vfs_mb  Handle to the VFS service's mailbox
    client(j6_handle_t vfs_mb);

    /// Load a file into a VMA
    /// \arg path  Path of the file to load
    /// \arg vma   [out] Handle to the loaded VMA, or invalid if not found
    /// \arg size  [out] Size of the file
    j6_status_t load_file(char *path, j6_handle_t &vma, size_t &size);

private:
    j6_handle_t m_service;
};

} // namespace j6::proto::vfs