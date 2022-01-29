from . import _indent
from . import Caps, Options

class Object:
    def __init__(self, name, uid, typename=None, opts=Options(), caps=Caps(), desc="", children=tuple(), cname=None):
        self.name = name
        self.uid = uid
        self.options = opts
        self.desc = desc
        self.methods = children
        self.cname = cname or name
        self.caps = caps

        self.__super = typename

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

    @property
    def super(self):
        if self.__super is not None:
            return self.__super.object
