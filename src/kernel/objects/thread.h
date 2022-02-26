#pragma once
/// \file thread.h
/// Definition of thread kobject types

#include <j6/caps.h>
#include <util/enum_bitfields.h>
#include <util/linked_list.h>
#include <util/spinlock.h>

#include "objects/kobject.h"

struct cpu_data;
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

    uint32_t time_left;
    uint64_t last_ran;

    uintptr_t kernel_stack;
    cpu_data *cpu;
};

using tcb_list = util::linked_list<TCB>;
using tcb_node = tcb_list::item_type;


namespace obj {

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

    /// Get the `exited` state of the thread.
    /// \returns True if the thread has exited.
    inline bool exited() const { return has_state(state::exited); }

    /// Get the thread priority.
    inline uint8_t priority() const { return m_tcb.priority; }

    /// Set the thread priority.
    /// \arg p  The new thread priority
    inline void set_priority(uint8_t p) { if (!constant()) m_tcb.priority = p; }

    /// Block this thread, waiting for a value
    /// \returns  The value passed to wake()
    uint64_t block();

    /// Wake this thread, giving it a value
    /// \arg value  The value that block() should return
    void wake(uint64_t value = 0);

    /// Set this thread as awake, but do not call the scheduler
    /// or set the wake value.
    void wake_only();

    /// Set a timeout to unblock this thread
    /// \arg time  The clock time at which to wake. 0 for no timeout.
    inline void set_wake_timeout(uint64_t time) { m_wake_timeout = time; }

    /// Get the timeout at which to unblock this thread
    /// \returns  The clock time at which to wake. 0 for no timeout.
    inline uint64_t wake_timeout() const { return m_wake_timeout; }

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

protected:
    /// Don't delete a thread on no handles, the scheduler takes
    /// care of that.
    virtual void on_no_handles() override {}

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

    // There should be 3 bytes of padding here

    int32_t m_return_code;

    uint64_t m_wake_value;
    uint64_t m_wake_timeout;

    j6_handle_t m_self_handle;
};

} // namespace obj
