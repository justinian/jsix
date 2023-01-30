from . import BonnibelError

class Manifest:
    from collections import namedtuple
    Entry = namedtuple("Entry", ("module", "target", "output", "flags"))

    flags = {
        "graphical": 0x01,
        "symbols":   0x80,
    }

    boot_flags = {
        "debug": 0x01,
        "test":  0x02,
    }

    def __init__(self, path, modules):
        from . import load_config

        config = load_config(path)

        self.location = config.get("location", "jsix")

        self.kernel = self.__build_entry(modules,
                config.get("kernel", dict()),
                name="kernel", target="kernel")

        self.init = self.__build_entry(modules,
                config.get("init", None))

        self.panics = [self.__build_entry(modules, i, target="kernel")
                for i in config.get("panic", tuple())]

        self.services = [self.__build_entry(modules, i)
                for i in config.get("services", tuple())]

        self.drivers = [self.__build_entry(modules, i)
                for i in config.get("drivers", tuple())]

        self.flags = config.get("flags", tuple())

        initrd = config.get("initrd", dict())
        self.initrd = {
            "name": initrd.get("name", "initrd.dat"),
            "format": initrd.get("format", "zstd"),
        }

        self.data = []
        for d in config.get("data", tuple()):
            self.add_data(**d)

    def __build_entry(self, modules, config, target="user", name=None):
        flags = tuple()

        if isinstance(config, str):
            name = config
        elif isinstance(config, dict):
            name = config.get("name", name)
            target = config.get("target", target)
            flags = config.get("flags", tuple())
            if isinstance(flags, str):
                flags = flags.split()

        mod = modules.get(name)
        if not mod:
            raise BonnibelError(f"Manifest specifies unknown module '{name}'")

        for f in flags:
            if not f in Manifest.flags:
                raise BonnibelError(f"Manifest specifies unknown flag '{f}'")

        return Manifest.Entry(name, target, mod.output, flags)

    def add_data(self, output, desc, flags=tuple()):
        e = Manifest.Entry(None, None, output, flags)
        self.data.append(e)
        return e

    def write_boot_config(self, path):
        from os.path import join
        import struct

        with open(path, 'wb') as outfile:
            magic = "jsixboot".encode("utf-8") # magic string
            version = 1

            bootflags = sum([Manifest.boot_flags.get(s, 0) for s in self.flags])

            outfile.write(struct.pack("<8s BBH",
                magic,
                version, len(self.panics), bootflags))

            def write_str(s):
                data = s.encode("utf-16le")
                outfile.write(struct.pack("<H", len(data)+2))
                outfile.write(data)
                outfile.write(b"\0\0")

            def write_path(name):
                write_str(join(self.location, name).replace('/','\\'))

            def write_ent(ent):
                flags = 0
                for f in ent.flags:
                    flags |= Manifest.flags[f]

                outfile.write(struct.pack("<H", flags))
                write_path(ent.output)

            write_ent(self.kernel)
            write_ent(self.init)
            write_path(self.initrd["name"])

            for p in self.panics:
                write_ent(p)

    def write_init_config(self, path, modules):
        from os.path import join
        import struct

        with open(path, 'wb') as outfile:
            magic = "jsixinit".encode("utf-8") # magic string
            version = 1
            reserved = 0

            string_data = bytearray()

            outfile.write(struct.pack("<8s BBH HH",
                magic,
                version, reserved, len(self.services),
                len(self.data), len(self.drivers)))

            offset_ptr = outfile.tell()
            outfile.write(struct.pack("<H", 0)) # filled in later

            def write_str(s):
                pos = len(string_data)
                string_data.extend(s.encode("utf-8") + b"\0")
                outfile.write(struct.pack("<H", pos))

            def write_driver(ent):
                write_str(ent.output)

                drivers = modules[ent.module].drivers
                outfile.write(struct.pack("<H", len(drivers)))
                for driver in drivers:
                    write_str(driver)

            for p in self.services:
                write_str(p.output)

            for p in self.data:
                write_str(p.output)

            for p in self.drivers:
                write_driver(p)

            offset = outfile.tell()
            outfile.write(string_data)
            outfile.seek(offset_ptr)
            outfile.write(struct.pack("<H", offset))
