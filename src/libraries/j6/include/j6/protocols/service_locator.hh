#include <j6/protocols/service_locator.h>
#include <j6/types.h>

namespace j6::proto::sl {

class client
{
public:
    /// Constructor.
    /// \arg slp_mb  Handle to the service locator service's mailbox
    client(j6_handle_t slp_mb);

    /// Register a handle as a service with the locator.
    /// \arg proto_id  The protocol this handle supports
    /// \arg handle    The mailbox or channel handle
    j6_status_t register_service(uint64_t proto_id, j6_handle_t handle);

    /// Look up a handle with the locator service.
    /// \arg proto_id  The protocol to look for
    /// \arg handle    [out] The mailbox or channel handle
    j6_status_t lookup_service(uint64_t proto_id, j6_handle_t &handle);

private:
    j6_handle_t m_service;
};

} // namespace j6::proto::sl