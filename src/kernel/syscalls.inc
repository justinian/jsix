SYSCALL(0x00, object_noop,      void)
SYSCALL(0x01, object_wait,      j6_handle_t, j6_signal_t, j6_signal_t *)

SYSCALL(0x10, process_koid,     j6_koid_t *)
SYSCALL(0x11, process_exit,     int64_t)
SYSCALL(0x12, process_log,      const char *)
SYSCALL(0x13, process_pause,    void)
SYSCALL(0x14, process_sleep,    uint64_t)
