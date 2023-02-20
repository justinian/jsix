#include <arch/acpi/tables.h>
#include <bootproto/acpi.h>
#include <bootproto/init.h>
#include <j6/syslog.hh>

#include "acpi.h"
#include "loader.h"
#include "pci.h"

inline constexpr size_t bus_mmio_size = 0x1000'0000;
inline constexpr j6_vm_flags mmio_flags = (j6_vm_flags)(j6_vm_flag_write | j6_vm_flag_mmio);

void
probe_pci(j6_handle_t sys, pci_group &pci)
{
    j6::syslog("Probing PCI group at base %016lx", pci.base);
    map_phys(sys, pci.base, bus_mmio_size, mmio_flags);

    for (unsigned b = pci.bus_start; b <= pci.bus_end; ++b) {
        uint8_t bus = static_cast<uint8_t>(b);

        for (uint8_t dev = 0; dev < 32; ++dev) {
            if (!pci.has_device(bus, dev, 0)) continue;

            pci_device d0 {pci, bus, dev, 0};
            if (!d0.multi()) continue;

            for (uint8_t i = 1; i < 8; ++i) {
                if (pci.has_device(bus, dev, i))
                    pci_device dn {pci, bus, dev, i};
            }
        }
    }
}

void
load_mcfg(j6_handle_t sys, const acpi::table_header *header)
{
    const auto *mcfg = acpi::check_get_table<acpi::mcfg>(header);

    size_t count = acpi::table_entries(mcfg, sizeof(acpi::mcfg_entry));

    /*
    m_pci.set_size(count);
    m_devices.set_capacity(16);
    */

    for (unsigned i = 0; i < count; ++i) {
        const acpi::mcfg_entry &mcfge = mcfg->entries[i];

        pci_group group = {
            .group = mcfge.group,
            .bus_start = mcfge.bus_start,
            .bus_end = mcfge.bus_end,
            .base = reinterpret_cast<uint32_t*>(mcfge.base),
        };

        probe_pci(sys, group);

        j6::syslog("  Found MCFG entry: base %lx  group %d  bus %d-%d",
                mcfge.base, mcfge.group, mcfge.bus_start, mcfge.bus_end);
    }

    //probe_pci();
}

void
load_acpi(j6_handle_t sys, const bootproto::module *mod)
{
    const bootproto::acpi *info = mod->data<bootproto::acpi>();
    const util::const_buffer &region = info->region;

    map_phys(sys, region.pointer, region.count);

    const void *root_table = info->root;
    if (!root_table) {
        j6::syslog("null ACPI root table pointer");
        return;
    }

    const acpi::rsdp2 *acpi2 =
        reinterpret_cast<const acpi::rsdp2 *>(root_table);

    const auto *xsdt =
        acpi::check_get_table<acpi::xsdt>(acpi2->xsdt_address);

    size_t num_tables = acpi::table_entries(xsdt, sizeof(void*));
    for (size_t i = 0; i < num_tables; ++i) {
        const acpi::table_header *header = xsdt->headers[i];
        if (!header->validate()) {
            j6::syslog("ACPI table at %lx failed validation", header);
            continue;
        }

        switch (header->type) {
        case acpi::mcfg::type_id:
            load_mcfg(sys, header);
            break;

        default:
            break;
        }
    }
}
