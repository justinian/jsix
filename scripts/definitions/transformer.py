from .parser import Transformer, v_args

def get_opts(args):
    from .types import Caps, CName, Description, Options, Type, UID

    kinds = {
        Description: "desc",
        Options: "opts",
        CName: "cname",
        Caps: "caps",
        UID: "uid",
        Type: "typename",
    }

    result = dict()
    outargs = []
    for a in args:
        for kind, name in kinds.items():
            if isinstance(a, kind):
                result[name] = a
                break
        else:
            outargs.append(a)

    return result, outargs

class DefTransformer(Transformer):
    def __init__(self, filename):
        self.filename = filename

    def start(self, args):
        from .types import Import, Interface, Object

        imports = set()
        objects = dict()
        interfaces = dict()

        for o in args:
            if isinstance(o, Object):
                objects[o.name] = o

            elif isinstance(o, Interface):
                interfaces[o.name] = o

            elif isinstance(o, Import):
                imports.add(o)

        return imports, objects, interfaces

    @v_args(inline=True)
    def import_statement(self, path):
        from .types import Import
        return Import(path)

    def object(self, args):
        from .types import Object
        specials, args = get_opts(args)
        name, args = args[0], args[1:]
        return Object(name, children=args, **specials)

    def interface(self, args):
        from .types import Interface
        specials, args = get_opts(args)
        name, args = args[0], args[1:]
        return Interface(name, children=args, **specials)

    def method(self, args):
        from .types import Method
        specials, args = get_opts(args)
        name, args = args[0], args[1:]
        return Method(name, children=args, **specials)

    def function(self, args):
        from .types import Function
        specials, args = get_opts(args)
        name, args = args[0], args[1:]
        return Function(name, children=args, **specials)

    def param(self, args):
        from .types import Param
        specials, args = get_opts(args)
        name = args[0]
        return Param(name, **specials)

    @v_args(inline=True)
    def expose(self, s):
        from .types import Expose
        return Expose(s)

    @v_args(inline=True)
    def uid(self, s):
        return s

    @v_args(inline=True)
    def cname(self, s):
        from .types import CName
        return CName(s)

    @v_args(inline=True)
    def name(self, s):
        return s

    @v_args(inline=True)
    def type(self, s):
        return s

    @v_args(inline=True)
    def super(self, s):
        from .types import ObjectRef
        return ObjectRef(s, self.filename)

    def options(self, args):
        from .types import Options
        return Options([str(s) for s in args])

    def capabilities(self, args):
        from .types import Caps
        return Caps([str(s) for s in args])

    def description(self, s):
        from .types import Description
        return Description("\n".join(s))

    @v_args(inline=True)
    def object_name(self, n):
        from .types import ObjectRef
        return ObjectRef(n, self.filename)

    def PRIMITIVE(self, s):
        from .types import get_primitive
        return get_primitive(s)

    def UID(self, s):
        from .types import UID
        return UID(int(s, base=16))

    def INT_TYPE(self, s):
        return s

    def NUMBER(self, s):
        if s.startswith("0x"):
            return int(s,16)
        return int(s)

    def COMMENT(self, s):
        return s[2:].strip()

    def OPTION(self, s):
        return str(s)

    def IDENTIFIER(self, s):
        return str(s)

    def PATH(self, s):
        return str(s[1:-1])

