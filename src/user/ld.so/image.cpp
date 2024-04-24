#include <stdlib.h>

#include <elf/file.h>
#include <j6/errors.h>
#include <j6/flags.h>
#include <j6/memutils.h>
#include <j6/protocols/vfs.hh>
#include <j6/syscalls.h>
#include <j6/syslog.hh>
#include <util/format.h>

#include "image.h"
#include "j6/types.h"
#include "relocate.h"
#include "symbols.h"

extern "C" void _ldso_plt_lookup();
extern image_list all_images;


// Can't use strcmp because it's from another library, and
// this needs to be used as part of relocation or symbol lookup
static inline bool
str_equal(const char *a, const char *b)
{
    if (!a || !b)
        return a == b;

    size_t i = 0;
    while(a[i] && b[i] && a[i] == b[i]) ++i;
    return a[i] == b[i];
}

static inline uint32_t
gnu_hash_func(const char *s)
{
    uint32_t h = 5381;
    while (s && *s)
        h = (h<<5) + h + *s++;
    return h;
}


inline image_list::item_type *
new_image(const char *name)
{
    // Use malloc() instead of new to simplify linkage
    image_list::item_type *i = reinterpret_cast<image_list::item_type*>(malloc(sizeof(*i)));
    i->base = 0;
    i->name = name;
    i->got = nullptr;
    return i;
}

static uintptr_t
load_image(image_list::item_type &img, j6::proto::vfs::client &vfs)
{
    uintptr_t eop = 0; // end of program

    char path [1024];
    util::format({path, sizeof(path)}, "/jsix/lib/%s", img.name);

    size_t file_size = 0;
    j6_handle_t vma = j6_handle_invalid;
    j6_status_t r = vfs.load_file(path, vma, file_size);
    if (r != j6_status_ok) {
        j6::syslog(j6::logs::app, j6::log_level::error, "Error %d opening %s", r, path);
        return 0;
    }

    uintptr_t file_addr = 0;
    r = j6_vma_map(vma, 0, &file_addr, 0);
    if (r != j6_status_ok) {
        j6::syslog(j6::logs::app, j6::log_level::error, "Error %d opening %s", r, path);
        return 0;
    }

    elf::file file { util::const_buffer::from(file_addr, file_size) };
    if (!file.valid(elf::filetype::shared)) {
        j6::syslog(j6::logs::app, j6::log_level::error, "Error opening %s: Not an ELF shared object", path);
        return 0;
    }

    for (auto &seg : file.segments()) {
        if (seg.type == elf::segment_type::dynamic) {
            const dyn_entry *table =
                reinterpret_cast<const dyn_entry*>(img.base + seg.vaddr);
            img.read_dyn_table(table);
        }

        if (seg.type != elf::segment_type::load)
            continue;

        // TODO: way to remap VMA as read-only if there's no write flag on
        // the segment
        unsigned long flags = j6_vm_flag_exact | j6_vm_flag_write;
        if (seg.flags.get(elf::segment_flags::exec))
            flags |= j6_vm_flag_exec;

        uintptr_t start = file.base() + seg.offset;
        size_t prologue = seg.vaddr & 0xfff;
        size_t epilogue = seg.mem_size - seg.file_size;

        uintptr_t addr = (img.base + seg.vaddr) & ~0xfffull;
        j6_handle_t sub_vma = j6_handle_invalid;
        j6_status_t res = j6_vma_create_map(&sub_vma, seg.mem_size+prologue, &addr, flags);
        if (res != j6_status_ok) {
            j6::syslog(j6::logs::app, j6::log_level::error, "error loading '%s': creating sub vma: %lx", path, res);
            return 0;
        }

        uint8_t *src = reinterpret_cast<uint8_t *>(start);
        uint8_t *dest = reinterpret_cast<uint8_t *>(addr);
        memset(dest, 0, prologue);
        memcpy(dest+prologue, src, seg.file_size);
        memset(dest+prologue+seg.file_size, 0, epilogue);

        // end of segment
        uintptr_t eos = addr + seg.vaddr + seg.mem_size + prologue;
        if (eos > eop)
            eop = eos;
    }

    j6_vma_unmap(vma, 0);

    return eop;
}

void
image::read_dyn_table(dyn_entry const *table)
{
    size_t dynrel_size = 0;
    size_t sizeof_rela = sizeof(rela);
    size_t jmprel_size = 0;
    size_t soname_index = 0;

    bool parsing = true;
    while (parsing) {
        const dyn_entry &dyn = *table++;

        switch (dyn.tag) {
        case dyn_type::null:
            parsing = false;
            break;

        case dyn_type::pltrelsz:
            jmprel_size = dyn.value;
            break;

        case dyn_type::pltgot:
            got = reinterpret_cast<uintptr_t*>(dyn.value + base);
            break;

        case dyn_type::strtab:
            strtab.pointer = reinterpret_cast<char const*>(dyn.value + base);
            break;

        case dyn_type::symtab:
            dynsym = reinterpret_cast<const symbol*>(dyn.value + base);
            break;

        case dyn_type::rela:
            dynrel.pointer = reinterpret_cast<rela const*>(dyn.value + base);
            break;

        case dyn_type::relasz:
            dynrel_size = dyn.value;
            break;

        case dyn_type::relaent:
            sizeof_rela = dyn.value;
            break;

        case dyn_type::strsz:
            strtab.count = dyn.value;
            break;

        case dyn_type::jmprel:
            jmprel.pointer = reinterpret_cast<rela const*>(dyn.value + base);
            break;

        case dyn_type::gnu_hash:
            gnu_hash = reinterpret_cast<const gnu_hash_table*>(dyn.value + base);
            break;

        case dyn_type::soname:
            soname_index = dyn.value;
            break;

        default:
            break;
        }
    }

    if (dynrel_size && sizeof_rela)
        dynrel.count = dynrel_size / sizeof_rela;

    if (jmprel_size && sizeof_rela)
        jmprel.count = jmprel_size / sizeof_rela;

    if (soname_index && strtab)
        name = string(soname_index);
}

