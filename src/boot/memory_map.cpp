#include <uefi/boot_services.h>
#include <uefi/types.h>

#include "allocator.h"
#include "error.h"
#include "kernel_memory.h"
#include "memory.h"
#include "memory_map.h"
#include "paging.h"
#include "status.h"

namespace boot {
namespace memory {

using kernel::init::frame_block;
using kernel::init::frames_per_block;
using kernel::init::mem_entry;
using kernel::init::mem_type;


void
efi_mem_map::update(uefi::boot_services &bs)
{
    size_t l = total;
    uefi::status status = bs.get_memory_map(
        &l, entries, &key, &size, &version);
    length = l;

    if (status == uefi::status::success)
        return;

    if (status != uefi::status::buffer_too_small)
        error::raise(status, L"Error getting memory map size");

    if (entries) {
        try_or_raise(
            bs.free_pool(reinterpret_cast<void*>(entries)),
            L"Freeing previous memory map space");
    }

    total = length + 10 * size;

    try_or_raise(
        bs.allocate_pool(
            uefi::memory_type::loader_data, total,
            reinterpret_cast<void**>(&entries)),
        L"Allocating space for memory map");

    length = total;
    try_or_raise(
        bs.get_memory_map(&length, entries, &key, &size, &version),
        L"Getting UEFI memory map");
}

static const wchar_t *memory_type_names[] = {
    L"reserved memory type",
    L"loader code",
    L"loader data",
    L"boot services code",
    L"boot services data",
    L"runtime services code",
    L"runtime services data",
    L"conventional memory",
    L"unusable memory",
    L"acpi reclaim memory",
    L"acpi memory nvs",
    L"memory mapped io",
    L"memory mapped io port space",
    L"pal code",
    L"persistent memory"
};

static const wchar_t *kernel_memory_type_names[] = {
    L"free",
    L"pending",
    L"acpi",
    L"uefi_runtime",
    L"mmio",
    L"persistent"
};

static const wchar_t *
memory_type_name(uefi::memory_type t)
{
    if (t < uefi::memory_type::max_memory_type)
        return memory_type_names[static_cast<uint32_t>(t)];

    return L"Bad Type Value";
}

static const wchar_t *
kernel_memory_type_name(kernel::init::mem_type t)
{
    return kernel_memory_type_names[static_cast<uint32_t>(t)];
}

inline bool
can_merge(mem_entry &prev, mem_type type, uefi::memory_descriptor &next)
{
    return
        prev.type == type &&
        prev.start + (page_size * prev.pages) == next.physical_start &&
        prev.attr == (next.attribute & 0xffffffff);
}

counted<mem_entry>
build_kernel_map(efi_mem_map &map)
{
    status_line status {L"Creating kernel memory map"};

    size_t map_size = map.num_entries() * sizeof(mem_entry);
    size_t num_pages = bytes_to_pages(map_size);
    mem_entry *kernel_map = reinterpret_cast<mem_entry*>(
        g_alloc.allocate_pages(num_pages, alloc_type::mem_map, true));

    size_t nent = 0;
    bool first = true;
    for (auto &desc : map) {
        /*
        // EFI map dump
        console::print(L"   eRange %lx (%lx) %x(%s) [%lu]\r\n",
            desc.physical_start, desc.attribute, desc.type, memory_type_name(desc.type), desc.number_of_pages);
        */

        mem_type type;
        switch (desc.type) {
            case uefi::memory_type::reserved:
            case uefi::memory_type::unusable_memory:
            case uefi::memory_type::acpi_memory_nvs:
            case uefi::memory_type::pal_code:
                continue;

            case uefi::memory_type::loader_code:
            case uefi::memory_type::boot_services_code:
            case uefi::memory_type::boot_services_data:
            case uefi::memory_type::conventional_memory:
            case uefi::memory_type::loader_data:
                type = mem_type::free;
                break;

            case uefi::memory_type::runtime_services_code:
            case uefi::memory_type::runtime_services_data:
                type = mem_type::uefi_runtime;
                break;

            case uefi::memory_type::acpi_reclaim_memory:
                type = mem_type::acpi;
                break;

            case uefi::memory_type::memory_mapped_io:
            case uefi::memory_type::memory_mapped_io_port_space:
                type = mem_type::mmio;
                break;

            case uefi::memory_type::persistent_memory:
                type = mem_type::persistent;
                break;

            default:
                error::raise(
                    uefi::status::invalid_parameter,
                    L"Got an unexpected memory type from UEFI memory map");
        }

        // TODO: validate uefi's map is sorted
        if (first) {
            first = false;
            mem_entry &ent = kernel_map[nent++];
            ent.start = desc.physical_start;
            ent.pages = desc.number_of_pages;
            ent.type = type;
            ent.attr = (desc.attribute & 0xffffffff);
            continue;
        }

        mem_entry &prev = kernel_map[nent - 1];
        if (can_merge(prev, type, desc)) {
            prev.pages += desc.number_of_pages;
        } else {
            mem_entry &next = kernel_map[nent++];
            next.start = desc.physical_start;
            next.pages = desc.number_of_pages;
            next.type = type;
            next.attr = (desc.attribute & 0xffffffff);
        }
    }

    /*
    // kernel map dump
    for (unsigned i = 0; i < nent; ++i) {
        const mem_entry &e = kernel_map[i];
        console::print(L"   kRange %lx (%lx) %x(%s) [%lu]\r\n",
            e.start, e.attr, e.type, kernel_memory_type_name(e.type), e.pages);
    }
    */

    return { .pointer = kernel_map, .count = nent };
}

inline size_t bitmap_size(size_t frames) { return (frames + 63) / 64; }
inline size_t num_blocks(size_t frames) { return (frames + (frames_per_block-1)) / frames_per_block; }

counted<kernel::init::frame_block>
build_frame_blocks(const counted<kernel::init::mem_entry> &kmap)
{
    status_line status {L"Creating kernel frame accounting map"};

    size_t block_count = 0;
    size_t total_bitmap_size = 0;
    for (size_t i = 0; i < kmap.count; ++i) {
        const mem_entry &ent = kmap[i];
        if (ent.type != mem_type::free)
            continue;

        block_count += num_blocks(ent.pages);
        total_bitmap_size += bitmap_size(ent.pages) * sizeof(uint64_t);
    }

    size_t total_size = block_count * sizeof(frame_block) + total_bitmap_size;

    frame_block *blocks = reinterpret_cast<frame_block*>(
        g_alloc.allocate_pages(bytes_to_pages(total_size), alloc_type::frame_map, true));

    frame_block *next_block = blocks;
    for (size_t i = 0; i < kmap.count; ++i) {
        const mem_entry &ent = kmap[i];
        if (ent.type != mem_type::free)
            continue;

        size_t page_count = ent.pages;
        uintptr_t base_addr = ent.start;
        while (page_count) {
            frame_block *blk = next_block++;

            blk->flags = static_cast<kernel::init::frame_flags>(ent.attr);
            blk->base = base_addr;
            base_addr += frames_per_block * page_size;

            if (page_count >= frames_per_block) {
                page_count -= frames_per_block;
                blk->count = frames_per_block;
                blk->map1 = ~0ull;
                g_alloc.memset(blk->map2, sizeof(blk->map2), 0xff);
            } else {
                blk->count = page_count;
                unsigned i = 0;

                uint64_t b1 = (page_count + 4095) / 4096;
                blk->map1 = (1 << b1) - 1;

                uint64_t b2 = (page_count + 63) / 64;
                uint64_t b2q = b2 / 64;
                uint64_t b2r = b2 % 64;
                g_alloc.memset(blk->map2, b2q, 0xff);
                blk->map2[b2q] = (1 << b2r) - 1;
                break;
            }
        }
    }

    uint64_t *bitmap = reinterpret_cast<uint64_t*>(next_block);
    for (unsigned i = 0; i < block_count; ++i) {
        frame_block &blk = blocks[i];
        blk.bitmap = bitmap;

        size_t b = blk.count / 64;
        size_t r = blk.count % 64;
        g_alloc.memset(blk.bitmap, b*8, 0xff);
        blk.bitmap[b] = (1 << r) - 1;

        bitmap += bitmap_size(blk.count);
    }

    return { .pointer = blocks, .count = block_count };
}

void
fix_frame_blocks(kernel::init::args *args)
{
    counted<frame_block> &blocks = args->frame_blocks;

    size_t size = blocks.count * sizeof(frame_block);
    for (unsigned i = 0; i < blocks.count; ++i)
        size += bitmap_size(blocks[i].count) * sizeof(uint64_t);

    size_t pages = bytes_to_pages(size);
    uintptr_t addr = reinterpret_cast<uintptr_t>(blocks.pointer);

    // Map the frame blocks to the appropriate address
    paging::map_pages(args, addr,
        ::memory::bitmap_start, pages, true, false);

    uintptr_t offset = ::memory::bitmap_start - addr;

    for (unsigned i = 0; i < blocks.count; ++i) {
        frame_block &blk = blocks[i];
        blk.bitmap = offset_ptr<uint64_t>(blk.bitmap, offset);
    }
}


} // namespace memory
} // namespace boot
