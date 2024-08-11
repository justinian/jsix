.. jsix syscall interface.
.. Automatically updated from the definition files using cog!

.. [[[cog code generation
.. from textwrap import indent
.. from definitions.context import Context
.. 
.. ctx = Context(definitions_path)
.. ctx.parse("syscalls.def")
.. syscalls = ctx.interfaces["syscalls"]
.. 
.. def caplist(caps):
..   return ', '.join([f"``{c}``" for c in caps])
.. ]]]
.. [[[end]]] (checksum: d41d8cd98f00b204e9800998ecf8427e)

Syscall interface
=================

The jsix kernel's syscall design is based around object handles. Object handles
are also a collection of capabilities, encoding certain rights over the object
they reference.

Very few syscalls in jsix can be made without some handle, and most of them are
requests to the kernel to create a given kind of object. This is analogous to
methods on an object in an object-oriented programming language.

.. [[[cog code generation
.. cog.outl()
.. for obj in syscalls.exposes:
..     cog.outl(f"``{obj.name}`` syscalls")
..     cog.outl(f"-------------------------")
..     desc = obj.desc or "Undocumented"
..     cog.outl(desc)
..     cog.outl()
..     cog.outl(f":capabilites:  {caplist(obj.caps)}")
..     cog.outl()
..     for method in obj.methods:
..       args = []
..       if method.constructor:
..         args.append("j6_handle_t *self")
..       elif not method.static:
..         args.append("j6_handle_t self")
..
..       for param in method.params:
..         for type, suffix in param.type.c_names(param.options):
..           args.append(f"{type} {param.name}{suffix}")
..
..       cog.outl(f".. cpp:function:: j6_result_t j6_{obj.name}_{method.name} ({', '.join(args)})")
..       cog.outl()
..       desc = method.desc or "Undocumented"
..       cog.outl(indent(desc, "   "))
..       cog.outl()
..       if "cap" in method.options:
..         cog.outl(f"   :capabilities: {caplist(method.options['cap'])}")
..         cog.outl()
..       if method.constructor:
..         cog.outl(f"   :param self: *[out]* Handle to the new {obj.name} object")
..       elif not method.static:
..         cog.outl(f"   :param self: Handle to the {obj.name} object")
..       for param in method.params:
..         opts = param.options and f"*[{', '.join(param.options)}]*" or ""
..         desc = param.desc or 'Undocumented'
..         cog.outl(f"   :param {param.name}: {opts} {desc}")
..       cog.outl()
.. ]]]

``object`` syscalls
-------------------------
All kernel-exposed objects inherit from the base ``object`` type, so the
``object`` syscalls can be used with any object's handle.

:capabilites:  ``clone``

.. cpp:function:: j6_result_t j6_object_koid (j6_handle_t self, uint64_t * koid)

   Get the internal kernel object id of an object

   :param self: Handle to the object object
   :param koid: *[out]* Undocumented

``event`` syscalls
-------------------------
An ``event`` is a simple synchronization object. It contains up to 64 signals
that threads can wait for and signal in parallel.

:capabilites:  ``signal``, ``wait``

.. cpp:function:: j6_result_t j6_event_create (j6_handle_t *self)

   Undocumented

   :param self: *[out]* Handle to the new event object

.. cpp:function:: j6_result_t j6_event_signal (j6_handle_t self, uint64_t signals)

   Signal events on this object

   :capabilities: ``signal``

   :param self: Handle to the event object
   :param signals:  A bitset of which events to signal

.. cpp:function:: j6_result_t j6_event_wait (j6_handle_t self, uint64_t * signals, uint64_t timeout)

   Wait for signaled events on this object

   :capabilities: ``wait``

   :param self: Handle to the event object
   :param signals: *[out]* A bitset of which events were signaled
   :param timeout:  Wait timeout in nanoseconds

``mailbox`` syscalls
-------------------------
Mailboxes are objects that enable synchronous IPC via arbitrary
message-passing of tagged data and/or handles. Not as efficient
as shared memory channels, but more flexible.

:capabilites:  ``send``, ``receive``, ``close``

.. cpp:function:: j6_result_t j6_mailbox_create (j6_handle_t *self)

   Undocumented

   :param self: *[out]* Handle to the new mailbox object

.. cpp:function:: j6_result_t j6_mailbox_close (j6_handle_t self)

   Undocumented

   :capabilities: ``close``

   :param self: Handle to the mailbox object

.. cpp:function:: j6_result_t j6_mailbox_call (j6_handle_t self, uint64_t * tag, void * data, size_t * data_len, size_t data_size, j6_handle_t * handles, size_t * handles_count, size_t handles_size)

   Send a message to the reciever, and block until a response is
   sent. Note that getting this response does not require the
   receive capability.

   :capabilities: ``send``

   :param self: Handle to the mailbox object
   :param tag: *[inout]* Undocumented
   :param data: *[optional, inout]* Undocumented
   :param data_size:  number of total bytes in data buffer
   :param handles: *[optional, inout, handle, list]* Undocumented
   :param handles_size:  total size of handles buffer

