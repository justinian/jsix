from . import _indent
from . import Options

class Object:
    def __init__(self, name, uid, typename=None, opts=Options(), desc="", children=tuple()):
        self.name = name
        self.uid = uid
        self.options = opts
        self.desc = desc
        self.super = typename
        self.methods = children

        from . import ObjectRef
        self.__ref = ObjectRef(name)

    def __str__(self):
        parts = [f"object {self.name}: {self.uid}"]
        if self.desc:
            parts.append(_indent(self.desc))
        if self.options:
            parts.append(f"    Options: {self.options}")
        parts.extend(map(_indent, self.methods))
        return "\n".join(parts)

    reftype = property(lambda self: self.__ref)
