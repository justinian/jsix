#include "j6/errors.h"
#include "j6/types.h"

#include "log.h"
#include "objects/endpoint.h"
#include "objects/process.h"
#include "scheduler.h"

namespace syscalls {

j6_status_t
endpoint_create(j6_handle_t *handle)
{
	scheduler &s = scheduler::get();
	TCB *tcb = s.current();
	thread *parent = thread::from_tcb(tcb);
	process &p = parent->parent();

	endpoint *e = new endpoint;
	*handle = p.add_handle(e);

	return j6_status_ok;
}

j6_status_t
endpoint_close(j6_handle_t handle)
{
	scheduler &s = scheduler::get();
	TCB *tcb = s.current();
	thread *parent = thread::from_tcb(tcb);
	process &p = parent->parent();

	kobject *o = p.lookup_handle(handle);
	if (!o || o->get_type() != kobject::type::endpoint)
		return j6_err_invalid_arg;

	p.remove_handle(handle);
	endpoint *e = static_cast<endpoint*>(o);
	e->close();

	return j6_status_ok;
}

j6_status_t
endpoint_send(j6_handle_t handle, size_t len, void *data)
{
	scheduler &s = scheduler::get();
	TCB *tcb = s.current();
	thread *parent = thread::from_tcb(tcb);
	process &p = parent->parent();

	kobject *o = p.lookup_handle(handle);
	if (!o || o->get_type() != kobject::type::endpoint)
		return j6_err_invalid_arg;

	endpoint *e = static_cast<endpoint*>(o);
	j6_status_t status = e->send(len, data);
	return status;
}

j6_status_t
endpoint_receive(j6_handle_t handle, size_t *len, void *data)
{
	scheduler &s = scheduler::get();
	TCB *tcb = s.current();
	thread *parent = thread::from_tcb(tcb);
	process &p = parent->parent();

	kobject *o = p.lookup_handle(handle);
	if (!o || o->get_type() != kobject::type::endpoint)
		return j6_err_invalid_arg;

	endpoint *e = static_cast<endpoint*>(o);
	j6_status_t status = e->receive(len, data);
	return status;
}

j6_status_t
endpoint_sendrecv(j6_handle_t handle, size_t *len, void *data)
{
	scheduler &s = scheduler::get();
	TCB *tcb = s.current();
	thread *parent = thread::from_tcb(tcb);
	process &p = parent->parent();

	kobject *o = p.lookup_handle(handle);
	if (!o || o->get_type() != kobject::type::endpoint)
		return j6_err_invalid_arg;

	endpoint *e = static_cast<endpoint*>(o);
	j6_status_t status = e->send(*len, data);
	if (status != j6_status_ok)
		return status;

	status = e->receive(len, data);
	return status;
}

} // namespace syscalls
