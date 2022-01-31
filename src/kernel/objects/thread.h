#pragma once
/// \file thread.h
/// Definition of thread kobject types

#include <j6/caps.h>
#include <util/enum_bitfields.h>
#include <util/linked_list.h>
#include <util/spinlock.h>

#include "objects/kobject.h"

struct page_table;

namespace obj {
    class thread;
}

struct TCB
{
    // Data used by assembly task control routines.  If you change any of these,
    // be sure to change the assembly definitions in 'tasking.inc'
    uintptr_t rsp;
    uintptr_t rsp0;
    uintptr_t rsp3;
    uintptr_t rflags3;
    uintptr_t pml4;
    // End of area used by asembly

    obj::thread* thread;

    uint8_t priority;
    // note: 3 bytes padding

    // TODO: move state into TCB?

    uintptr_t kernel_stack;

    uint32_t time_left;
    uint64_t last_ran;
};

using tcb_list = util::linked_list<TCB>;
using tcb_node = tcb_list::item_type;


namespace obj {

enum class wait_type : uint8_t
{
    none   = 0x00,
    signal = 0x01,
    time   = 0x02,
    object = 0x04,
};
is_bitfield(wait_type);

class process;

class thread :
    public kobject
{
public:
    /// Capabilities on a newly constructed thread handle
    constexpr static j6_cap_t creation_caps = j6_cap_thread_all;

    /// Capabilities the parent process gets on new thread handles
    constexpr static j6_cap_t parent_caps = j6_cap_thread_all;

    enum class state : uint8_t {
        none     = 0x00,
        ready    = 0x01,
        exited   = 0x02,

        constant = 0x80
    };

    /// Destructor
    virtual ~thread();

    static constexpr kobject::type type = kobject::type::thread;

    /// Get the currently executing thread.
    static thread & current();

    /// Get the `ready` state of the thread.
    /// \returns True if the thread is ready to execute.
    inline bool ready() const { return has_state(state::ready); }

    /// Get the `constant` state of the thread.
    /// \returns True if the thread has constant priority.
    inline bool constant() const { return has_state(state::constant); }

    /// Get the thread priority.
    inline uint8_t priority() const { return m_tcb.priority; }

    /// Set the thread priority.
    /// \arg p  The new thread priority
    inline void set_priority(uint8_t p) { if (!constant()) m_tcb.priority = p; }

    /// Block the thread, waiting an object's signals.
    /// \arg signals Mask of signals to wait for
    void wait_on_signals(j6_signal_t signals);

    /// Block the thread, waiting for a given clock value
    /// \arg t  Clock value to wait for
    void wait_on_time(uint64_t t);

    /// Block the thread, waiting on the given object
    /// \arg o  The object that should wake this thread
    /// \arg t  The timeout clock value to wait for
    void wait_on_object(kobject *o, uint64_t t = 0);

    /// Wake the thread if it is waiting on signals.
    /// \arg obj     Object that changed signals
    /// \arg signals Signal state of the object
    /// \returns     True if this action unblocked the thread
    bool wake_on_signals(kobject *obj, j6_signal_t signals);

    /// Wake the thread if it is waiting on the clock.
    /// \arg now  Current clock value
    /// \returns  True if this action unblocked the thread
    bool wake_on_time(uint64_t now);

    /// Wake the thread if it is waiting on the given object.
    /// \arg o  Object trying to wake the thread
    /// \returns  True if this action unblocked the thread
    bool wake_on_object(kobject *o);

    /// Wake the thread with a given result code.
    /// \arg obj     Object that changed signals
    /// \arg result  Result code to return to the thread
    void wake_on_result(kobject *obj, j6_status_t result);

    /// Get the result status code from the last blocking operation
    j6_status_t get_wait_result() const { return m_wait_result; }

    /// Get the current blocking opreation's wait data
    uint64_t get_wait_data() const { return m_wait_data; }

    /// Get the current blocking operation's wait ojbect (as a handle)
    j6_koid_t get_wait_object() const { return m_wait_obj; }

    inline bool has_state(state s) const {
        return static_cast<uint8_t>(m_state) & static_cast<uint8_t>(s);
    }

    inline void set_state(state s) {
        m_state = static_cast<state>(static_cast<uint8_t>(m_state) | static_cast<uint8_t>(s));
    }

    inline void clear_state(state s) {
        m_state = static_cast<state>(static_cast<uint8_t>(m_state) & ~static_cast<uint8_t>(s));
    }

    inline tcb_node * tcb() { return &m_tcb; }
    inline process & parent() { return m_parent; }

    /// Terminate this thread.
    /// \arg code   The return code to exit with.
    void exit(int32_t code);

    /// Add a stack header that returns to the given address in kernel space.
    /// \arg rip  The address to return to, must be kernel space
    void add_thunk_kernel(uintptr_t rip);

    /// Add a stack header that returns to the given address in user space
    /// via a function in kernel space.
    /// \arg rip3  The user space address to return to
    /// \arg rip0  The kernel function to pass through, optional
    /// \arg flags Extra RFLAGS values to set, optional
    void add_thunk_user(uintptr_t rip3, uintptr_t rip0 = 0, uint64_t flags = 0);

    /// Get the handle representing this thread to its process
    j6_handle_t self_handle() const { return m_self_handle; }

    /// Create the kernel idle thread
    /// \arg kernel The process object that owns kernel tasks
    /// \arg pri    The idle thread priority value
    /// \arg rsp    The existing stack for the idle thread
    static thread * create_idle_thread(process &kernel, uint8_t pri, uintptr_t rsp);

private:
    thread() = delete;
    thread(const thread &other) = delete;
    thread(const thread &&other) = delete;
    friend class process;

    /// Constructor. Used when a kernel stack already exists.
    /// \arg parent  The process which owns this thread
    /// \arg pri     Initial priority level of this thread
    /// \arg rsp0    The existing kernel stack rsp, 0 for none
    thread(process &parent, uint8_t pri, uintptr_t rsp0 = 0);

    /// Set up a new empty kernel stack for this thread.
    void setup_kernel_stack();

    tcb_node m_tcb;

    process &m_parent;
    thread *m_creator;

    state m_state;
    wait_type m_wait_type;
    // There should be 1 byte of padding here

    int32_t m_return_code;

    uint64_t m_wait_data;
    uint64_t m_wait_time;
    j6_status_t m_wait_result;
    j6_koid_t m_wait_obj;
    util::spinlock m_wait_lock;

    j6_handle_t m_self_handle;
};

} // namespace obj
