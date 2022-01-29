from . import _indent
from . import Options

class Expose(object):
    def __init__(self, type):
        self.type = type

    def __repr__(self):
        return f'expose {repr(self.type)}'

class Interface:
    def __init__(self, name, uid, opts=Options(), desc="", children=tuple()):
        from .function import Function

        self.name = name
        self.uid = uid
        self.options = opts
        self.desc = desc

        self.functions = [c for c in children if isinstance(c, Function)]
        self.__exposes = [e.type for e in children if isinstance(e, Expose)]

    def __str__(self):
        parts = [f"interface {self.name}: {self.uid}"]
        if self.desc:
            parts.append(_indent(self.desc))
        if self.options:
            parts.append(f"    Options: {self.options}")
        parts.extend(map(_indent, self.exposes))
        parts.extend(map(_indent, self.functions))
        return "\n".join(parts)

    @property
    def methods(self):
        mm = [(i, None, self.functions[i]) for i in range(len(self.functions))]

        base = len(mm)
        for o in self.exposes:
            mm.extend([(base + i, o, o.methods[i]) for i in range(len(o.methods))])
            base += len(o.methods)

        return mm

    @property
    def exposes(self):
        return [ref.object for ref in self.__exposes]
