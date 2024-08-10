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
ldso_init(const uint64_t *stack, uintptr_t *got)
{
    j6_arg_loader const *arg_loader = nullptr;
    j6_arg_handles const *arg_handles = nullptr;

    // Walk the stack to get the aux vector
    uint64_t argc = *stack++;
    stack += argc + 1; // Skip argv's and sentinel
    while (*stack++); // Skip envp's and sentinel

    j6_aux const *aux = reinterpret_cast<const j6_aux*>(stack);
    bool more = true;
    while (aux && more) {
        switch (aux->type)
        {
        case j6_aux_null:
            more = false;
            break;

        case j6_aux_loader:
            arg_loader = reinterpret_cast<const j6_arg_loader*>(aux->pointer);
            break;

        case j6_aux_handles:
            arg_handles = reinterpret_cast<const j6_arg_handles*>(aux->pointer);
            break;

        default:
            break;
        }
        ++aux;
    }

    if (!arg_loader) {
        exit(127);
    }

    j6_handle_t vfs = j6_handle_invalid;
    if (arg_handles) {
        for (size_t i = 0; i < arg_handles->nhandles; ++i) {
            const j6_arg_handle_entry &ent = arg_handles->handles[i];
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
    target_image.ctors = true; // crt0 will call the ctors
    target_image.read_dyn_table(
        reinterpret_cast<const dyn_entry*>(arg_loader->got[0] + arg_loader->image_base));

    all_images.push_back(&target_image);
    all_images.load(vfs, arg_loader->start_addr);

    j6::syslog(j6::logs::app, j6::log_level::verbose, "ld.so finished, jumping to entrypoint");
    return arg_loader->entrypoint + arg_loader->image_base;
}
