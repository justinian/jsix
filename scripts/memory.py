class Layout:
    from collections import namedtuple
    Region = namedtuple("Region", ("name", "start", "size", "shared"))

    sizes = {'G': 1024 ** 3, 'T': 1024 ** 4}

    def __init__(self, path):
        import csv

        regions = []
        addr = 1 << 64

        with open(path, newline='') as infile:
            reader = csv.reader(infile)
            for row in reader:
                name, size = row[:2]
                shared = (len(row) > 2 and "shared" in row[2])

                size, mag = int(size[:-1]), size[-1]

                try:
                    mult = Layout.sizes[mag]
                except KeyError:
                    raise RuntimeError(f"No magnitude named '{mag}'.")

                size *= mult
                addr -= size
                regions.append(Layout.Region(name, addr, size, shared))

        self.regions = tuple(regions)