.. cpp:function:: j6_result_t j6_mailbox_respond (j6_handle_t self, uint64_t * tag, void * data, size_t * data_len, size_t data_size, j6_handle_t * handles, size_t * handles_count, size_t handles_size, uint64_t * reply_tag, uint64_t flags)

   Respond to a message sent using call, and wait for another
   message to arrive. Note that this does not require the send
   capability. A reply tag of 0 skips the reply and goes directly
   to waiting for a new message.

   :capabilities: ``receive``

   :param self: Handle to the mailbox object
   :param tag: *[inout]* Undocumented
   :param data: *[optional, inout]* Undocumented
   :param data_size:  number of total bytes in data buffer
   :param handles: *[optional, inout, handle, list]* Undocumented
   :param handles_size:  total size of handles buffer
   :param reply_tag: *[inout]* Undocumented
   :param flags:  Undocumented

``process`` syscalls
-------------------------
A ``process`` object represents a process running on the system, and allows
control over the threads, handles, and virtual memory space of that process.

:capabilites:  ``kill``, ``create_thread``

.. cpp:function:: j6_result_t j6_process_create (j6_handle_t *self)

   Create a new empty process

   :param self: *[out]* Handle to the new process object

.. cpp:function:: j6_result_t j6_process_kill (j6_handle_t self)

   Stop all threads and exit the given process

   :capabilities: ``kill``

   :param self: Handle to the process object

.. cpp:function:: j6_result_t j6_process_exit (int64_t result)

   Stop all threads and exit the current process

   :param result:  The result to retrun to the parent process

.. cpp:function:: j6_result_t j6_process_give_handle (j6_handle_t self, j6_handle_t target)

   Give the given process a handle that points to the same
   object as the specified handle.

   :param self: Handle to the process object
   :param target: *[handle]* A handle in the caller process to send

``system`` syscalls
-------------------------
The singular ``system`` object represents a handle to kernel functionality
needed by drivers and other priviledged services.

:capabilites:  ``get_log``, ``bind_irq``, ``map_phys``, ``change_iopl``

.. cpp:function:: j6_result_t j6_system_get_log (j6_handle_t self, uint64_t seen, void * buffer, size_t * buffer_len)

   Get the next log line from the kernel log

   :capabilities: ``get_log``

   :param self: Handle to the system object
   :param seen:  Last seen log id
   :param buffer: *[out, zero_ok]* Buffer for the log message data structure

.. cpp:function:: j6_result_t j6_system_bind_irq (j6_handle_t self, j6_handle_t dest, unsigned irq, unsigned signal)

   Ask the kernel to send this process messages whenever
   the given IRQ fires

   :capabilities: ``bind_irq``

   :param self: Handle to the system object
   :param dest:  Event object that will receive messages
   :param irq:  IRQ number to bind
   :param signal:  Signal number on the event to bind to

.. cpp:function:: j6_result_t j6_system_map_phys (j6_handle_t self, j6_handle_t * area, uintptr_t phys, size_t size, uint32_t flags)

   Create a VMA and map an area of physical memory into it,
   also mapping that VMA into the current process

   :capabilities: ``map_phys``

   :param self: Handle to the system object
   :param area: *[out]* Receives a handle to the VMA created
   :param phys:  The physical address of the area
   :param size:  Size of the area, in bytes
   :param flags:  Flags to apply to the created VMA

.. cpp:function:: j6_result_t j6_system_request_iopl (j6_handle_t self, unsigned iopl)

   Request the kernel change the IOPL for this process. The only values
   that make sense are 0 and 3.

   :capabilities: ``change_iopl``

   :param self: Handle to the system object
   :param iopl:  The IOPL to set for this process

``thread`` syscalls
-------------------------
A ``thread`` object represents a thread of execution within a process running
on the system. The actual thread does not need to be currently running to
hold a handle to it.

:capabilites:  ``kill``, ``join``

.. cpp:function:: j6_result_t j6_thread_create (j6_handle_t *self, j6_handle_t process, uintptr_t stack_top, uintptr_t entrypoint, uint64_t arg0, uint64_t arg1)

   Undocumented

   :param self: *[out]* Handle to the new thread object
   :param process: *[optional, cap]* Undocumented
   :param stack_top:  Undocumented
   :param entrypoint:  Undocumented
   :param arg0:  Undocumented
   :param arg1:  Undocumented

.. cpp:function:: j6_result_t j6_thread_kill (j6_handle_t self)

   Undocumented

   :capabilities: ``kill``

   :param self: Handle to the thread object

.. cpp:function:: j6_result_t j6_thread_join (j6_handle_t self)

   Undocumented

   :capabilities: ``join``

   :param self: Handle to the thread object

