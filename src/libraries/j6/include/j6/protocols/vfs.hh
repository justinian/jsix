#include <j6/protocols/vfs.h>
#include <j6/types.h>

namespace j6::proto::vfs {

class client
{
public:
    /// Constructor.
    /// \arg vfs_mb  Handle to the VFS service's mailbox
    client(j6_handle_t vfs_mb);

    /// Load a file into a VMA
    /// \arg path  Path of the file to load
    /// \arg vma   [out] Handle to the loaded VMA, or invalid if not found
    j6_status_t load_file(char *path, j6_handle_t &vma);

private:
    j6_handle_t m_service;
};

} // namespace j6::proto::vfs