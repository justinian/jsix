from .type import Type

class Primitive(Type):
    def __init__(self, name, c_type):
        super().__init__(name)
        self.c_type = c_type

    def __repr__(self):
        return f'Primitive({self.name})'

    def c_names(self, options=dict()):
        one = self.c_type
        if "out" in options or "inout" in options:
            one += " *"

        return ((one, ""),)

    def cxx_names(self, options):
        return self.c_names(options)

class PrimitiveRef(Primitive):
    def __init__(self, name, c_type, counted=False):
        super().__init__(name, c_type)
        self.__counted = counted

    reference = property(lambda self: True)

    def c_names(self, options=dict()):
        one = f"{self.c_type} *"
        two = "size_t"

        if "out" in options or "inout" in options:
            two += " *"
        else:
            one = "const " + one

        if self.__counted:
            return ((one, ""), (two, "_len"))
        else:
            return ((one, ""),)

    def cxx_names(self, options):
        return self.c_names(options)

_primitives = {
    "string": PrimitiveRef("string", "char"),
    "buffer": PrimitiveRef("buffer", "void", counted=True),

    "int": Primitive("int", "int"),
    "uint": Primitive("uint", "unsigned"),
    "size": Primitive("size", "size_t"),
    "address": Primitive("address", "uintptr_t"),

    "int8": Primitive("int8", "int8_t"),
    "uint8": Primitive("uint8", "uint8_t"),

    "int16": Primitive("int16", "int16_t"),
    "uint16": Primitive("uint16", "uint16_t"),

    "int32": Primitive("int32", "int32_t"),
    "uint32": Primitive("uint32", "uint32_t"),

    "int64": Primitive("int64", "int64_t"),
    "uint64": Primitive("uint64", "uint64_t"),
}

def get_primitive(name):
    p = _primitives.get(name)
    if not p:
        from ..errors import InvalidType
        raise InvalidType(name)
    return p
