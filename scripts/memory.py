
def unit(size):
    units = ['B', 'KiB', 'MiB', 'GiB', 'TiB', 'PiB']

    o = 0
    while size >= 1024 and o < len(units):
        o += 1
        size = size >> 10

    return f"{size} {units[o]}"


class Layout:
    from collections import namedtuple
    Region = namedtuple("Region", ("name", "desc", "start", "size", "shared"))

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
                regions.append(Layout.Region(r["name"], r["desc"], addr, size,
                    r.get("shared", False)))

        self.regions = tuple(regions)
