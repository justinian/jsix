_SECTOR_SIZE = 512
_MEDIA_TYPE = 0xf8

_FAT16_MIN_SECTORS = 4115 # Higher than spec, first size where mtools uses FAT16
_FAT16_MAX_SECTORS = 65525

_DIRENT_SIZE = 32
_DIRENT_PER_SECTOR = _SECTOR_SIZE // _DIRENT_SIZE

def b_to_s(b):
    return (b+(_SECTOR_SIZE-1))//_SECTOR_SIZE

def fat16_size_for_clusters(c):
    return b_to_s(c*2)

def get_sector(image):
    return b_to_s(image.tell())


def write_bpb(image, clusters, root_entries, fat_sectors):
    from struct import pack

    clusters_16 = 0
    clusters_32 = clusters
    if clusters < 1<<16:
        clusters_16 = clusters
        clusters_32 = 0

    # Write the end signature
    image.seek(510)
    image.write(b"\xaa\x55")

    image.seek(0)
    image.write(b"\xeb\x3c\x90")                # jump instruction
    image.write(b"j6mkfat0")                    # OEM identifier
    image.write(pack("<H", _SECTOR_SIZE))       # Bytes per sector
    image.write(pack("B", 1))                   # Sectors per cluster
    image.write(pack("<H", 1))                  # Number of reserved sectors
    image.write(pack("B", 1))                   # Number of FATs
    image.write(pack("<H", root_entries))       # Number of root directory entries
    image.write(pack("<H", clusters_16))        # Number of clusters total (if small)
    image.write(pack("B", _MEDIA_TYPE))         # Media descriptor byte
    image.write(pack("<H", fat_sectors))        # Number of sectors per FAT
    image.write(pack("<H", 26))                 # Number of sectors per track
    image.write(pack("<H", 2))                  # Number of heads/sides
    image.write(pack("<L", 0))                  # Number of hidden sectors
    image.write(pack("<L", clusters_32))        # Number of clusters total (if large)


