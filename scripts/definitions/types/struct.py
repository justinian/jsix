from .type import Type

class Struct(Type):
    def __repr__(self):
        return f'Struct({self.name})'

    def c_names(self, options):
        one = f"struct j6_{self.name} *"
        two = "size_t"

        out = bool({"out", "inout"}.intersection(options))
        if out:
            two += " *"
        return ((one, ""), (two, "_size"))

    def cxx_names(self, options):
        return self.c_names(options)
