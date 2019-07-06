SYSCALL(0x00, object_noop,      void)
SYSCALL(0x01, object_wait,      j6_handle_t, j6_signal_t, j6_signal_t *)

SYSCALL(0x11, process_exit,     int64_t)
SYSCALL(0x12, process_fork,     pid_t*)
SYSCALL(0x13, process_getpid,   pid_t*)
SYSCALL(0x14, process_log,      const char *)
SYSCALL(0x15, process_pause,    void)
SYSCALL(0x16, process_sleep,    uint64_t)
