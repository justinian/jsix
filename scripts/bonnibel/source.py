from os.path import join, splitext
from . import mod_rel

def _resolve(path):
    if path.startswith('/') or path.startswith('$'):
        return path
    from pathlib import Path
    return str(Path(path).resolve())

def _dynamic_action(name):
    def prop(self):
        root, suffix = splitext(self.path)
        return f"{name}{suffix}"
    return prop


class Source:
    next = tuple()
    action = None
    args = dict()
    gather = False
    outputs = tuple()
    input = False

    def __init__(self, path, root = "${module_dir}", deps=tuple()): 
        self.path = path
        self.root = root
        self.deps = deps

    def add_deps(self, deps):
        self.deps += tuple(deps)

    @property
    def fullpath(self):
        return join(self.root, self.path)

class ParseSource(Source):
    action = property(_dynamic_action("parse"))

    @property
    def output(self):
        root, _ = splitext(self.path)
        return root

    @property
    def outputs(self):
        return (self.output,)

    @property
    def gather(self):
        _, suffix = splitext(self.output)
        return suffix in (".h", ".inc")

    @property
    def next(self):
        _, suffix = splitext(self.output)
        nextType = {
            ".s": CompileSource,
            ".cpp": CompileSource,
            }.get(suffix)

        if nextType:
            return (nextType(self.output),)
        return tuple()

    @property
    def args(self):
        return dict(
            outputs = list(map(mod_rel, self.outputs)),
            inputs = [self.fullpath],
            implicit = list(map(_resolve, self.deps)),
            variables = dict(name=self.path),
            )

class HeaderSource(Source):
    action = "cp"
    gather = True

    @property
    def outputs(self):
        return (self.path,)

    @property
    def args(self):
        return dict(
            outputs = [mod_rel(self.path)],
            inputs = [join(self.root, self.path)],
            implicit = list(map(_resolve, self.deps)),
            variables = dict(name=self.path),
            )

class CompileSource(Source):
    action = property(_dynamic_action("compile"))
    input = True

    @property
    def outputs(self):
        return (self.path + ".o",)

    @property
    def args(self):
        return dict(
            outputs = list(map(mod_rel, self.outputs)),
            inputs = [join(self.root, self.path)],
            implicit = list(map(_resolve, self.deps)) + [mod_rel(".headers.phony")],
            variables = dict(name=self.path),
            )

def make_source(root, path):
    _, suffix = splitext(path)

    if suffix in (".s", ".c", ".cpp"):
        return CompileSource(path, root)
    elif suffix in (".cog",):
        return ParseSource(path, root)
    elif suffix in (".h", ".inc"):
        return HeaderSource(path, root)
    else:
        raise RuntimeError(f"{path} has no Source type")
