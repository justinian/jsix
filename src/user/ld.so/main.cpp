#include <stdint.h>
#include <stdlib.h>

#include <elf/headers.h>
#include <j6/init.h>
#include <j6/protocols/vfs.hh>
#include <j6/syslog.hh>
#include <util/pointers.h>

#include "image.h"

image_list all_images;

extern "C" uintptr_t
ldso_init(j6_arg_header *stack_args, uintptr_t *got)
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

    j6_handle_t vfs = j6_handle_invalid;
    if (arg_handles) {
        for (size_t i = 0; i < arg_handles->nhandles; ++i) {
            j6_arg_handle_entry &ent = arg_handles->handles[i];
            if (ent.proto == j6::proto::vfs::id) {
                vfs = ent.handle;
                break;
            }
        }
    }


    // First relocate ld.so itself. It cannot have any dependencies
    image_list::item_type ldso_image;
    ldso_image.base = arg_loader->loader_base;
    ldso_image.got = got;
    ldso_image.read_dyn_table(
        reinterpret_cast<const dyn_entry*>(got[0] + arg_loader->loader_base));

    image_list just_ldso;
    just_ldso.push_back(&ldso_image);
    ldso_image.relocate(just_ldso);

    image_list::item_type target_image;
    target_image.base = arg_loader->image_base;
    target_image.got = arg_loader->got;
    target_image.read_dyn_table(
        reinterpret_cast<const dyn_entry*>(arg_loader->got[0] + arg_loader->image_base));

    all_images.push_back(&target_image);
    all_images.load(vfs, 0xb00'0000);

    return arg_loader->entrypoint + arg_loader->image_base;
}
