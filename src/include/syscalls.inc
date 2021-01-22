SYSCALL(0x00, system_log,        const char *)
SYSCALL(0x01, system_noop,       void)
SYSCALL(0x02, system_get_log,    j6_handle_t, char *, size_t *)
SYSCALL(0x03, system_bind_irq,   j6_handle_t, j6_handle_t, unsigned)

SYSCALL(0x08, object_koid,       j6_handle_t, j6_koid_t *)
SYSCALL(0x09, object_wait,       j6_handle_t, j6_signal_t, j6_signal_t *)
SYSCALL(0x0a, object_signal,     j6_handle_t, j6_signal_t)
SYSCALL(0x0b, object_close,      j6_handle_t)

SYSCALL(0x10, process_create,    j6_handle_t *)
SYSCALL(0x11, process_start,     j6_handle_t *, uintptr_t)
SYSCALL(0x11, process_kill,      j6_handle_t)
SYSCALL(0x17, process_exit,      int32_t)

SYSCALL(0x18, thread_create,     void *, j6_handle_t *)
SYSCALL(0x19, thread_exit,       int32_t)
SYSCALL(0x1a, thread_pause,      void)
SYSCALL(0x1b, thread_sleep,      uint64_t)

SYSCALL(0x20, channel_create,    j6_handle_t *)
SYSCALL(0x21, channel_send,      j6_handle_t, size_t *, void *)
SYSCALL(0x22, channel_receive,   j6_handle_t, size_t *, void *)

SYSCALL(0x28, endpoint_create,   j6_handle_t *)
SYSCALL(0x29, endpoint_send,     j6_handle_t, j6_tag_t, size_t, void *)
SYSCALL(0x2a, endpoint_receive,  j6_handle_t, j6_tag_t *, size_t *, void *)
SYSCALL(0x2b, endpoint_sendrecv, j6_handle_t, j6_tag_t *, size_t *, void *)

SYSCALL(0x30, vma_create,        j6_handle_t *, size_t, uint32_t)
SYSCALL(0x31, vma_create_map,    j6_handle_t *, size_t, uintptr_t, uint32_t)
SYSCALL(0x32, vma_map,           j6_handle_t, j6_handle_t, uintptr_t)
SYSCALL(0x33, vma_unmap,         j6_handle_t, j6_handle_t)
SYSCALL(0x34, vma_resize,        j6_handle_t, size_t *)
