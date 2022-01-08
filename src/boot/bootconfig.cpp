#include <uefi/boot_services.h>
#include <uefi/types.h>

#include "bootconfig.h"
#include "error.h"
#include "fs.h"
#include "status.h"

namespace boot {

constexpr uint64_t jsixboot = 0x746f6f627869736a; // "jsixboot"

static const wchar_t *
read_string(util::buffer &data)
{
    uint16_t size = *util::read<uint16_t>(data);
    const wchar_t *string = reinterpret_cast<const wchar_t*>(data.pointer);
    data += size;
    return string;
}

static void
read_descriptor(descriptor &e, util::buffer &data)
{
    e.flags = static_cast<desc_flags>(*util::read<uint16_t>(data));
    e.path = read_string(data);
    e.desc = read_string(data);
}

bootconfig::bootconfig(util::buffer data, uefi::boot_services *bs)
{
    status_line status {L"Loading boot config"};

    if (*util::read<uint64_t>(data) != jsixboot)
        error::raise(uefi::status::load_error, L"Bad header in jsix_boot.dat");

    const uint8_t version = *util::read<uint8_t>(data);
    if (version != 0)
        error::raise(uefi::status::incompatible_version, L"Bad version in jsix_boot.dat");

    data += 1; // reserved byte
    uint16_t num_programs = *util::read<uint16_t>(data);
    uint16_t num_data = *util::read<uint16_t>(data);
    data += 2; // reserved short

    read_descriptor(m_kernel, data);
    read_descriptor(m_init, data);

    m_programs.count = num_programs;
    m_programs.pointer = new descriptor [num_programs];

    for (unsigned i = 0; i < num_programs; ++i)
        read_descriptor(m_programs[i], data);

    m_data.count = num_programs;
    m_data.pointer = new descriptor [num_data];

    for (unsigned i = 0; i < num_data; ++i)
        read_descriptor(m_data[i], data);
}

} // namespace boot
