#pragma once
/// \file init.h
/// Process initialization utility functions

#include <stddef.h>
#include <stdint.h>

#include <j6/types.h>
#include <util/api.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
#define add_header(name) \
    static constexpr j6_arg_type type_id = j6_arg_type_ ## name; \
    j6_arg_header header;
#else
#define add_header(name) \
    j6_arg_header header;
#endif

enum j6_arg_type {
    j6_arg_type_none,
    j6_arg_type_sysv_init,
    j6_arg_type_loader,
    j6_arg_type_driver,
    j6_arg_type_handles,
};

struct j6_arg_header
{
    uint32_t size;
    uint16_t type;
};

struct j6_arg_loader
{
    add_header(loader);
    uintptr_t loader_base;
    uintptr_t image_base;
    uintptr_t *got;
    uintptr_t entrypoint;
};

struct j6_arg_driver
{
    add_header(driver);
    uint64_t device;
    uint8_t data [0];
};

struct j6_arg_handle_entry
{
    uint64_t proto;
    j6_handle_t handle;
};

struct j6_arg_handles
{
    add_header(handles);
    size_t nhandles;
    j6_arg_handle_entry handles[0];
};

struct j6_init_args
{
    uint64_t args[2];
};



/// Find the first handle of the given type held by this process
j6_handle_t API j6_find_first_handle(j6_object_type obj_type);

/// Get the init args
const j6_init_args * j6_get_init_args();

/// Drivers may use driver_main instead of main
int driver_main(unsigned, const char **, const char **, const j6_init_args *);

#ifdef __cplusplus
} // extern "C"
#endif

#undef add_header