.. cpp:function:: j6_result_t j6_thread_exit ()

   Undocumented


.. cpp:function:: j6_result_t j6_thread_sleep (uint64_t duration)

   Undocumented

   :param duration:  Undocumented

``vma`` syscalls
-------------------------
A ``vma`` object represents a single virtual memory area, which may be shared
between several processes. A process having a handle to a ``vma`` does not
necessarily mean that it is mapped into that process' virtual memory space.

:capabilites:  ``map``, ``unmap``, ``resize``

.. cpp:function:: j6_result_t j6_vma_create (j6_handle_t *self, size_t size, uint32_t flags)

   Undocumented

   :param self: *[out]* Handle to the new vma object
   :param size:  Undocumented
   :param flags:  Undocumented

.. cpp:function:: j6_result_t j6_vma_create_map (j6_handle_t *self, size_t size, uintptr_t * address, uint32_t flags)

   Undocumented

   :capabilities: ``map``

   :param self: *[out]* Handle to the new vma object
   :param size:  Undocumented
   :param address: *[inout]* Undocumented
   :param flags:  Undocumented

.. cpp:function:: j6_result_t j6_vma_map (j6_handle_t self, j6_handle_t process, uintptr_t * address, uint32_t flags)

   Undocumented

   :capabilities: ``map``

   :param self: Handle to the vma object
   :param process: *[optional]* Undocumented
   :param address: *[inout]* Undocumented
   :param flags:  Undocumented

.. cpp:function:: j6_result_t j6_vma_unmap (j6_handle_t self, j6_handle_t process)

   Undocumented

   :capabilities: ``unmap``

   :param self: Handle to the vma object
   :param process: *[optional]* Undocumented

.. cpp:function:: j6_result_t j6_vma_resize (j6_handle_t self, size_t * size)

   Undocumented

   :capabilities: ``resize``

   :param self: Handle to the vma object
   :param size: *[inout]* New size for the VMA, or 0 to query the current size without changing

.. [[[end]]] (checksum: cb17f54e443d1d3b85995870f3e8dbf2)

Non-object syscalls
-------------------

The following are the system calls that aren't constructors for objects, and
either do not require an object handle, or operate generically on handles.

.. [[[cog code generation
.. cog.outl()
.. for func in syscalls.functions:
..   args = []
..   for param in func.params:
..     for type, suffix in param.type.c_names(param.options):
..       args.append(f"{type} {param.name}{suffix}")
..
..   cog.outl(f".. cpp:function:: j6_result_t j6_{func.name} ({', '.join(args)})")
..   cog.outl()
..   desc = func.desc or "Undocumented"
..   cog.outl(indent(desc, "   "))
..   cog.outl()
..   for param in func.params:
..     opts = param.options and f"*[{', '.join(param.options)}]*" or ""
..     desc = param.desc or 'Undocumented'
..     cog.outl(f"   :param {param.name}: {opts} {desc}")
..   cog.outl()
.. ]]]

.. cpp:function:: j6_result_t j6_noop ()

   Simple no-op syscall for testing


.. cpp:function:: j6_result_t j6_log (uint8_t area, uint8_t severity, const char * message)

   Write a message to the kernel log

   :param area:  Undocumented
   :param severity:  Undocumented
   :param message:  Undocumented

.. cpp:function:: j6_result_t j6_handle_list (struct j6_handle_descriptor * handles, size_t * handles_size)

   Get a list of handles owned by this process. If the
   supplied list is not big enough, will set the size
   needed in `size` and return j6_err_insufficient

   :param handles: *[list, inout, zero_ok]* A list of handles to be filled

.. cpp:function:: j6_result_t j6_handle_clone (j6_handle_t orig, j6_handle_t * clone, uint32_t mask)

   Create a clone of an existing handle, possibly with
   some capabilities masked out.

   :param orig: *[handle, cap]* The handle to clone
   :param clone: *[out]* The new handle
   :param mask:  The capability bitmask

.. cpp:function:: j6_result_t j6_handle_close (j6_handle_t hnd)

   Close the handle to an object

   :param hnd: *[handle]* The handle to close

.. cpp:function:: j6_result_t j6_futex_wait (const uint32_t * address, uint32_t current, uint64_t timeout)

   Block waiting on a futex

   :param address:  Address of the futex value
   :param current:  Current value of the futex
   :param timeout:  Wait timeout in nanoseconds

.. cpp:function:: j6_result_t j6_futex_wake (const uint32_t * address, uint64_t count)

   Wake threads waiting on a futex

   :param address:  Address of the futex value
   :param count:  Number of threads to wake, or 0 for all

.. cpp:function:: j6_result_t j6_test_finish (uint32_t exit_code)

   Testing mode only: Have the kernel finish and exit QEMU with the given exit code

   :param exit_code:  Undocumented

.. [[[end]]] (checksum: 0b9d051972abcbb6de408f411331785f)

