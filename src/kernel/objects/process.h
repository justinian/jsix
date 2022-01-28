#pragma once
/// \file process.h
/// Definition of process kobject types

#include <j6/caps.h>
#include <util/map.h>
#include <util/vector.h>

#include "objects/handle.h"
#include "objects/kobject.h"
#include "page_table.h"
#include "vm_space.h"

namespace obj {

class process :
    public kobject
{
public:
    /// Capabilities on a newly constructed process handle
    constexpr static j6_cap_t creation_caps = j6_cap_process_all;

    /// Capabilities on a process to itself
    constexpr static j6_cap_t self_caps = j6_cap_process_all;

    /// Top of memory area where thread stacks are allocated
    constexpr static uintptr_t stacks_top = 0x0000800000000000;

    /// Size of userspace thread stacks
    constexpr static size_t stack_size = 0x4000000; // 64MiB

    /// Value that represents default priority
    constexpr static uint8_t default_priority = 0xff;

    /// Constructor.
    process();

    /// Destructor.
    virtual ~process();

    static constexpr kobject::type type = kobject::type::process;

    /// Get the currently executing process.
    static process & current();

    /// Terminate this process.
    /// \arg code   The return code to exit with.
    void exit(int32_t code);

    /// Update internal bookkeeping about threads.
    void update();

    /// Get the process' virtual memory space
    vm_space & space() { return m_space; }

    /// Create a new thread in this process
    /// \args rsp3      If non-zero, sets the ring3 stack pointer to this value
    /// \args priority  The new thread's scheduling priority
    /// \returns        The newly created thread object
    thread * create_thread(uintptr_t rsp3 = 0, uint8_t priorty = default_priority);

    /// Start tracking an object with a handle.
    /// \args obj  The object this handle refers to
    /// \args caps The capabilities on this handle
    /// \returns   The new handle for this object
    j6_handle_t add_handle(kobject *obj, j6_cap_t caps);

    /// Stop tracking an object with a handle.
    /// \args handle The handle that refers to the object
    /// \returns     True if the handle was removed
    bool remove_handle(j6_handle_t handle);

    /// Lookup an object for a handle
    /// \args handle The handle to the object
    /// \returns     Pointer to the handle struct, or null if not found
    handle * lookup_handle(j6_handle_t handle);

    /// Get the list of handle ids this process owns
    /// \arg handles  Pointer to an array of handles to copy into
    /// \arg len      Size of the array
    /// \returns      Total number of handles (may be more than number copied)
    size_t list_handles(j6_handle_t *handles, size_t len);

    /// Inform the process of an exited thread
    /// \args th  The thread which has exited
    /// \returns  True if this thread ending has ended the process
    bool thread_exited(thread *th);

    /// Get the handle for this process to refer to itself
    inline j6_handle_t self_handle() const { return m_self_handle; }

    /// Get the process object that owns kernel threads and the
    /// kernel address space
    static process & kernel_process();

    /// Create the special kernel process that owns kernel tasks
    /// \arg pml4 The kernel-only pml4
    /// \returns  The kernel process object
    static process * create_kernel_process(page_table *pml4);

private:
    // This constructor is called by create_kernel_process
    process(page_table *kpml4);

    j6_handle_t m_self_handle;
    int32_t m_return_code;

    vm_space m_space;

    util::vector<thread*> m_threads;

    j6_handle_t m_next_handle;
    util::map<j6_handle_t, handle> m_handles;

    enum class state : uint8_t { running, exited };
    state m_state;
};

} // namespace obj
