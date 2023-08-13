#include <stdint.h>
#include <stdlib.h>

#include <elf/headers.h>
#include <j6/init.h>
#include <j6/syslog.hh>
#include <util/pointers.h>
#include "relocate.h"

uintptr_t
locate_dyn_section(uintptr_t base, uintptr_t phdr, size_t phdr_size, size_t phdr_count)
{
    if (!phdr || !phdr_count)
        return 0;

    const elf::segment_header *ph_base =
        reinterpret_cast<const elf::segment_header*>(phdr + base);

    for (size_t i = 0; i < phdr_count; ++i) {
        const elf::segment_header *ph =
            util::offset_pointer(ph_base, i*phdr_size);

        if (ph->type == elf::segment_type::dynamic)
            return ph->vaddr;
    }

    return 0;
}

extern "C" uintptr_t
ldso_init(j6_arg_header *stack_args, uintptr_t const *got)
{
    j6_arg_loader *arg_loader = nullptr;
    j6_arg_handles *arg_handles = nullptr;

    j6_arg_header *arg = stack_args;
    while (arg && arg->type != j6_arg_type_none) {
        switch (arg->type)
        {
        case j6_arg_type_loader:
            arg_loader = reinterpret_cast<j6_arg_loader*>(arg);
            break;

        case j6_arg_type_handles:
            arg_handles = reinterpret_cast<j6_arg_handles*>(arg);
            break;
        
        default:
            break;
        }

        arg = util::offset_pointer(arg, arg->size);
    }

    if (!arg_loader) {
        j6::syslog("ld.so: error: did not get j6_arg_loader");
        exit(127);
    }

    relocate_image(arg_loader->loader_base, got[0]);

    uintptr_t dyn_section = locate_dyn_section(
        arg_loader->image_base,
        arg_loader->phdr,
        arg_loader->phdr_size,
        arg_loader->phdr_count);
    relocate_image(arg_loader->image_base, dyn_section);

    return arg_loader->entrypoint + arg_loader->image_base;
}