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
                value = gdb.parse_and_eval(f"*(uint64_t*)({base} + 0x{offset:x})")
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

        frame = gdb.parse_and_eval(f"{frame}")

        for i in range(depth-1, -1, -1):
            ret = gdb.parse_and_eval(f"*(uint64_t*)({frame} + 0x8)")

            name = ""
            try:
                block = gdb.block_for_pc(int(ret))
                if block:
                    name = block.function or ""
            except RuntimeError:
                pass

            print("{:016x}: {:016x} {}".format(int(frame), int(ret), name))

            frame = gdb.parse_and_eval(f"*(uint64_t*)({frame})")
            if frame == 0 or ret == 0:
                return


class TableWalkCommand(gdb.Command):
    def __init__(self):
        super().__init__("j6tw", gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        args = gdb.string_to_argv(arg)
        if len(args) < 2:
            raise Exception("Must be: j6tw <pml4> <addr>")

        pml4 = int(gdb.parse_and_eval(args[0]))
        addr = int(gdb.parse_and_eval(args[1]))

        indices = [
            (addr >> 39) & 0x1ff,
            (addr >> 30) & 0x1ff,
            (addr >> 21) & 0x1ff,
            (addr >> 12) & 0x1ff,
            ]

        names = ["PML4", "PDP", "PD", "PT"]

        table_flags = [
            (0x0001, "present"),
            (0x0002, "write"),
            (0x0004, "user"),
            (0x0008, "pwt"),
            (0x0010, "pcd"),
            (0x0020, "accessed"),
            (0x0040, "dirty"),
            (0x0080, "largepage"),
            (0x0100, "global"),
            (0x1080, "pat"),
            ((1<<63), "xd"),
        ]

        page_flags = [
            (0x0001, "present"),
            (0x0002, "write"),
            (0x0004, "user"),
            (0x0008, "pwt"),
            (0x0010, "pcd"),
            (0x0020, "accessed"),
            (0x0040, "dirty"),
            (0x0080, "pat"),
            (0x0100, "global"),
            ((1<<63), "xd"),
        ]

        flagsets = [table_flags, table_flags, table_flags, page_flags]

        table = pml4
        entry = 0
        for i in range(len(indices)):
            entry = int(gdb.parse_and_eval(f'((uint64_t*){table})[{indices[i]}]'))
            flagset = flagsets[i]
            flag_names = " | ".join([f[1] for f in flagset if (entry & f[0]) == f[0]])

            print(f"{names[i]:>4}: {table:016x}")
            print(f"      index: {indices[i]:3} {entry:016x}")
            print(f"      flags: {flag_names}")

            if (entry & 1) == 0 or (i < 3 and (entry & 0x80)):
                break

            table = (entry & 0x7ffffffffffffe00) | 0xffffc00000000000



PrintStackCommand()
PrintBacktraceCommand()
TableWalkCommand()

gdb.execute("target remote :1234")
gdb.execute("display/i $rip")
