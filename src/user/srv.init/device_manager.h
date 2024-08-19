#pragma once
/// \file device_manager.h
/// APCI and PCI device lookup

#include <vector>
#include <j6/types.h>
#include <acpi/acpi.h>
#include <pci/group.h>

struct device_info
{
    pci::header_unknown *base;
    pci::bus_addr addr;
};

class device_manager
{
public:
    device_manager(j6_handle_t sys, const void *root);

    std::vector<device_info> find_devices(uint8_t klass, uint8_t subclass, uint8_t subsubclass);

private:
    void load_mcfg();

    j6_handle_t m_sys;
    acpi::system m_acpi;
    std::vector<pci::group> m_groups;
};
