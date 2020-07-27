#include "j6/errors.h"
#include "j6/types.h"

#include "objects/channel.h"
#include "objects/thread.h"
#include "objects/process.h"
#include "scheduler.h"

namespace syscalls {

j6_status_t
channel_create(j6_handle_t *handle)
{
	scheduler &s = scheduler::get();
	TCB *tcb = s.current();
	thread *parent = thread::from_tcb(tcb);
	process &p = parent->parent();

	channel *c = new channel;
	*handle = p.add_handle(c);

	return j6_status_ok;
}

j6_status_t
channel_close(j6_handle_t handle)
{
	scheduler &s = scheduler::get();
	TCB *tcb = s.current();
	thread *parent = thread::from_tcb(tcb);
	process &p = parent->parent();

	kobject *o = p.lookup_handle(handle);
	if (!o || o->get_type() != kobject::type::channel)
		return j6_err_invalid_arg;

	p.remove_handle(handle);
	channel *c = static_cast<channel*>(o);
	c->close();

	return j6_status_ok;
}

j6_status_t
channel_send(j6_handle_t handle, size_t len, void *data)
{
	scheduler &s = scheduler::get();
	TCB *tcb = s.current();
	thread *parent = thread::from_tcb(tcb);
	process &p = parent->parent();

	kobject *o = p.lookup_handle(handle);
	if (!o || o->get_type() != kobject::type::channel)
		return j6_err_invalid_arg;

	channel *c = static_cast<channel*>(o);
	return c->enqueue(len, data);
}

j6_status_t
channel_receive(j6_handle_t handle, size_t *len, void *data)
{
	scheduler &s = scheduler::get();
	TCB *tcb = s.current();
	thread *parent = thread::from_tcb(tcb);
	process &p = parent->parent();

	kobject *o = p.lookup_handle(handle);
	if (!o || o->get_type() != kobject::type::channel)
		return j6_err_invalid_arg;

	channel *c = static_cast<channel*>(o);
	return c->dequeue(len, data);
}

} // namespace syscalls
