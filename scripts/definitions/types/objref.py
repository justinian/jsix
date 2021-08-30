from .type import Type

class ObjectRef(Type):
    all_refs = {}

    def __init__(self, name, filename=None):
        super().__init__(name)
        self.__c_type = "j6_handle_t"
        self.__object = None
        ObjectRef.all_refs[self] = filename

    def __repr__(self):
        return f'ObjectRef({self.name})'

    object = property(lambda self: self.__object)

    def c_names(self, options):
        one = self.__c_type
        out = bool({"out", "inout"}.intersection(options))

        if "list" in options:
            two = "size_t"
            if out:
                one = f"const {one} *"
                two += " *"
            else:
                one = f"{one} *"
            return ((one, ""), (two, "_count"))

        else:
            if out:
                one += " *"
            return ((one, ""),)

    def cxx_names(self, options):
        return self.c_names(options)

    @classmethod
    def connect(cls, objects):
        for ref, filename in cls.all_refs.items():
            ref.__object = objects.get(ref.name)
            if ref.__object is None:
                from ..errors import UnknownTypeError
                raise UnknownTypeError(ref.name, filename)
