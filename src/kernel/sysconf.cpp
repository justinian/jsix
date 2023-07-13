#include <j6/memutils.h>

#include "kassert.h"
#include "cpu.h"
#include "frame_allocator.h"
#include "memory.h"
#include "sysconf.h"

struct kheader
{
    uint64_t magic;
    uint16_t header_len;
    uint16_t header_version;

    uint16_t version_major;
    uint16_t version_minor;
    uint16_t version_patch;
    uint16_t _reserved;
    uint32_t version_gitsha;

    uint64_t flags;
};

extern kheader _kernel_header;

system_config *g_sysconf = nullptr;
uintptr_t g_sysconf_phys = 0;

constexpr size_t sysconf_pages = mem::bytes_to_pages(sizeof(system_config));

void
sysconf_create()
{
    auto &fa = frame_allocator::get();

    size_t count = fa.allocate(sysconf_pages, &g_sysconf_phys);

    kassert(count == sysconf_pages,
            "Could not get enough contiguous pages for sysconf");

    g_sysconf = mem::to_virtual<system_config>(g_sysconf_phys);
    memset(g_sysconf, 0, sysconf_pages * mem::frame_size);

    g_sysconf->kernel_version_major = _kernel_header.version_major;
    g_sysconf->kernel_version_minor = _kernel_header.version_minor;
    g_sysconf->kernel_version_patch = _kernel_header.version_patch;
    g_sysconf->kernel_version_gitsha = _kernel_header.version_gitsha;

    g_sysconf->sys_page_size = mem::frame_size;
    g_sysconf->sys_large_page_size = 0;
    g_sysconf->sys_huge_page_size = 0;
    g_sysconf->sys_num_cpus = g_num_cpus;
}
