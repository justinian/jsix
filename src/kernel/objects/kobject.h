#pragma once
/// \file kobject.h
/// Definition of base type for user-interactable kernel objects

#include <j6/errors.h>
#include <j6/signals.h>
#include <j6/types.h>
#include <util/deque.h>

namespace obj {

class thread;

/// Base type for all user-interactable kernel objects
class kobject
{
public:
    /// Types of kernel objects.
    enum class type : uint8_t
    {
#define OBJECT_TYPE( name, val ) name = val,
#include <j6/tables/object_types.inc>
#undef OBJECT_TYPE

        max
    };

    kobject(type t, j6_signal_t signals = 0ull);
    virtual ~kobject();

    /// Generate a new koid for a given type
    /// \arg t    The object type
    /// \returns  A new unique koid
    static j6_koid_t koid_generate(type t);

    /// Get the kobject type from a given koid
    /// \arg koid  An existing koid
    /// \returns   The object type for the koid
    static type koid_type(j6_koid_t koid);

    /// Get this object's type
    inline type get_type() const { return koid_type(m_koid); }

    /// Get this object's koid
    inline j6_koid_t koid() const { return m_koid; }

    /// Set the given signals active on this object
    /// \arg s    The set of signals to assert
    /// \returns  The previous state of the signals
    j6_signal_t assert_signal(j6_signal_t s);

    /// Clear the given signals on this object
    /// \arg s    The set of signals to deassert
    /// \returns  The previous state of the signals
    j6_signal_t deassert_signal(j6_signal_t s);

    /// Check if the given signals are set on this object
    /// \arg s  The set of signals to check
    inline bool check_signal(j6_signal_t s) const { return (m_signals & s) == s; }

    /// Get the current object signal state
    inline j6_signal_t signals() const { return m_signals; }

    /// Increment the handle refcount
    inline void handle_retain() { ++m_handle_count; }

    /// Decrement the handle refcount
    inline void handle_release() {
        if (--m_handle_count == 0) on_no_handles();
    }

    /// Add the given thread to the list of threads waiting on this object.
    inline void add_blocked_thread(thread *t) { m_blocked_threads.push_back(t); }

    /// Remove the given thread from the list of threads waiting on this object.
    void remove_blocked_thread(thread *t);

    /// Perform any cleanup actions necessary to mark this object closed
    virtual void close();

    /// Check if this object has been closed
    inline bool closed() const { return check_signal(j6_signal_closed); }

protected:
    /// Interface for subclasses to handle when all handles are closed. Subclasses
    /// should either call the base version, or assert j6_signal_no_handles.
    virtual void on_no_handles();

private:
    kobject() = delete;
    kobject(const kobject &other) = delete;
    kobject(const kobject &&other) = delete;

    /// Notifiy observers of this object
    /// \arg result  The result to pass to the observers
    void notify_signal_observers();

    /// Compact the blocked threads deque
    void compact_blocked_threads();

    j6_koid_t m_koid;
    j6_signal_t m_signals;
    uint16_t m_handle_count;

protected:
    /// Notify threads waiting on this object
    /// \arg count  Number of observers to wake, or -1ull for all
    void notify_object_observers(size_t count = -1ull);

    util::deque<thread*, 8> m_blocked_threads;
};

} // namespace obj
