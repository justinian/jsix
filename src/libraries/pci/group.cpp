#include "pci/config.h"
#include <j6/syslog.hh>
#include <pci/config.h>
#include <pci/device.h>
#include <pci/group.h>

namespace pci {

//map config space into memory:
//inline constexpr j6_vm_flags mmio_flags = (j6_vm_flags)(j6_vm_flag_write | j6_vm_flag_mmio);
//map_phys(sys, group.base, pci::group::config_size, mmio_flags);
group::iterator::iterator(const group &g, bus_addr a) :
    m_group {g}, m_addr {a}
{
    increment_until_valid(false);
}

void
group::iterator::increment()
{
    // If we're iterating functions, the device must be valid.
    // Finish iterating all functions.
    if (m_addr.function && m_addr.function < bus_addr::max_functions) {
        ++m_addr.function;
        return;
    }

    m_addr.function = 0;
    if (m_addr.device < bus_addr::max_devices) {
        ++m_addr.device;
        return;
    }

    m_addr.device = 0;
    ++m_addr.bus;
}

void
group::iterator::increment_until_valid(bool pre_increment)
{
    if (pre_increment)
        increment();

    bus_addr end_addr = {0, 0, uint16_t(m_group.bus_end+1)};
    if (!m_group.has_device(m_addr) && m_addr < end_addr)
        increment();
}

bool
group::has_device(bus_addr addr) const
{
    return device_base(addr)->vendor != 0xffff;
}

} // namespace pci
