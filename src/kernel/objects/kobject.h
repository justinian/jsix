#pragma once
/// \file kobject.h
/// Definition of base type for user-interactable kernel objects

#include "j6/errors.h"
#include "j6/types.h"
#include "kutil/vector.h"

/// Base type for all user-interactable kernel objects
class kobject
{
public:
	/// Types of kernel objects.
	enum class type : uint16_t
	{
		none,

		event,
		eventpair,

		vms,
		vmo,

		job,
		process,
		thread,
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

	/// Set the given signals active on this object
	/// \arg s  The set of signals to assert
	void assert_signal(j6_signal_t s);

	/// Clear the given signals on this object
	/// \arg s  The set of signals to deassert
	void deassert_signal(j6_signal_t s);

	class observer
	{
	public:
		/// Callback for when signals change.
		/// \arg obj     The object triggering the callback
		/// \arg s       The current state of obj's signals
		/// \arg ds      Which signals caused the callback
		/// \arg result  Status code for this notification
		/// \returns     True if this object wants to keep watching signals
		virtual bool on_signals_changed(
				kobject *obj,
				j6_signal_t s,
				j6_signal_t ds,
				j6_status_t result) = 0;
	};

	/// Register a signal observer
	/// \arg object  The observer
	/// \arg s       The signals the object wants notifiations for
	void register_signal_observer(observer *object, j6_signal_t s);

	/// Deegister a signal observer
	/// \arg object  The observer
	void deregister_signal_observer(observer *object);

	/// Increment the handle refcount
	inline void handle_retain() { ++m_handle_count; }

	/// Decrement the handle refcount
	inline void handle_release() {
		if (--m_handle_count == 0) on_no_handles();
	}

protected:
	kobject() = delete;
	kobject(const kobject &other) = delete;
	kobject(const kobject &&other) = delete;

	/// Notifiy observers of this object
	/// \arg mask    The signals that triggered this call. If 0, notify all observers.
	/// \arg result  The result to pass to the observers
	void notify_signal_observers(j6_signal_t mask, j6_status_t result = j6_status_ok);

	/// Interface for subclasses to handle when all handles are closed. Subclasses
	/// should either call the base version, or assert j6_signal_no_handles.
	virtual void on_no_handles();

	j6_koid_t m_koid;
	j6_signal_t m_signals;
	uint16_t m_handle_count;

	struct observer_registration
	{
		observer_registration(observer* o = nullptr, j6_signal_t s = 0) :
			object(o), signals(s) {}

		observer *object;
		j6_signal_t signals;
	};
	kutil::vector<observer_registration> m_observers;
};
