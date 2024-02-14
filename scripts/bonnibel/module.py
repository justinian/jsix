from . import include_rel, mod_rel, target_rel
from . import BonnibelError

def resolve(path):
    if path.startswith('/') or path.startswith('$'):
        return path
    from pathlib import Path
    return str(Path(path).resolve())

class BuildOptions:
    def __init__(self, includes=tuple(), local=tuple(), late=tuple(), libs=tuple(), order_only=tuple(), ld_script=None):
        self.includes = list(includes)
        self.local = list(local)
        self.late = list(late)
        self.libs = list(libs)
        self.order_only = list(order_only)
        self.ld_script = ld_script and str(ld_script)

    @property
    def implicit(self):
        libfiles = list(map(lambda x: x[0], self.libs))
        if self.ld_script is not None:
            return libfiles + [self.ld_script]
        else:
            return libfiles

    @property
    def linker_args(self):
        from pathlib import Path
        def arg(path, static):
            if static: return path
            return "-l:" + Path(path).name
        return [arg(*l) for l in self.libs]


class Module:
    __fields = {
        # name: (type, default)
        "kind": (str, "exe"),
        "outfile": (str, None),
        "basename": (str, None),
        "targets": (set, ()),
        "deps":  (set, ()),
        "public_headers":  (set, ()),
        "copy_headers": (bool, False),
        "includes": (tuple, ()),
        "include_phase": (str, "normal"),
        "sources": (tuple, ()),
        "drivers": (tuple, ()),
        "variables": (dict, ()),
        "default": (bool, False),
        "description": (str, None),
        "no_libc": (bool, False),
        "ld_script": (str, None),
        "static": (bool, False),
    }

    def __init__(self, name, modfile, root, **kwargs):
        from pathlib import Path
        from .source import make_source, make_copy_source

        # Required fields
        self.root = Path(root)
        self.name = name
        self.modfile = modfile

        for name, data in self.__fields.items():
            ctor, default = data
            value = kwargs.get(name, default)
            if value is not None:
                value = ctor(value)

            setattr(self, name, value)

        for name in kwargs:
            if not name in self.__fields:
                raise AttributeError(f"No attribute named {name} on Module ({modfile})")

        if not self.no_libc:
            self.deps.add("libc_free")

        # Turn strings into real Source objects
        self.sources = [make_source(root, f) for f in self.sources]

        header_source = lambda f: make_source(root, Path("include") / f)
        if self.copy_headers:
            header_source = lambda f: make_copy_source(root, f, "include")
        self.public_headers = [header_source(f) for f in self.public_headers]

        # Filled by Module.update
        self.depmods = set()

    def __repr__(self):
        return f"<Module {self.kind} {self.name}>"

    @property
    def basename(self):
        if self.__basename is not None:
            return self.__basename
        if self.kind == "lib":
            return f"lib{self.name}"
        else:
            return self.name

    @basename.setter
    def basename(self, value):
        self.__basename = value

    def get_output(self, static=False):
        if self.outfile is not None:
            return self.outfile
        elif self.kind == "headers":
            return None
        else:
            ext = dict(exe=".elf", driver=".drv", lib=(static and ".a" or ".so"))
            return self.basename + ext.get(self.kind, "")

    @classmethod
    def update(cls, mods):
        from . import BonnibelError

        def resolve(source, modlist):
            resolved = set()
            for dep in modlist:
                if not dep in mods:
                    raise BonnibelError(f"module '{source.name}' references unknown module '{dep}'")
                mod = mods[dep]
                resolved.add(mod)
            return resolved

        for mod in mods.values():
            mod.depmods = resolve(mod, mod.deps)

        target_mods = [mod for mod in mods.values() if mod.targets]
        for mod in target_mods:
            closed = set()
            children = set(mod.depmods)
            while children:
                child = children.pop()
                closed.add(child)
                child.targets |= mod.targets
                children |= {m for m in child.depmods if not m in closed}

    def generate(self, output):
        from pathlib import Path
        from collections import defaultdict
        from ninja.ninja_syntax import Writer

        def walk_deps(deps, static, results):
            for mod in deps:
                if static or mod.name not in results:
                    results[mod.name] = (mod, static)
                walk_deps(mod.depmods, static or mod.static, results)

        all_deps = {}
        walk_deps(self.depmods, self.static, all_deps)
        all_deps = all_deps.values()

        def gather_phony(build, deps, child_rel):
            phony = ".headers.phony"
            child_phony = [child_rel(phony, module=c.name)
                    for c, _ in all_deps]

            build.build(
                rule = "touch",
                outputs = [mod_rel(phony)],
                implicit = child_phony,
                order_only = list(map(mod_rel, deps)),
            )

        filename = str(output / f"module.{self.name}.ninja")
        with open(filename, "w") as buildfile:
            build = Writer(buildfile)

            build.comment("This file is automatically generated by bonnibel")
            build.newline()

            build.variable("module_dir", target_rel(self.name + ".dir"))
            build.variable("module_kind", self.kind)
            build.newline()

            build.include(f"${{target_dir}}/config.{self.kind}.ninja")
            build.newline()

            modopts = BuildOptions(
                local = [self.root, "${module_dir}"],
                ld_script = self.ld_script and self.root / self.ld_script,
                )
            if self.public_headers:
                modopts.includes += [
                    self.root / "include",
                    f"${{target_dir}}/{self.name}.dir/include",
                ]

            for key, value in self.variables.items():
                build.variable(key, value)
            build.newline()

            for include in self.includes:
                p = Path(include)
                if p.is_absolute():
                    if not p in modopts.includes:
                        modopts.includes.append(str(p.resolve()))
                elif include != ".":
                    incpath = self.root / p
                    destpath = mod_rel(p)
                    for header in incpath.rglob("*.h"):
                        dest_header = f"{destpath}/" + str(header.relative_to(incpath))
                    modopts.includes.append(str(incpath))
                    modopts.includes.append(destpath)

            for dep, static in all_deps:
                if dep.public_headers:
                    if dep.include_phase == "normal":
                        modopts.includes += [dep.root / "include", f"${{target_dir}}/{dep.name}.dir/include"]
                    elif dep.include_phase == "late":
                        modopts.late += [dep.root / "include", f"${{target_dir}}/{dep.name}.dir/include"]
                    else:
                        from . import BonnibelError
                        raise BonnibelError(f"Module {dep.name} has invalid include_phase={dep.include_phase}")

                if dep.kind == "headers":
                    continue
                elif dep.kind == "lib":
                    modopts.libs.append((target_rel(dep.get_output(static)), static))
                else:
                    modopts.order_only.append(target_rel(dep.get_output(static)))

            cc_includes = []
            if modopts.local:
                cc_includes += [f"-iquote{i}" for i in modopts.local]

            if modopts.includes:
                cc_includes += [f"-I{i}" for i in modopts.includes]

            if modopts.late:
                cc_includes += [f"-idirafter{i}" for i in modopts.late]

            if cc_includes:
                build.variable("ccflags", ["${ccflags}"] + cc_includes)

            as_includes = [f"-I{d}" for d in modopts.local + modopts.includes + modopts.late]
            if as_includes:
                build.variable("asflags", ["${asflags}"] + as_includes)

            if modopts.libs:
                build.variable("libs", ["-L${target_dir}", "${libs}"] + modopts.linker_args)

            if modopts.ld_script:
                build.variable("ldflags", ["${ldflags}"] + ["-T", modopts.ld_script])

            header_deps = []
            inputs = []
            headers = set(self.public_headers)
            while headers:
                source = headers.pop()
                headers.update(source.next)

                if source.action:
                    build.newline()
                    build.build(rule=source.action, **source.args)

                if source.gather:
                    header_deps += list(source.outputs)

                if source.input:
                    inputs.extend(map(mod_rel, source.outputs))

            build.newline()

            inputs = []
            sources = set(self.sources)
            while sources:
                source = sources.pop()
                sources.update(source.next)

                if source.action:
                    build.newline()
                    build.build(rule=source.action, **source.args)

                if source.gather:
                    header_deps += list(source.outputs)

                if source.input:
                    inputs.extend(map(mod_rel, source.outputs))

            gather_phony(build, header_deps, target_rel)

            if self.kind == "headers":
                # Header-only, don't output a build rule
                return

            output = target_rel(self.get_output())
            build.newline()
            build.build(
                rule = self.kind,
                outputs = output,
                inputs = inputs,
                implicit = modopts.implicit,
                order_only = modopts.order_only,
                variables = {"name": self.name,
                             "soname": self.get_output()},
            )

            dump = output + ".dump"
            build.newline()
            build.build(
                rule = "dump",
                outputs = dump,
                inputs = output,
                variables = {"name": self.name},
            )

            s_output = target_rel(self.get_output(static=True))
            if s_output != output:
                build.newline()
                build.build(
                    rule = self.kind + "_static",
                    outputs = s_output,
                    inputs = inputs,
                    order_only = modopts.order_only,
                    variables = {"name": self.name},
                )

                dump = s_output + ".dump"
                build.newline()
                build.build(
                    rule = "dump",
                    outputs = dump,
                    inputs = s_output,
                    variables = {"name": self.name},
                )

            if self.default:
                build.newline()
                build.default(output)
                build.default(dump)

    def add_input(self, path, **kwargs):
        from .source import make_source
        s = make_source(self.root, path, **kwargs)
        self.sources.append(s)
        return s.outputs

    def add_depends(self, paths, deps):
        for source in self.sources:
            if source.path in paths:
                source.add_deps(deps)

        for source in self.public_headers:
            if source.path in paths:
                source.add_deps(deps)
