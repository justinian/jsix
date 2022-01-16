from . import _indent
from . import Options

def _hasopt(opt):
    def test(self):
        return opt in self.options
    return test

class Function:
    typename = "function"

    def __init__(self, name, opts=Options(), desc="", children=tuple()):
        self.name = name
        self.options = opts
        self.desc = desc
        self.params = [c for c in children if isinstance(c, Param)]
        self.id = -1

    def __str__(self):
        parts = ["{} {}".format(self.typename, self.name)]
        if self.desc:
            parts.append(_indent(self.desc))
        if self.options:
            parts.append(f"    Options: {self.options}")
        parts.extend(map(_indent, self.params))
        return "\n".join(parts)

    static = property(lambda x: True)
    constructor = property(lambda x: False)


class Method(Function):
    typename = "method"

    static = property(_hasopt("static"))
    constructor = property(_hasopt("constructor"))


class Param:
    def __init__(self, name, typename, opts=Options(), desc=""):
        self.name = name
        self.type = typename
        self.options = opts
        self.desc = desc

    def __str__(self):
        return "param {} {} {}  {}".format(
                self.name, repr(self.type), self.options, self.desc or "")

    @property
    def outparam(self):
        return "out" in self.options or "inout" in self.options

    @property
    def refparam(self):
        return self.type.reference or self.outparam

    @property
    def optional(self):
        return "optional" in self.options

