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

enum j6_aux_type {
    // The SysV ABI-specified aux vector types
    j6_aux_null,        // AT_NULL
    j6_aux_ignore,      // AT_IGNORE
    j6_aux_execfd,      // AD_EXECFD - File descriptor of the exe to load
    j6_aux_phdr,        // AD_PHDR   - Program headers pointer for the exe to load
    j6_aux_phent,       // AD_PHENT  - Size of a program header entry
    j6_aux_phnum,       // AD_PHNUM  - Number of program header entries
    j6_aux_pagesz,      // AD_PAGESZ - System page size
    j6_aux_base,        // AD_BASE   - Base address of dynamic loader
    j6_aux_flags,       // AD_FLAGS  - Flags
    j6_aux_entry,       // AD_ENTRY  - Entrypoint for the exe to load
    j6_aux_notelf,      // AD_NOTELF - If non-zero, this program is not ELF
    j6_aux_uid,         // AD_UID    - User ID
    j6_aux_euid,        // AD_EUID   - Effective User ID
    j6_aux_gid,         // AD_GID    - Group ID
    j6_aux_egid,        // AD_EGID   - Effective Group ID

    j6_aux_start = 0xf000,
    j6_aux_handles,     // Pointer to a j6_arg_handles structure
    j6_aux_device,      // Pointer to a j6_arg_driver structure
    j6_aux_loader,      // Pointer to a j6_arg_loader structure
};

struct j6_aux
{
    uint64_t type;
    union {
        uint64_t value;
        void *pointer;
        void (*func)();
    };
};

struct j6_arg_loader
{
    uintptr_t loader_base;
    uintptr_t image_base;
    uintptr_t *got;
    uintptr_t entrypoint;
    uintptr_t start_addr;
};

struct j6_arg_driver
{
    uint64_t device;
    uint32_t size;
    uint8_t data [0];
};

struct j6_arg_handle_entry
{
    uint64_t proto;
    j6_handle_t handle;
};

struct j6_arg_handles
{
    size_t nhandles;
    j6_arg_handle_entry handles[0];
};

/// Find the first handle tagged with the given proto in the process init args
j6_handle_t API j6_find_init_handle(uint64_t proto);

#ifdef __cplusplus
} // extern "C"
#endif
