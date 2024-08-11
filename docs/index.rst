.. jsix documentation master file
.. |amd64| replace:: :abbr:`amd64 (aka x86_64)`

The jsix Operating System
=========================
Introduction
------------

**jsix** is a custom multi-core x64 operating system being built from scratch,
supporting modern [#]_ Intel or AMD CPUs, and UEFI firmware. It was initially
created out of a desire to explore UEFI and to explore what's possible with a
microkernel architecture on modern 64-bit architectures.

Most of jsix is written in C++ (C++17, using `LLVM <https://llvm.org>`_), but
you'll also find some assembly (in `NASM <https://nasm.us>`_ syntax) and Python
for development tooling.

jsix can be found `on GitHub <https://github.com/justinian/jsix>`_, and is
released under the terms of the `MPL 2.0 <https://mozilla.org/MPL/2.0/>`_.

.. admonition:: A note on the name

    This kernel was originally named Popcorn, but I have since discovered that
    the Popcorn Linux project is also developing a kernel with that name,
    started around the same time as this project. So I've renamed this kernel
    jsix as an homage to L4, xv6, and my wonderful wife.

    The name jsix is always styled *jsix* or ``j6``, never capitalized.

.. [#] jsix aims to support amd64 (x86_64) CPUs released in the last 10 years.

Current Features
----------------

The jsix kernel is quite far along now, but the userland systems are still lacking.

- Platform: |amd64|
- UEFI bootloader
- Multi-core & multi-tasking microkernel

  - Work-stealing SMP scheduler
  - Pluggable panic handler modules

- Capability-style object-oriented syscall API

  - Custom IDL for specifying and documenting syscalls

- Virtual memory based on sharable Virtual Memory Area objects (VMAs)
- Kernel API library (libj6), also provides features built on kernel primitives:

  - Channels (async stream IPC) built on shared memory and futexes
  - Ring buffers via doubly-mapped pages

- Custom libc
- Runtime dynamic linker
- Init service

  - Built-in VFS service for the initrd
  - ELF loader
  - Service-lookup protocol service

- Userland UART driver
- Userland UEFI framebuffer driver
- Userland kernel log output service
- Userland unit test runner
- Build configuration system (bonnibel)


.. toctree::
   :maxdepth: 1
   :caption: Site Contents:

   syscall_interface
   kernel_memory
   process_initialization


* :ref:`genindex`
* :ref:`search`


