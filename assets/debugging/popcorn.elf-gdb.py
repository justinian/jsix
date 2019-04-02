import gdb

class PrintStackCommand(gdb.Command):
    def __init__(self):
        super().__init__("popc_stack", gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        args = gdb.string_to_argv(arg)

        base = "$rsp"
        if len(args) > 0:
            base = args[0]

        depth = 22
        if len(args) > 1:
            depth = int(args[1])

        for i in range(depth-1, -1, -1):
            offset = i * 8
            base_addr = gdb.parse_and_eval(base)
            value = gdb.parse_and_eval(f"*(uint64_t*)({base} + {offset})")
            print("{:016x} (+{:04x}): {:016x}".format(int(base_addr) + offset, offset, int(value)))


PrintStackCommand()

import time
time.sleep(3.5)
gdb.execute("target remote :1234")
gdb.execute("set waiting = false")
gdb.execute("display/i $rip")
