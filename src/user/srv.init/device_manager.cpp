#include <j6/syslog.hh>
#include <j6/flags.h>
#include <pci/config.h>

#include "device_manager.h"
#include "loader.h"

inline constexpr j6_vm_flags mmio_flags = (j6_vm_flags)(j6_vm_flag_write | j6_vm_flag_mmio);


device_manager::device_manager(j6_handle_t sys, const void *root) :
    m_sys {sys}, m_acpi {root, root}
{
    load_mcfg();
}

void
device_manager::load_mcfg()
{
    m_groups.clear();

    for (const auto header : m_acpi) {
        if (header->type != acpi::mcfg::type_id)
            continue;

        const auto *mcfg = acpi::check_get_table<acpi::mcfg>(header);
        if (!mcfg) {
            j6::syslog(j6::logs::srv, j6::log_level::error, "MCFG table at %lx failed validation", header);
            continue;
        }

        size_t count = acpi::table_entries(mcfg, sizeof(acpi::mcfg_entry));

        for (unsigned i = 0; i < count; ++i) {
            const acpi::mcfg_entry &mcfge = mcfg->entries[i];

            pci::group group = {
                .group_id = mcfge.group,
                .bus_start = mcfge.bus_start,
                .bus_end = mcfge.bus_end,
                .base = reinterpret_cast<pci::header_unknown*>(mcfge.base),
            };

            j6::syslog(j6::logs::srv, j6::log_level::info, "Found MCFG entry: base %lx  group %d  bus %d-%d",
                    mcfge.base, mcfge.group, mcfge.bus_start, mcfge.bus_end);

            map_phys(m_sys, group.base, pci::group::config_size, mmio_flags);
            m_groups.push_back(group);
        }
    }
}

std::vector<device_info>
device_manager::find_devices(uint8_t klass, uint8_t subclass, uint8_t subsubclass)
{
    std::vector<device_info> found;

    for (auto group : m_groups) {
        for (unsigned b = group.bus_start; b <= group.bus_end; ++b) {
            uint8_t bus = static_cast<uint8_t>(b - group.bus_start);

            for (uint8_t dev = 0; dev < 32; ++dev) {
                pci::bus_addr a {0, dev, bus};
                if (!group.has_device(a)) continue;

                pci::header_unknown *h = reinterpret_cast<pci::header_unknown*>(group.device_base(a));
                j6::syslog(j6::logs::srv, j6::log_level::info,
                    "  Found PCI device at %02d:%02d:%d class %x.%x.%x id %04x:%04x",
                    a.bus, a.device, a.function,
                    h->device_class[2], h->device_class[1], h->device_class[0],
                    h->vendor, h->device);

                found.push_back({h, a});

                if (!h->multi_device) continue;
                for (uint8_t i = 1; i < 8; ++i) {
                    pci::bus_addr fa {i, dev, bus};
                    if (group.has_device(fa)) {
                        pci::header_unknown *fh =
                            reinterpret_cast<pci::header_unknown*>(group.device_base(fa));

                        j6::syslog(j6::logs::srv, j6::log_level::info,
                            "    found PCI func at %02d:%02d:%d class %x.%x.%x id %04x:%04x",
                            fa.bus, fa.device, fa.function,
                            fh->device_class[2], fh->device_class[1], fh->device_class[0],
                            fh->vendor, fh->device);

                        found.push_back({fh, fa});
                    }
                }
            }
        }
    }

    return found;
}
