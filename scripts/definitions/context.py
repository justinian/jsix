class NotFound(Exception): pass

class Context:
    def __init__(self, path, verbose=False):
        if isinstance(path, str):
            self.__paths = [path]
        else:
            self.__paths = path

        self.__closed = set()
        self.__verbose = verbose

        self.__deps = {}

        self.objects = dict()
        self.interfaces = dict()

    verbose = property(lambda self: self.__verbose)

    def find(self, filename):
        from os.path import exists, isabs, join

        if exists(filename) or isabs(filename):
            return filename

        for path in self.__paths:
            full = join(path, filename)
            if exists(full):
                return full

        raise NotFound(filename)

    def parse(self, filename):
        pending = set()
        pending.add(filename)

        from .parser import Lark_StandAlone as Parser
        from .transformer import DefTransformer

        objects = {}
        interfaces = {}

        while pending:
            name = pending.pop()
            self.__closed.add(name)

            path = self.find(name)

            parser = Parser(transformer=DefTransformer(name))
            imps, objs, ints = parser.parse(open(path, "r").read())
            objects.update(objs)
            interfaces.update(ints)

            self.__deps[name] = imps

            pending.update(imps.difference(self.__closed))

        from .types import ObjectRef
        ObjectRef.connect(objects)

        self.objects.update(objects)
        self.interfaces.update(interfaces)

    def deps(self):
        return {self.find(k): tuple(map(self.find, v)) for k, v in self.__deps.items()}
