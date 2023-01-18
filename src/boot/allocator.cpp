#include <uefi/boot_services.h>
#include <uefi/types.h>

#include <bootproto/init.h>
#include <bootproto/kernel.h>
#include <util/no_construct.h>
#include <util/pointers.h>

#include "allocator.h"
#include "error.h"
#include "memory.h"

namespace boot {

util::no_construct<memory::allocator> __g_alloc_storage;
memory::allocator &g_alloc = __g_alloc_storage.value;

namespace memory {

using bootproto::allocation_register;
using bootproto::module;
using bootproto::page_allocation;

static_assert(sizeof(allocation_register) == page_size);


void
allocator::init(
        allocation_register *&allocs,
        modules_page *&modules,
        uefi::boot_services *bs)
{
    new (&g_alloc) allocator(*bs);
    allocs = g_alloc.m_register;
    modules = g_alloc.m_modules;
}


allocator::allocator(uefi::boot_services &bs) :
    m_bs(bs),
    m_register(nullptr),
    m_modules(nullptr)
{
    add_register();
    add_modules();
}

void
allocator::add_register()
{
    allocation_register *reg = nullptr;

    try_or_raise(
        m_bs.allocate_pages(uefi::allocate_type::any_pages,
            uefi::memory_type::loader_data, 1, reinterpret_cast<void**>(&reg)),
        L"Failed allocating allocation register page");

    m_bs.set_mem(reg, sizeof(allocation_register), 0);

    if (m_register)
        m_register->next = reg;

    m_register = reg;
    return;
}

void
allocator::add_modules()
{
    modules_page *mods = reinterpret_cast<modules_page*>(
        allocate_pages(1, alloc_type::init_args, true));

    if (m_modules)
        m_modules->next = reinterpret_cast<uintptr_t>(mods);

    m_modules = mods;
    m_next_mod = mods->modules;
    return;
}

void *
allocator::allocate_pages(size_t count, alloc_type type, bool zero)
{
    if (count & ~0xffffffffull) {
        error::raise(uefi::status::unsupported,
            L"Cannot allocate more than 16TiB in pages at once.",
            __LINE__);
    }

    if (!m_register || m_register->count == 0xff)
        add_register();

    void *pages = nullptr;

    try_or_raise(
        m_bs.allocate_pages(uefi::allocate_type::any_pages,
            uefi::memory_type::loader_data, count, &pages),
        L"Failed allocating usable pages");

    page_allocation &ent = m_register->entries[m_register->count++];
    ent.address = reinterpret_cast<uintptr_t>(pages);
    ent.count = count;
    ent.type = type;

    if (zero)
        m_bs.set_mem(pages, count * page_size, 0);

    return pages;
}

module *
allocator::allocate_module()
{
    static constexpr size_t size = sizeof(module);

    size_t remaining =
        reinterpret_cast<uintptr_t>(m_modules) + page_size
        - reinterpret_cast<uintptr_t>(m_next_mod) - sizeof(module);

    if (size > remaining)
        add_modules();

    ++m_modules->count;
    module *m = m_next_mod;
    m_next_mod = util::offset_pointer(m_next_mod, size);
    return m;
}

void *
allocator::allocate(size_t size, bool zero)
{
    void *p = nullptr;
    try_or_raise(
        m_bs.allocate_pool(uefi::memory_type::loader_data, size, &p),
        L"Could not allocate pool memory");

    if (zero)
        m_bs.set_mem(p, size, 0);

    return p;
}

void
allocator::free(void *p)
{
    try_or_raise(
        m_bs.free_pool(p),
        L"Freeing pool memory");
}

void
allocator::memset(void *start, size_t size, uint8_t value)
{
    m_bs.set_mem(start, size, value);
}

void
allocator::copy(void *to, void *from, size_t size)
{
    m_bs.copy_mem(to, from, size);
}

} // namespace memory
} // namespace boot


void * operator new (size_t size, void *p) { return p; }
void * operator new(size_t size)           { return boot::g_alloc.allocate(size); }
void * operator new [] (size_t size)       { return boot::g_alloc.allocate(size); }
void operator delete (void *p) noexcept    { return boot::g_alloc.free(p); }
void operator delete [] (void *p) noexcept { return boot::g_alloc.free(p); }

