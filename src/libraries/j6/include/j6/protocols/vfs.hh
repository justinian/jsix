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
    client(j6_handle_t vfs_mb = 0);

    /// Copy constructor
    client(const client& c);

    /// Check if this client's handle is valid
    inline bool valid() const { return m_service != j6_handle_invalid; }

    /// Load a file into a VMA
    /// \arg path  Path of the file to load
    /// \arg vma   [out] Handle to the loaded VMA, or invalid if not found
    /// \arg size  [out] Size of the file
    j6_status_t load_file(char *path, j6_handle_t &vma, size_t &size);

    /// Get fs tag
    /// \arg tag   [out] The filesystem's tag
    /// \arg size  [inout] Size of the input buffer, length of the returned string
    j6_status_t get_tag(char *tag, size_t &len);

private:
    j6_handle_t m_service;
};

} // namespace j6::proto::vfs