def make_fat16(root):
    from io import BytesIO
    from os import scandir
    from random import randbytes
    from struct import pack

    data_sectors = max(root.total_sectors, _FAT16_MIN_SECTORS)
    table_sectors = fat16_size_for_clusters(data_sectors)
    fs_sectors = data_sectors + table_sectors + 1  # include the BPB
    size = fs_sectors * _SECTOR_SIZE

    image = BytesIO(b'\0'*size)

    root_entry_sectors = ((len(root.children) + _DIRENT_PER_SECTOR - 1) // _DIRENT_PER_SECTOR)
    root_entries = root_entry_sectors * _DIRENT_PER_SECTOR

    # Write generic BPB
    write_bpb(image, fs_sectors, root_entries, table_sectors)

    # Write FAT16 extBPB
    image.write(pack("B", 0x80))                # Drive number
    image.write(pack("B", 0))                   # WinNT flags
    image.write(pack("B", 0x29))                # Signature
    image.write(randbytes(4))                   # Serial number
    image.write(b"jsix OS ESP")                 # Volume label (11 bytes, pad with spaces)
    image.write(b"FAT16   ")                    # FS identifier (8 bytes, pad with spaces)

    file_start_sector = 1 + table_sectors + root.num_sectors

    image.seek(_SECTOR_SIZE * file_start_sector)
    root.write(image, file_start_sector-2, 1 + table_sectors)

    # Fill in the FAT table
    image.seek(_SECTOR_SIZE)
    image.write(pack("<BBH", _MEDIA_TYPE, 0xff, 0xffff))

    def fill_fat(cluster, num):
        if num < 1: return
        image.seek(_SECTOR_SIZE + cluster*2)
        vals = list(range(cluster+1, cluster+num+1))
        vals[-1] = 0xffff # the last cluster gets EOC
        for i in vals: image.write(pack("<H", i))

    openset = set(root.children)
    while openset:
        f = openset.pop()
        fill_fat(f.cluster, f.num_sectors)
        if isinstance(f, FATDir):
            openset.update(f.children)

    return image.getvalue()


class EightDotThree:
    import re
    _special = re.compile(r"[^0-9A-Z$%'-`|(){}^#&.]")

    def __init__(self, name):
        upper = name.upper()
        s = self._special.sub('_', upper)

        parts = s.rsplit(".", 1)
        self.__base = parts[0].replace(".", "")[:8]
        if len(parts) > 1:
            self.__ext = parts[1][:3]
        else:
            self.__ext = ""

        self.__modified = str(self) != upper

    modified = property(lambda self: self.__modified)

    def __str__(self):
        if self.__ext:
            return f"{self.__base}.{self.__ext}"
        return self.__base

    def dir_name(self):
        return f"{self.__base:8}{self.__ext:3}".encode('ascii')

    def make_unique(self):
        self.__base = self.__base[:6] + "~1"


class FATFileBase:
    _DIR_FLAGS = 0

    def __init__(self, path, parent=None, name=None):
        self.__path = path
        self.__name = name or path.name
        self.__sfn = EightDotThree(self.name)
        self.__sector = 0
        self.__cluster = 0
        self.__parent = parent
        self.__build_lfn()

    path = property(lambda self: self.__path)
    name = property(lambda self: self.__name)
    lfn = property(lambda self: self.__lfn)
    sfn = property(lambda self: self.__sfn)
    is_dir = property(lambda self: False)
    sector = property(lambda self: self.__sector)
    cluster = property(lambda self: self.__cluster)
    parent = property(lambda self: self.__parent)

    def __str__(self):
        return f"{self.name}: {self.sfn} {self.lfn}"

    def __build_lfn(self):
        if not self.sfn.modified:
            self.__lfn = []
            return

        self.sfn.make_unique()
        self.__lfn = [self.name[n:n+13] for n in range(0, len(self.name), 13)]

    def start_sector(self, image, cluster_off):
        loc = image.tell()
        self.__sector = b_to_s(loc)
        self.__cluster = self.__sector - cluster_off
        image.seek(self.__sector * _SECTOR_SIZE)

    def write_dirents(self, image):
        from struct import pack

        checksum = 0
        sfn_name = self.sfn.dir_name()
        for c in sfn_name:
            checksum = ((checksum>>1) + ((checksum<<7)&0xff) + c) & 0xff

        max_chars = 13 # Chars per LFN dirent

        # Order the LFN entries in reverse on disk
        for i in reversed(range(len(self.lfn))):
            lfn = self.lfn[i]

            ordinal = i+1
            if i == len(self.lfn) - 1:
                ordinal |= 0x40 # End flag


            from io import BytesIO
            data = BytesIO(b'\xff\xff'*max_chars)
            n = data.write(lfn.encode('utf-16-le'))
            if n < max_chars*2:
                data.write(b'\0\0')
            data = data.getvalue()

            part1 = data[:10]
            part2 = data[10:22]
            part3 = data[22:]

            image.write(pack(
                "<B10sBxB12s2x4s",
                ordinal,
                part1,
                0x0f, # LFN Attribute
                checksum,
                part2,
                part3,
                ))

        image.write(sfn_name)
        image.write(pack(
            "<BBBHHHHHHHL",
            self._DIR_FLAGS,     # Dirent flags
            0,                   # NT Flags
            0,                   # Create time sub-seconds
            0,                   # Create time
            0,                   # Create date
            0,                   # Access date
            0,                   # Cluster high bits
            0,                   # Write time
            0,                   # Write date
            self.cluster,        # Cluster low bits
            self.size,           # File size
            ))


class FATFile(FATFileBase):
    def __init__(self, path, parent=None, name=None):
        from os.path import getsize
        super().__init__(path, parent, name)
        assert path.is_file()
        self.__size = getsize(path)

    size = property(lambda self: self.__size)

    @property
    def num_sectors(self):
        return b_to_s(self.__size)
    total_sectors = num_sectors

    def write(self, image, cluster_off):
        self.start_sector(image, cluster_off)
        with open(self.path, 'rb') as f:
            image.write(f.read())


class FATDir(FATFileBase):
    _DIR_FLAGS = 0x10

    def __init__(self, path, parent=None, name=None):
        super().__init__(path, parent, name)
        assert path.is_dir()

        children = []
        for child in path.iterdir():
            if child.is_dir():
                children.append(FATDir(child, self))
            else:
                children.append(FATFile(child, self))
        self.__children = tuple(children)

    children = property(lambda self: self.__children)
    is_dir = property(lambda self: True)
    size = property(lambda self: 0)

    @property
    def num_sectors(self):
        dirents = sum([len(c.lfn) for c in self.children]) + len(self.children) + 2
        return b_to_s(dirents * _DIRENT_SIZE)

    @property
    def total_sectors(self):
        return self.num_sectors + sum([c.total_sectors for c in self.children])

    def __str__(self):
        from textwrap import indent
        dirents = len(self.children) + sum([len(c.lfn) for c in self.children])
        s = super().__str__() + f" ({dirents}: {b_to_s(dirents*32)})"
        return "\n".join([s,] + [indent(str(c), "    ") for c in self.children])

    def write(self, image, cluster_off, root_sector=None):
        from struct import pack

        if self.parent:
            # For non-root dirs, save the cluster and advance
            # the writer so that subdirs can write "..", then
            # come back to it and write our own data after.
            assert root_sector is None
            self.start_sector(image, cluster_off)

            next_sector = self.sector + self.num_sectors
            image.seek(next_sector * _SECTOR_SIZE)

        for c in self.children:
            c.write(image, cluster_off)

        last_write = image.tell()

        if root_sector is not None:
            image.seek(root_sector * _SECTOR_SIZE)
        else:
            image.seek(self.sector * _SECTOR_SIZE)

        if not self.parent:
            image.write(pack("<11sB20x", self.sfn.dir_name(), 0x08))
        else:
            dotname = b".." + b" "*10
            image.write(pack("<11sB14xH4x",
                dotname[1:12], 0x10, self.cluster))
            image.write(pack("<11sB14xH4x",
                dotname[0:11], 0x10, self.parent.cluster))

        for c in self.children:
            c.write_dirents(image)

        remaining = (_SECTOR_SIZE - (image.tell() % _SECTOR_SIZE)) // _DIRENT_SIZE
        for i in range(remaining):
            # Pad out empty dirents
            image.write(b'\0'*_DIRENT_SIZE)

        image.seek(last_write)


if __name__ == "__main__":
    import sys
    from pathlib import Path

    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <image file> <source dir>")
        sys.exit(1)

    image = Path(sys.argv[1])
    root = Path(sys.argv[2])

    assert root.is_dir()

    source = FATDir(root, name="jsix_esp")
    with open(image, 'wb') as f:
        f.write(make_fat16(source))
