SYSCALL(0x00, noop,     void)
SYSCALL(0x01, exit,     void, int64_t)
SYSCALL(0x02, getpid,   pid_t)
SYSCALL(0x03, fork,     pid_t)

SYSCALL(0x10, message,  void, const char *)

SYSCALL(0x20, pause,    void)
SYSCALL(0x21, sleep,    void, uint64_t)

SYSCALL(0x30, send,     void)
SYSCALL(0x31, receive,  void)
