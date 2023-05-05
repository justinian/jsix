#include <stdint.h>
#include <stdlib.h>

#include <j6/channel.hh>
#include <j6/errors.h>
#include <j6/flags.h>
#include <j6/syscalls.h>
#include <j6/syslog.hh>
#include <j6/thread.hh>
#include <j6/types.h>

char inbuf[1024];
extern j6_handle_t __handle_sys;
j6_handle_t endp = j6_handle_invalid;

extern j6_handle_t __handle_self;

constexpr uintptr_t stack_top = 0xf80000000;
uint32_t flipflop = 0;

bool
test_floats()
{
    static constexpr int len = 30;
    double as[len];
    double bs[len];

    double orig = 345.72;
    double mult = 3.21;
    for (int i = 0; i < len * 100; ++i) {
        int idx = i % len;
        as[idx] = orig * idx;
    }
    for (int i = 0; i < len * 100; ++i) {
        int idx = i % len;
        bs[idx] = as[idx] * mult;
    }

    for (int i = 0; i < len; ++i) {
        if (bs[i] != orig * i * mult) {
            j6::syslog("ERROR: floating point discrepency");
            return false;
        }
    }

    return true;
}

void
thread_proc(void* channelp)
{
    j6_log("sub thread starting");

    for (int i = 0; i < 100; ++i)
        if (!test_floats()) break;

    j6::channel *chan = reinterpret_cast<j6::channel*>(channelp);

    char buffer[512];
    size_t len = sizeof(buffer);
    j6_status_t result = chan->receive((void*)buffer, &len);

    __sync_synchronize();
    flipflop = 1;

    if (result != j6_status_ok)
        j6_thread_exit();

    j6_log("sub thread received message");

    for (int i = 0; i < len; ++i)
        if (buffer[i] >= 'A' && buffer[i] <= 'Z')
            buffer[i] += 0x20;

    result = chan->send((void*)buffer, len);
    if (result != j6_status_ok)
        j6_thread_exit();

    j6_log("sub thread sent message");

    for (int i = 1; i < 5; ++i)
        j6_thread_sleep(i*10);

    j6_log("sub thread exiting");
    j6_thread_exit();
}

int
main(int argc, const char **argv)
{
    j6_signal_t out = 0;

    j6_log("main thread starting");

    for (int i = 0; i < argc; ++i)
        j6_log(argv[i]);

    void *base = malloc(0x1000);
    if (!base)
        return 1;

    uint64_t *vma_ptr = reinterpret_cast<uint64_t*>(base);
    for (int i = 0; i < 3; ++i)
        vma_ptr[i*100] = uint64_t(i);

    j6_log("main thread wrote to memory area");

    j6::channel *chan = j6::channel::create(0x2000);
    if (!chan)
        return 2;

    j6_log("main thread created channel");

    j6::thread child_thread {thread_proc, stack_top};
    j6_status_t result = child_thread.start(chan);
    if (result != j6_status_ok)
        return result;

    j6_log("main thread created sub thread");

    for (int i = 0; i < 100; ++i)
        if (!test_floats()) break;

    char message[] = "MAIN THREAD SUCCESSFULLY CALLED SEND AND RECEIVE IF THIS IS LOWERCASE";
    size_t size = sizeof(message);

    result = chan->send((void*)message, size);
    if (result != j6_status_ok)
        return result;

    while (!flipflop);

    result = chan->receive((void*)message, &size);
    if (result != j6_status_ok)
        return result;

    j6_log(message);

    j6_log("main thread waiting on sub thread");
    child_thread.join();

    j6_log("main thread done, exiting");
    return 0;
}

