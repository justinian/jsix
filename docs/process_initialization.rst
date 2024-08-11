.. jsix process initialization in userspace

Process Initialization
======================

jsix follows the `System V ABI`_ on the ``amd64`` architecture. All arguments
needed for program initialization are passed to the program's initial thread on
the stack.

Note that jsix adds a number of additional auxiliary vector entry types for
passing jsix-specific data to a program. The jsix-specific auxiliary vector type
codes (what the ABI document refers to as ``a_type``) start from ``0xf000``. See
the header file ``<j6/init.h>`` for more detail.

.. _System V ABI: https://gitlab.com/x86-psABIs/x86-64-ABI

The initial stack frame
-----------------------

==============  ====================  ============  =======
Address         Value                 Bytes         Notes
==============  ====================  ============  =======
``top``                                             Stack top (out of stack bounds)
``top`` - 16    0                     16            Stack sentinel
\               ``envp`` string data  ?
\               ``argv`` string data  ?
\               ...                   ?             Possible padding
\               0, 0 (``AT_NULL``)    16            Aux vector sentinel
\               Aux vectors           16 * `m`      ``AT_NULL``-terminated array of Aux vectors
\               0                     8             Environment sentinel
\                ``envp``             8 * `n`       0-terminated array of environment
                                                    string pointers
\               0                     8             Args sentinel
\               ``argv``              8 * ``argc``  Pointers to argument strings
``rsp``         ``argc``              8             Number of elements in argv
==============  ====================  ============  =======


* :ref:`genindex`
* :ref:`search`


