#pragma once
/// \file allocator.h
/// Page allocator class definition

namespace uefi {
    class boot_services;
}

namespace kernel {
namespace init {
    enum class allocation_type : uint8_t;
    struct allocation_register;
    struct module;
    struct modules_page;
}}

namespace boot {
namespace memory {

using alloc_type = kernel::init::allocation_type;

class allocator
{
public:
    using allocation_register = kernel::init::allocation_register;
    using module = kernel::init::module;
    using modules_page = kernel::init::modules_page;

    allocator(uefi::boot_services &bs);

    void * allocate(size_t size, bool zero = false);
    void free(void *p);

    void * allocate_pages(size_t count, alloc_type type, bool zero = false);

    template <typename M>
    M * allocate_module(size_t extra = 0) {
        return static_cast<M*>(allocate_module_untyped(sizeof(M) + extra));
    }

    void memset(void *start, size_t size, uint8_t value);
    void copy(void *to, void *from, size_t size);

    inline void zero(void *start, size_t size) { memset(start, size, 0); }

    /// Initialize the global allocator
    /// \arg allocs   [out] Poiinter to the initial allocation register
    /// \arg modules  [out] Pointer to the initial modules_page
    /// \arg bs       UEFI boot services
    static void init(
            allocation_register *&allocs,
            modules_page *&modules,
            uefi::boot_services *bs);

private:
    void add_register();
    void add_modules();
    module * allocate_module_untyped(size_t size);

    uefi::boot_services &m_bs;

    allocation_register *m_register;
    modules_page *m_modules;
    module *m_next_mod;
};

} // namespace memory

extern memory::allocator &g_alloc;

} // namespace boot

void * operator new (size_t size, void *p);
void * operator new(size_t size);
void * operator new [] (size_t size);
void operator delete (void *p) noexcept;
void operator delete [] (void *p) noexcept;
