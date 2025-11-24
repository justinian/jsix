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
        "target": (str, None),
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
        "arch_source": (dict, ()),
        "skip_arches": (tuple, ()),
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
        for arch in self.arch_source:
            self.arch_source[arch] = [make_source(root, f) for f in self.arch_source[arch]]

        header_source = lambda f: make_source(root, Path("include") / f)
        if self.copy_headers:
            header_source = lambda f: make_copy_source(root, f, "include")
        self.public_headers = [header_source(f) for f in self.public_headers]

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


class ModuleList:
    def __init__(self, arch):
        self.__arch = arch
        self.__mods = {}
        self.__used = {}
        self.__targets = frozenset()
        self.__kinds = frozenset()

    def __getitem__(self, name):
        return self.__mods.get(name)

    def __contains__(self, name):
        """Return if the module name is known."""
        return name in self.__mods

    def __iter__(self):
        """Iterate over _non-skipped_ modules."""
        return self.used.__iter__()

    def __skip(self, mod):
        if self.__arch in mod.skip_arches: return True
        return False

    @property
    def used(self):
        if self.__used is None:
            self.__used = {n:m for n,m in self.__mods.items() if not self.__skip(m)}
        return self.__used

    @property
    def targets(self):
        if self.__targets is None:
            self.__targets = frozenset([m.target for m in self.used.values() if m.target])
        return self.__targets

    @property
    def kinds(self):
        if self.__kinds is None:
            self.__kinds = frozenset([m.kind for m in self.used.values()])
        return self.__kinds

    def get(self, name):
        return self.__mods.get(name)

    def add(self, mod):
        if mod.name in self.__mods:
            raise BonnibelError(f"re-adding module '{mod.name}' to this ModuleList")
        self.__mods[mod.name] = mod
        self.__used = None
        self.__targets = None
        self.__kinds = None

    def get_mods(self, names, filt=None):
        return {self[n] for n in names
                if n in self.used
                and ((not filt) or filt(self[n]))}

    def all_deps(self, mods, stop_at_static=False):
        search = set(mods)
        closed = list()

        while search:
            mod = search.pop()
            if mod in closed: continue
            closed.append(mod)

            if stop_at_static and mod.static:
                continue

            for dep in mod.deps:
                if not dep in self.__mods:
                    raise BonnibelError(f"module '{mod.name}' references unknown module '{dep}'")
                if dep in self.used:
                    search.add(self.used[dep])

        return closed

    def target_mods(self, target):
        return self.all_deps([m for m in self.used.values() if m.target == target])

    def generate(self, output):
        from pathlib import Path
        from collections import defaultdict
        from ninja.ninja_syntax import Writer

        def walk_deps(deps, static, results):
            for modname in deps:
                mod = self.used.get(modname)
                if not mod: continue # skipped
                if static or modname not in results:
                    results[modname] = (mod, static)
                walk_deps(mod.deps, static or mod.static, results)

        for mod in self.used.values():
            all_deps = {}
            walk_deps(mod.deps, mod.static, all_deps)
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

            filename = str(output / f"module.{mod.name}.ninja")
            with open(filename, "w") as buildfile:
                build = Writer(buildfile)

                build.comment("This file is automatically generated by bonnibel")
                build.newline()

                build.variable("module_dir", target_rel(mod.name + ".dir"))
                build.variable("module_kind", mod.kind)
                build.newline()

                build.include(f"${{target_dir}}/config.{mod.kind}.ninja")
                build.newline()

                modopts = BuildOptions(
                    local = [mod.root, "${module_dir}"],
                    ld_script = mod.ld_script and mod.root / mod.ld_script,
                    )
                if mod.public_headers:
                    modopts.includes += [
                        mod.root / "include",
                        f"${{target_dir}}/{mod.name}.dir/include",
                    ]

                for key, value in mod.variables.items():
                    build.variable(key, value)
                build.newline()

                for include in mod.includes:
                    p = Path(include)
                    if p.is_absolute():
                        if not p in modopts.includes:
                            modopts.includes.append(str(p.resolve()))
                    elif include != ".":
                        incpath = mod.root / p
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
                headers = set(mod.public_headers)
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
                sources = set(mod.sources)
                sources.update(set(mod.arch_source.get(self.__arch, tuple())))
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

                if mod.kind == "headers":
                    # Header-only, don't output a build rule
                    continue

                mod_output = target_rel(mod.get_output())
                build.newline()
                build.build(
                    rule = mod.kind,
                    outputs = mod_output,
                    inputs = inputs,
                    implicit = modopts.implicit,
                    order_only = modopts.order_only,
                    variables = {"name": mod.name,
                                 "soname": mod.get_output()},
                )

                dump = mod_output + ".dump"
                build.newline()
                build.build(
                    rule = "dump",
                    outputs = dump,
                    inputs = mod_output,
                    variables = {"name": mod.name},
                )

                s_output = target_rel(mod.get_output(static=True))
                if s_output != mod_output:
                    build.newline()
                    build.build(
                        rule = mod.kind + "_static",
                        outputs = s_output,
                        inputs = inputs,
                        order_only = modopts.order_only,
                        variables = {"name": mod.name},
                    )

                    dump = s_output + ".dump"
                    build.newline()
                    build.build(
                        rule = "dump",
                        outputs = dump,
                        inputs = s_output,
                        variables = {"name": mod.name},
                    )

                if mod.default:
                    build.newline()
                    build.default(mod_output)
                    build.default(dump)


