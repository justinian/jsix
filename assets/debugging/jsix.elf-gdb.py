import gdb
import gdb.printing

import sys
sys.path.append('./scripts')

import re

from collections import namedtuple
Capability = namedtuple("Capability", ["id", "parent", "refcount", "caps", "type", "koid"])
LogEntry = namedtuple("LogHeader", ["id", "bytes", "severity", "area", "message"])

class PrintStackCommand(gdb.Command):
    def __init__(self):
        super().__init__("j6stack", gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        args = gdb.string_to_argv(arg)

        base = "$rsp"
        if len(args) > 0:
            base = args[0]
        base = int(gdb.parse_and_eval(base))

        depth = 22
        if len(args) > 1:
            depth = int(args[1])

        for i in range(depth-1, -1, -1):
            try:
                offset = i * 8
                value = gdb.parse_and_eval(f"*(uint64_t*)({base:#x} + {offset:#x})")
                print("{:016x} (+{:04x}): {:016x}".format(base + offset, offset, int(value)))
            except Exception as e:
                print(e)
                continue


def stack_walk(frame, depth):
    for i in range(depth-1, -1, -1):
        ret = gdb.parse_and_eval(f"*(uint64_t*)({frame:#x} + 0x8)")

        name = ""
        try:
            block = gdb.block_for_pc(int(ret))
            if block:
                name = block.function or ""
        except RuntimeError:
            pass

        try:
            print("{:016x}: {:016x} {}".format(int(frame), int(ret), name))
            frame = int(gdb.parse_and_eval(f"*(uint64_t*)({frame:#x})"))
        except gdb.MemoryError:
            return

        if frame == 0 or ret == 0:
            return


class PrintBacktraceCommand(gdb.Command):
    def __init__(self):
        super().__init__("j6bt", gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        args = gdb.string_to_argv(arg)

        frame = "$rbp"
        if len(args) > 0:
            frame = args[0]

        frame = int(gdb.parse_and_eval(f"{frame}"))

        depth = 30
        if len(args) > 1:
            depth = int(gdb.parse_and_eval(args[1]))

        stack_walk(frame, depth)


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
            entry = int(gdb.parse_and_eval(f'((uint64_t*)0x{table:x})[0x{indices[i]:x}]'))
            flagset = flagsets[i]
            flag_names = " | ".join([f[1] for f in flagset if (entry & f[0]) == f[0]])

            print(f"{names[i]:>4}: {table:016x}")
            print(f"      index: {indices[i]:3} {entry:016x}")
            print(f"      flags: {flag_names}")

            if (entry & 1) == 0 or (i < 3 and (entry & 0x80)):
                break

            table = (entry & 0x7ffffffffffffe00) | 0xffffc00000000000


class GetThreadsCommand(gdb.Command):
    FLAGS = {
        "ready":    0x01,
        "loading":  0x02,
        "exited":   0x04,
        "constant": 0x80,
    }

    def __init__(self):
        super().__init__("j6threads", gdb.COMMAND_DATA)

    def get_flags(self, bitset):
        flags = []
        for k, v in GetThreadsCommand.FLAGS.items():
            if bitset & v:
                flags.append(k)
        return " ".join(flags)

    def print_thread(self, addr):
        if addr == 0:
            print("  <no thread>\n")
            return

        tcb = f"((TCB*){addr:#x})"
        thread = f"({tcb}->thread)"
        stack = int(gdb.parse_and_eval(f"{tcb}->kernel_stack"))
        rsp = int(gdb.parse_and_eval(f"{tcb}->rsp"))
        pri = int(gdb.parse_and_eval(f"{tcb}->priority"))
        flags = int(gdb.parse_and_eval(f"{thread}->m_state"))
        koid = int(gdb.parse_and_eval(f"{thread}->m_obj_id"))
        proc = int(gdb.parse_and_eval(f"{thread}->m_parent.m_obj_id"))

        creator = int(gdb.parse_and_eval(f"{thread}->m_creator"))
        if creator != 0:
            creator_koid = int(gdb.parse_and_eval(f"{thread}->m_creator->m_obj_id"))
            creator = f"{creator_koid:x}"
        else:
            creator = "<no thread>"

        print(f"  Thread {proc:x}:{koid:x}")
        print(f"         creator: {creator}")
        print(f"        priority: {pri}")
        print(f"           flags: {self.get_flags(flags)}")
        print(f"          kstack: {stack:#x}")
        print(f"             rsp: {rsp:#x}")

        if stack == 0: return 0
        return int(gdb.parse_and_eval(f"{tcb}->rsp"))


    def print_thread_list(self, addr, name):
        if addr == 0:
            return

        print(f"=== {name} ===")

        while addr != 0:
            rsp = self.print_thread(addr)
            if rsp != 0:
                print("------------------------------------")
                stack_walk(rsp + 5*8, 5)

            addr = int(gdb.parse_and_eval(f"((tcb_node*){addr:#x})->m_next"))
            print()

    def print_cpudata(self, index):
        cpu = f"(g_cpu_data[{index}])"
        tss = f"{cpu}->tss"
        tss_rsp0 = int(gdb.parse_and_eval(f"{tss}->m_rsp[0]"))
        tss_ist = [int(gdb.parse_and_eval(f"{tss}->m_ist[{i}]")) for i in range(8)]
        print(f"        tss rsp0: {tss_rsp0:#x}")
        for i in range(1,8):
            if tss_ist[i] == 0: continue
            print(f"        tss ist{i}: {tss_ist[i]:#x}")

    def invoke(self, arg, from_tty):
        args = gdb.string_to_argv(arg)
        if len(args) > 1:
            raise RuntimeError("Usage: j6threads [cpu]")

        ncpus = int(gdb.parse_and_eval("g_num_cpus"))
        cpus = list(range(ncpus))
        if len(args) == 1:
            cpus = [int(args[0])]

        for cpu in cpus:
            runlist = f"scheduler::s_instance->m_run_queues.m_elements[{cpu:#x}]"

            print(f"=== CPU {cpu:2}: CURRENT ===")
            current = int(gdb.parse_and_eval(f"{runlist}.current"))
            self.print_thread(current)
            self.print_cpudata(cpu)

            previous = int(gdb.parse_and_eval(f"{runlist}.prev"))
            print(f"            prev: {previous:x}")
            print()

            for pri in range(8):
                ready = int(gdb.parse_and_eval(f"{runlist}.ready[{pri:#x}].m_head"))
                self.print_thread_list(ready, f"CPU {cpu:2}: PRIORITY {pri}")

            blocked = int(gdb.parse_and_eval(f"{runlist}.blocked.m_head"))
            self.print_thread_list(blocked, f"CPU {cpu:2}: BLOCKED")
            print()


class PrintProfilesCommand(gdb.Command):
    def __init__(self):
        super().__init__("j6prof", gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        args = gdb.string_to_argv(arg)
        if len(args) != 1:
            raise RuntimeError("Usage: j6prof <profiler class>")

        profclass = args[0]
        root_type = f"profile_class<{profclass}>"

        try:
            baseclass = gdb.lookup_type(root_type)
        except Exception as e:
            print(e)
            return

        results = {}
        max_len = 0
        count = gdb.parse_and_eval(f"{profclass}::count")
        for i in range(count):
            name = gdb.parse_and_eval(f"{root_type}::function_names[{i:#x}]")
            if name == 0: continue

            call_counts = gdb.parse_and_eval(f"{root_type}::call_counts[{i:#x}]")
            call_durations = gdb.parse_and_eval(f"{root_type}::call_durations[{i:#x}]")
            results[name.string()] = float(call_durations) / float(call_counts)
            max_len = max(max_len, len(name.string()))

        for name, avg in results.items():
            print(f"{name:>{max_len}}: {avg:15.3f}")


class DumpLogCommand(gdb.Command):
    level_names = ["", "fatal", "error", "warn", "info", "verbose", "spam"]

    def __init__(self):
        super().__init__("j6log", gdb.COMMAND_DATA)

        from memory import Layout
        layout = Layout("definitions/memory_layout.yaml")
        for region in layout.regions:
            if region.name == "logs":
                self.base_addr = region.start
                break

        self.areas = []
        area_re = re.compile(r"LOG\(\s*(\w+).*")
        with open("src/libraries/j6/include/j6/tables/log_areas.inc", 'r') as areas_inc:
            for line in areas_inc:
                m = area_re.match(line)
                if m:
                    self.areas.append(m.group(1))

    def get_entry(self, addr):
        addr = int(addr)
        size = int(gdb.parse_and_eval(f"((j6_log_entry*){addr:#x})->bytes"))
        mlen = size - 8

        return LogEntry(
                int(gdb.parse_and_eval(f"((j6_log_entry*){addr:#x})->id")),
                size,
                int(gdb.parse_and_eval(f"((j6_log_entry*){addr:#x})->severity")),
                int(gdb.parse_and_eval(f"((j6_log_entry*){addr:#x})->area")),
                gdb.parse_and_eval(f"((j6_log_entry*){addr:#x})->message").string(length=mlen))

    def invoke(self, arg, from_tty):
        start = gdb.parse_and_eval("g_logger.m_start & (g_logger.m_buffer.count - 1)")
        end = gdb.parse_and_eval("g_logger.m_end & (g_logger.m_buffer.count - 1)")
        if end < start:
            end += gdb.parse_and_eval("g_logger.m_buffer.count")

        print(f"Logs are {start} -> {end}")

        addr = self.base_addr + start
        end += self.base_addr
        while addr < end:
            entry = self.get_entry(addr)
            if entry.bytes < 8:
                print(f"Bad log header size: {entry.bytes}")
                break
            addr += entry.bytes
            area = "??"
            if entry.area < len(self.areas):
                area = self.areas[entry.area]
            level = self.level_names[entry.severity]
            print(f"{area:>7}:{level:7}  {entry.message}")


class ShowCurrentProcessCommand(gdb.Command):
    def __init__(self):
        super().__init__("j6current", gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        def get_obj_and_id(name):
            obj = int(gdb.parse_and_eval(f"((cpu_data*)$gs_base)->{name}"))
            oid = -1
            if obj != 0:
                oid = int(gdb.parse_and_eval(f"((obj::kobject*){obj:#x})->m_obj_id"))
            return obj, oid

        process, pid = get_obj_and_id("process")
        thread, tid = get_obj_and_id("thread")
        print(f"{pid:02x}/{tid:02x}  [ {process:x} / {thread:x} ]")


class CapTablePrinter:
    def __init__(self, val):
        node_map = val["m_caps"]
        self.nodes = node_map["m_nodes"]
        self.count = int(node_map["m_count"])
        self.capacity = int(node_map["m_capacity"])

    class _iterator:
        def __init__(self, nodes, capacity):
            self.nodes = []

            for i in range(capacity):
                node = nodes[i]

                node_id = int(node["id"])
                if node_id == 0:
                    continue

                self.nodes.append(Capability(
                    id = node_id,
                    parent = int(node["parent"]),
                    refcount = int(node["holders"]),
                    caps = int(node["caps"]),
                    type = str(node["type"])[14:],
                    koid = node['object']['m_obj_id']))

            self.nodes.sort(key=lambda n: n.id, reverse=True)

        def __iter__(self):
            return self

        def __next__(self):
            if not self.nodes:
                raise StopIteration

            node = self.nodes.pop()

            desc = f'p:{node.parent:016x}  refs:{node.refcount:2}  caps:{node.caps:04x}  {node.type:14} {node.koid}'
            return (f"{node.id:016x}", desc)

    def to_string(self):
        return f"Cap table with {self.count}/{self.capacity} nodes.\n"

    def children(self):
        return self._iterator(self.nodes, self.capacity)


class VectorPrinter:
    def __init__(self, vector):
        self.name = vector.type.tag
        self.count = vector["m_size"]
        self.array = vector["m_elements"]

    class _iterator:
        def __init__(self, array, count, deref):
            self.array = array
            self.count = count
            self.deref = deref
            self.index = 0

        def __iter__(self):
            return self

        def __next__(self):
            if self.index >= self.count:
                raise StopIteration

            item = self.array[self.index]
            if self.deref:
                item = item.dereference()

            result = (f"{self.index:3}", item)
            self.index += 1
            return result

    def to_string(self):
        return f"{self.name} [{self.count}]"

    def children(self):
        deref = self.count > 0 and self.array[0].type.code == gdb.TYPE_CODE_PTR
        return self._iterator(self.array, int(self.count), deref)


class HandleSetPrinter:
    def __init__(self, nodeset):
        self.node_map = nodeset['m_map']
        self.count = self.node_map['m_count']
        self.capacity = self.node_map['m_capacity']

    class _iterator:
        def __init__(self, nodes, capacity):
            self.items = []
            self.index = 0
            for i in range(capacity):
                item = nodes[i]
                if int(item) != 0:
                    self.items.append(int(item))
            self.items.sort()

        def __iter__(self):
            return self

        def __next__(self):
            if self.index >= len(self.items):
                raise StopIteration
            result = (f"{self.index}", f"{self.items[self.index]:016x}")
            self.index += 1
            return result

    def to_string(self):
        return f"node_set[{self.count} / {self.capacity}]:"

    def children(self):
        return self._iterator(self.node_map['m_nodes'], self.capacity)


class LinkedListPrinter:
    def __init__(self, llist):
        self.name = llist.type.tag
        self.head = llist['m_head']
        self.tail = llist['m_tail']

        self.items = []
        current = self.head
        while current:
            item = current.dereference()
            self.items.append((str(len(self.items)), item))
            current = item['m_next']

    class _iterator:
        def __iter__(self):
            return self

        def __next__(self):
            raise StopIteration

    def to_string(self):
        return f"{self.name}[{len(self.items)}]"

    def children(self):
        return self.items


class IsRunning(gdb.Function):
    def __init__(self):
        super(IsRunning, self).__init__("is_running")

    def invoke(self):
        inferior = gdb.selected_inferior()
        return \
            inferior is not None and \
            inferior.is_valid() and \
            len(inferior.threads()) > 0


def build_pretty_printers():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("jsix")
    pp.add_printer("cap table", '^cap_table$', CapTablePrinter)
    pp.add_printer("handle set", '^util::node_set<unsigned long, 0, heap_allocated>$', HandleSetPrinter)
    pp.add_printer("vector", '^util::vector<.*>$', VectorPrinter)
    pp.add_printer("linked list", '^util::linked_list<.*>$', LinkedListPrinter)
    return pp

gdb.printing.register_pretty_printer(
    gdb.current_objfile(),
    build_pretty_printers())

PrintStackCommand()
PrintBacktraceCommand()
TableWalkCommand()
GetThreadsCommand()
PrintProfilesCommand()
DumpLogCommand()
ShowCurrentProcessCommand()
IsRunning()

gdb.execute("display/i $rip")
gdb.execute("define hook-quit\nif $is_running()\n  kill\nend\nend")
if not gdb.selected_inferior().was_attached:
    gdb.execute("add-symbol-file build/panic.serial.elf")
    gdb.execute("target remote :1234")
