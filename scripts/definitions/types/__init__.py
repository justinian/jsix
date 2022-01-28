def _indent(x):
    from textwrap import indent
    return indent(str(x), '    ')

class CName(str): pass
class Description(str): pass
class Import(str): pass
class Caps(list): pass

class Options(dict):
    def __init__(self, opts = tuple()):
        for opt in opts:
            parts = opt.split(":", 1)
            self[parts[0]] = self.get(parts[0], []) + ["".join(parts[1:])]

class UID(int):
    def __str__(self):
        return f"{self:016x}"

from .object import Object
from .interface import Interface, Expose
from .function import Function, Method, Param

from .type import Type
from .primitive import get_primitive
from .objref import ObjectRef
