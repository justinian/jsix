class Layout:
    from collections import namedtuple
    Region = namedtuple("Region", ("name", "start", "size", "shared"))

    sizes = {'G': 1024 ** 3, 'T': 1024 ** 4}

    @staticmethod
    def get_size(desc):
        size, mag = int(desc[:-1]), desc[-1]

        try:
            mult = Layout.sizes[mag]
        except KeyError:
            raise RuntimeError(f"No magnitude named '{mag}'.")

        return size * mult

    def __init__(self, path):
        from yaml import safe_load

        regions = []
        addr = 1 << 64

        with open(path, 'r') as infile:
            data = safe_load(infile.read())
            for r in data:
                size = Layout.get_size(r["size"])
                addr -= size
                regions.append(Layout.Region(r["name"], addr, size,
                    r.get("shared", False)))

        self.regions = tuple(regions)
