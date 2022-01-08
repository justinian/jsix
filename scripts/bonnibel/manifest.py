from . import BonnibelError

class Manifest:
    from collections import namedtuple
    Entry = namedtuple("Entry", ("module", "target", "output", "location", "description", "flags"))

    flags = {
        "graphical": 0x01,
        "panic":     0x02,
        "symbols":   0x04,
    }

    def __init__(self, path, modules):
        from . import load_config

        config = load_config(path)

        self.kernel = self.__build_entry(modules,
                config.get("kernel", dict()),
                name="kernel", target="kernel")

        self.init = self.__build_entry(modules,
                config.get("init", None))

        self.programs = [self.__build_entry(modules, i)
                for i in config.get("programs", tuple())]

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

        return Manifest.Entry(name, target, mod.output, mod.location, mod.description, flags)

    def add_data(self, output, location, desc, flags=tuple()):
        e = Manifest.Entry(None, None, output, location, desc, flags)
        self.data.append(e)
        return e

    def write_boot_config(self, path):
        from os.path import join
        import struct

        with open(path, 'wb') as outfile:
            magic = "jsixboot".encode("utf-8") # magic string
            version = 0
            reserved = 0

            outfile.write(struct.pack("<8sBBHHH",
                magic, version, reserved,
                len(self.programs), len(self.data),
                reserved))

            def write_str(s):
                outfile.write(struct.pack("<H", (len(s)+1)*2))
                outfile.write(s.encode("utf-16le"))
                outfile.write(b"\0\0")

            def write_ent(ent):
                flags = 0
                for f in ent.flags:
                    flags |= Manifest.flags[f]

                outfile.write(struct.pack("<H", flags))
                write_str(join(ent.location, ent.output).replace('/','\\'))
                write_str(ent.description)

            write_ent(self.kernel)
            write_ent(self.init)

            for p in self.programs:
                write_ent(p)

            for d in self.data:
                write_ent(d)
