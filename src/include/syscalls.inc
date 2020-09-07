SYSCALL(0x00, system_log,       const char *)
SYSCALL(0x01, system_noop,      void)

SYSCALL(0x09, object_wait,      j6_handle_t, j6_signal_t, j6_signal_t *)
SYSCALL(0x0a, object_signal,     j6_handle_t, j6_signal_t)

SYSCALL(0x10, process_koid,      j6_koid_t *)
SYSCALL(0x11, process_exit,      int64_t)

SYSCALL(0x18, thread_koid,       j6_koid_t *)
SYSCALL(0x19, thread_create,     void *, j6_handle_t *)
SYSCALL(0x1a, thread_exit,       int64_t)
SYSCALL(0x1b, thread_pause,      void)
SYSCALL(0x1c, thread_sleep,      uint64_t)

SYSCALL(0x20, channel_create,    j6_handle_t *)
SYSCALL(0x21, channel_close,     j6_handle_t)
SYSCALL(0x22, channel_send,      j6_handle_t, size_t *, void *)
SYSCALL(0x23, channel_receive,   j6_handle_t, size_t *, void *)

SYSCALL(0x28, endpoint_create,   j6_handle_t *)
SYSCALL(0x29, endpoint_close,    j6_handle_t)
SYSCALL(0x2a, endpoint_send,     j6_handle_t, size_t, void *)
SYSCALL(0x2b, endpoint_receive,  j6_handle_t, size_t *, void *)
SYSCALL(0x2c, endpoint_sendrecv, j6_handle_t, size_t *, void *)

