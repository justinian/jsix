#include <j6/protocols/service_locator.h>
#include <j6/types.h>
#include <util/api.h>
#include <util/counted.h>

namespace j6::proto::sl {

class API client
{
public:
    /// Constructor.
    /// \arg slp_mb  Handle to the service locator service's mailbox
    client(j6_handle_t slp_mb);

    /// Register a handle as a service with the locator.
    /// \arg proto_id  The protocol this handle supports
    /// \arg handles   The mailbox or channel handles
    j6_status_t register_service(uint64_t proto_id, util::counted<j6_handle_t> handles);

    /// Look up a handle with the locator service.
    /// \arg proto_id  The protocol to look for
    /// \arg handles   [out] The mailbox or channel handles
    j6_status_t lookup_service(uint64_t proto_id, util::counted<j6_handle_t> &handles);

private:
    j6_handle_t m_service;
};

} // namespace j6::proto::sl