uintptr_t
image::lookup(const char *name) const
{
    if (!gnu_hash || !dynsym || !strtab.pointer)
        return 0;

    // Convenience references
    const gnu_hash_table &gh = *gnu_hash;

    uint32_t h = gnu_hash_func(name);

    // Check bloom filter
    static constexpr uint64_t bloom_bits = 6;
    static constexpr uint64_t mask = (1ull << bloom_bits) - 1;
    uint64_t bloom_index = (h >> bloom_bits) % gh.bloom_count;
    uint64_t bloom = gh.bloom[bloom_index];
    uint64_t test = (1ull << (h & mask)) | (1ull << ((h >> gh.bloom_shift) & mask));
    if ((bloom & test) != test)
        return 0;

    const uint32_t *buckets = reinterpret_cast<const uint32_t*>(
            &gh.bloom[gh.bloom_count]);
    const uint32_t *chains = &buckets[gh.bucket_count];

    uint32_t i = buckets[h % gh.bucket_count];
    if (i < gh.start_symbol)
        return 0;

    while (true) {
        const symbol &sym = dynsym[i];
        const char *sym_name = strtab.lookup(sym.name);
        uint32_t sym_hash = chains[i - gh.start_symbol];

        // Low bit is used to mark end-of-chain
        if ((h|1) == (sym_hash|1) && str_equal(name, sym_name))
            return base + sym.address;

        if (sym_hash & 1)
            break;

        ++i;
    }

    return 0;
}

void
add_needed_entries(image &img, image_list &open, image_list &closed)
{
    dyn_entry const *dyn = img.dyn_table();

    while (dyn->tag != dyn_type::null) {
        if (dyn->tag == dyn_type::needed) {
            const char *name = img.string(dyn->value);
            if (!open.find_image(name) && !closed.find_image(name))
                open.push_back(new_image(name));
        }
        ++dyn;
    }
}

void
image_list::load(j6_handle_t vfs_mb, uintptr_t addr)
{
    image_list open;
    j6::proto::vfs::client vfs {vfs_mb};

    for (auto *img : *this)
        add_needed_entries(*img, open, *this);

    while (!open.empty()) {
        image_list::item_type *img = open.pop_front();
        img->base = addr;

        // Load the file
        addr = load_image(*img, vfs);
        if (!img->got) {
            j6::syslog(j6::logs::app, j6::log_level::error, "Error opening %s: Could not find GOT", img->name);
            return;
        }

        j6::syslog(j6::logs::app, j6::log_level::verbose, "Loaded %s at offset 0x%lx", img->name, img->base);
        addr = (addr & ~0xffffull) + 0x10000;

        // Find the DT_NEEDED entries
        add_needed_entries(*img, open, *this);
        push_back(img);
    }

    for (auto *img : *this)
        img->relocate(*this);
}

void
image::parse_rela_table(const util::counted<const rela> &table, image_list &ctx)
{
    for (size_t i = 0; i < table.count; ++i) {
        const rela &rel = table[i];

        const symbol *sym_obj = dynsym ? &dynsym[rel.symbol] : nullptr;
        const char *sym_name = sym_obj ? string(sym_obj->name) : nullptr;
        uintptr_t sym_addr = sym_name && *sym_name ? ctx.resolve(sym_name) : 0;

        switch (rel.type)
        {
        case reloc::glob_dat:
        case reloc::jump_slot:
            *reinterpret_cast<uint64_t*>(rel.address + base) = sym_addr;
            break;

        case reloc::relative:
            *reinterpret_cast<uint64_t*>(rel.address + base) = base + rel.offset;
            break;

        default:
            j6::syslog(j6::logs::app, j6::log_level::verbose, "Unknown rela relocation type %d in %s", rel.type, name);
            exit(126);
            break;
        }
    }
}

void
image::relocate(image_list &ctx)
{
    if (relocated)
        return;

    parse_rela_table(dynrel, ctx);
    parse_rela_table(jmprel, ctx);

    got[1] = reinterpret_cast<uintptr_t>(this);
    got[2] = reinterpret_cast<uintptr_t>(&_ldso_plt_lookup);
    relocated = true;
}

image_list::item_type *
image_list::find_image(const char *name)
{
    for (auto *i : *this) {
        if (str_equal(i->name, name))
            return i;
    }
    return nullptr;
}

uintptr_t
image_list::resolve(const char *name)
{
    for (auto *img : *this) {
        uintptr_t addr = img->lookup(name);
        if (addr) return addr;
    }
    return 0;
}

extern "C" uintptr_t
ldso_plt_lookup(const image *img, unsigned jmprel_index)
{
    const rela &rel = img->jmprel[jmprel_index];
    const symbol &sym = img->dynsym[rel.symbol];
    const char *name = img->string(sym.name);
    uintptr_t addr = all_images.resolve(name);
    return addr;
}
