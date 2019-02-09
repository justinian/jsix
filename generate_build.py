#!/usr/bin/env python3

from collections import namedtuple

library = namedtuple('library', ['path', 'deps'])
program = namedtuple('program', ['path', 'deps', 'output', 'targets'])
source = namedtuple('source', ['name', 'input', 'output', 'action'])
version = namedtuple('version', ['major', 'minor', 'patch', 'sha', 'dirty'])

MODULES = {}

class Source:
    Actions = {'.c': 'cc', '.cpp': 'cxx', '.s': 'nasm'}

    def __init__(self, path, root, modroot):
        from os.path import relpath, splitext
        self.input = path
        self.name = relpath(path, root)
        self.output = relpath(path, modroot) + ".o"
        self.action = self.Actions.get(splitext(path)[1], None)

    def __str__(self):
        return "{} {}:{}:{}".format(self.action, self.output, self.name, self.input)


class Module:
    def __init__(self, name, output, root, **kwargs):
        from os.path import commonpath, dirname, isdir, join

        self.name = name
        self.output = output
        self.kind = kwargs.get("kind", "exe")
        self.target = kwargs.get("target", None)
        self.deps = kwargs.get("deps", tuple())
        self.includes = kwargs.get("includes", tuple())
        self.defines = kwargs.get("defines", tuple())
        self.depmods = []

        sources = [join(root, f) for f in kwargs.get("source", tuple())]
        modroot = commonpath(sources)
        while not isdir(modroot):
            modroot = dirname(modroot)

        self.sources = [Source(f, root, modroot) for f in sources]

    def __str__(self):
        return "Module {} {}\n\t".format(self.kind, self.name)

    def __find_depmods(self, modules):
        self.depmods = set()
        open_list = set(self.deps)
        closed_list = set()

        while open_list:
            dep = modules[open_list.pop()]
            open_list |= (set(dep.deps) - closed_list)
            self.depmods.add(dep)

        self.libdeps = [d for d in self.depmods if d.kind == "lib"]
        self.exedeps = [d for d in self.depmods if d.kind != "lib"]

    @classmethod
    def load(cls, filename):
        from os.path import abspath, dirname
        from yaml import load

        root = dirname(filename)
        modules = {}
        moddata = load(open(filename, "r"))

        for name, data in moddata.items():
            modules[name] = cls(name, root=root, **data)

        for mod in modules.values():
            mod.__find_depmods(modules)

        targets = {}
        for mod in modules.values():
            if mod.target is None: continue
            if mod.target not in targets:
                targets[mod.target] = set()
            targets[mod.target].add(mod)
            targets[mod.target] |= mod.depmods

        return modules.values(), targets


def get_template(env, typename, name):
    from jinja2.exceptions import TemplateNotFound
    try:
        return env.get_template("{}.{}.j2".format(typename, name))
    except TemplateNotFound:
        return env.get_template("{}.default.j2".format(typename))


def get_git_version():
    from subprocess import run
    cp = run(['git', 'describe', '--dirty'],
            check=True, capture_output=True)
    full_version = cp.stdout.decode('utf-8').strip()

    cp = run(['git', 'rev-parse', 'HEAD'],
            check=True, capture_output=True)
    full_sha = cp.stdout.decode('utf-8').strip()

    dirty = False
    parts1 = full_version.split('-')
    if parts1[-1] == "dirty":
        dirty = True
        parts1 = parts1[:-1]

    parts2 = parts1[0].split('.')

    return version(
        parts2[0],
        parts2[1],
        parts2[2],
        full_sha[:7],
        dirty)


def main(buildroot="build", modulefile="modules.yaml"):
    import os
    from os.path import abspath, dirname, isabs, isdir, join

    generator = abspath(__file__)
    srcroot = dirname(generator)
    if not isabs(modulefile):
        modulefile = join(srcroot, modulefile)

    if not isabs(buildroot):
        buildroot = join(srcroot, buildroot)

    if not isdir(buildroot):
        os.mkdir(buildroot)

    git_version = get_git_version()
    print("Generating build files for Popcorn {}.{}.{}-{}...".format(
        git_version.major, git_version.minor, git_version.patch, git_version.sha))

    from jinja2 import Environment, FileSystemLoader
    template_dir = join(srcroot, "scripts", "templates")
    env = Environment(loader=FileSystemLoader(template_dir))

    buildfiles = []
    templates = set()
    modules, targets = Module.load(modulefile)

    for mod in modules:
        buildfile = join(buildroot, mod.name + ".ninja")
        buildfiles.append(buildfile)
        with open(buildfile, 'w') as out:
            template = get_template(env, mod.kind, mod.name)
            templates.add(template.filename)
            out.write(template.render(
                module=mod,
                buildfile=buildfile,
                version=git_version))

    for target, mods in targets.items():
        root = join(buildroot, target)
        if not isdir(root):
            os.mkdir(root)

        buildfile = join(root, "target.ninja")
        buildfiles.append(buildfile)
        with open(buildfile, 'w') as out:
            template = get_template(env, "target", target)
            templates.add(template.filename)
            out.write(template.render(
                target=target,
                modules=mods,
                buildfile=buildfile,
                version=git_version))

    # Top level buildfile cannot use an absolute path or ninja won't
    # reload itself properly on changes.
    # See: https://github.com/ninja-build/ninja/issues/1240
    buildfile = "build.ninja"
    buildfiles.append(buildfile)

    with open(join(buildroot, buildfile), 'w') as out:
        template = env.get_template("build.ninja.j2")
        templates.add(template.filename)

        out.write(template.render(
            targets=targets,
            buildroot=buildroot,
            srcroot=srcroot,
            buildfile=buildfile,
            buildfiles=buildfiles,
            templates=[abspath(f) for f in templates],
            generator=generator,
            modulefile=modulefile,
            version=git_version))

if __name__ == "__main__":
    import sys
    main(*sys.argv[1:])
