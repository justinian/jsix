#include <acpi/acpi.h>
#include <acpi/tables.h>
#include <j6/syslog.hh>

namespace acpi {

system::system(const void* phys, const void *virt) :
    m_offset { util::get_offset(phys, virt) },
    m_root { reinterpret_cast<const rsdp2*>(virt) }
{}

system::iterator
system::begin() const
{
    const xsdt *sdt =
        acpi::check_get_table<xsdt>(m_root->xsdt_address);

    if (!sdt)
        return {nullptr, 0};

    return {&sdt->headers[0], m_offset};
}


system::iterator
system::end() const
{
    const xsdt *sdt =
        acpi::check_get_table<xsdt>(m_root->xsdt_address);

    if (!sdt)
        return {nullptr, 0};

    size_t nheaders = table_entries<xsdt>(sdt, sizeof(table_header*));
    return {&sdt->headers[nheaders], m_offset};
}

#if 0
void
load_acpi(j6_handle_t sys, const bootproto::module *mod)
{
    const bootproto::acpi *info = mod->data<bootproto::acpi>();
    const util::const_buffer &region = info->region;

    map_phys(sys, region.pointer, region.count);

    const void *root_table = info->root;
    if (!root_table) {
        j6::syslog(j6::logs::srv, j6::log_level::error, "null ACPI root table pointer");
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
            j6::syslog(j6::logs::srv, j6::log_level::error, "ACPI table at %lx failed validation", header);
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
#endif

} // namespace acpi
