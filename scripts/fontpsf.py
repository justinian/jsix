_MAGIC = (0x72, 0xb5, 0x4a, 0x86)
_VERSION = 0

class PSF2:
    from collections import namedtuple
    Header = namedtuple("PSF2Header",
        ["version", "offset", "flags", "count", "charsize", "height", "width"])

    def __init__(self, filename, header, data):
        self.__filename = filename
        self.__header = header
        self.__data = data

    data = property(lambda self: self.__data)
    header = property(lambda self: self.__header)
    count = property(lambda self: self.__header.count)
    charsize = property(lambda self: self.__header.charsize)
    dimension = property(lambda self: (self.__header.width, self.__header.height))
    filename = property(lambda self: self.__filename)

    @classmethod
    def load(cls, filename):
        from os.path import basename
        from struct import unpack_from

        data = open(filename, 'rb').read()

        fmt = "BBBBIIIIIII"
        values = unpack_from(fmt, data)

        if values[:len(_MAGIC)] != _MAGIC:
            raise Exception("Bad magic number in header")

        header = PSF2.Header(*values[len(_MAGIC):])
        if header.version != _VERSION:
            raise Exception(f"Bad version {header.version} in header")

        return cls(basename(filename), header, data)

    class Glyph:
        __slots__ = ['index', 'data']
        def __init__(self, i, data):
            self.index = i
            self.data = data
        def __index__(self):
            return self.index
        def empty(self):
            return not bool([b for b in self.data if b != 0])
        def description(self):
            c = chr(self.index)
            if c.isprintable():
                return "Glyph {:02x}: '{}'".format(self.index, c)
            else:
                return "Glyph {:02x}".format(self.index)

    def __getitem__(self, i):
        c = self.__header.charsize
        n = i * c + self.__header.offset
        return PSF2.Glyph(i, self.__data[n:n+c])

    class __iter:
        __slots__ = ['font', 'n']
        def __init__(self, font):
            self.font = font
            self.n = 0
        def __next__(self):
            if self.n < self.font.count:
                glyph = self.font[self.n]
                self.n += 1
                return glyph
            else:
                raise StopIteration

    def __iter__(self):
        return PSF2.__iter(self)
