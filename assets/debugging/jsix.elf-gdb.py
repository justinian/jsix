import gdb

class PrintStackCommand(gdb.Command):
    def __init__(self):
        super().__init__("j6stack", gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        args = gdb.string_to_argv(arg)

        base = "$rsp"
        if len(args) > 0:
            base = args[0]

        depth = 22
        if len(args) > 1:
            depth = int(args[1])

        for i in range(depth-1, -1, -1):
            try:
                offset = i * 8
                base_addr = gdb.parse_and_eval(base)
                value = gdb.parse_and_eval(f"*(uint64_t*)({base} + {offset})")
                print("{:016x} (+{:04x}): {:016x}".format(int(base_addr) + offset, offset, int(value)))
            except Exception as e:
                print(e)
                continue


class PrintBacktraceCommand(gdb.Command):
    def __init__(self):
        super().__init__("j6bt", gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        args = gdb.string_to_argv(arg)

        depth = 30
        if len(args) > 0:
            depth = int(args[0])

        frame = "$rbp"
        if len(args) > 1:
            frame = args[1]

        for i in range(depth-1, -1, -1):
            ret = gdb.parse_and_eval(f"*(uint64_t*)({frame} + 8)")
            frame = gdb.parse_and_eval(f"*(uint64_t*)({frame})")

            name = ""
            block = gdb.block_for_pc(int(ret))
            if block:
                name = block.function or ""

            print("{:016x} {}".format(int(ret), name))

            if frame == 0 or ret == 0:
                return


PrintStackCommand()
PrintBacktraceCommand()

gdb.execute("target remote :1234")
gdb.execute("display/i $rip")
