#!/usr/bin/env python3

from collections import namedtuple

library = namedtuple('library', ['path', 'deps'])
program = namedtuple('program', ['path', 'deps', 'output', 'targets'])
source = namedtuple('source', ['name', 'input', 'output', 'action'])
version = namedtuple('version', ['major', 'minor', 'patch', 'sha'])

MODULES = {
    "elf":       library('src/libraries/elf', ['kutil']),
    "initrd":    library('src/libraries/initrd', ["kutil"]),
    "kutil":     library('src/libraries/kutil', []),

    "makerd":    program('src/tools/makerd', ["initrd", "kutil"], "makerd", ["native"]),
    "boot":      program('src/boot', ["elf"], "boot.elf", ["host"]),
    "kernel":    program('src/kernel', ["elf", "initrd", "kutil"], "popcorn.elf", ["host"]),
}


def get_template(env, typename, name):
    from jinja2.exceptions import TemplateNotFound
    try:
        return env.get_template("{}.{}.ninja.j2".format(typename, name))
    except TemplateNotFound:
        return env.get_template("{}.default.ninja.j2".format(typename))


def get_sources(path):
    import os
    from os.path import abspath, join, splitext

    actions = {'.c': 'cc', '.cpp': 'cxx', '.s': 'nasm'}

    sources = []
    for root, dirs, files in os.walk(path):
        for f in files:
            base, ext = splitext(f)
            if not ext in actions: continue
            name = join(root, f)
            sources.append(
                source(
                    name,
                    abspath(name),
                    f + ".o",
                    actions[ext]))

    return sources


def get_git_version():
    return version(0,5,0,'aaaaaa')


def main(buildroot):
    import os
    from os.path import abspath, dirname, isdir, join

    buildroot = abspath(buildroot)
    srcroot = dirname(abspath(__file__))
    if not isdir(buildroot):
        os.mkdir(buildroot)

    git_version = get_git_version()

    from jinja2 import Environment, FileSystemLoader
    env = Environment(loader=FileSystemLoader("scripts/templates"))

    targets = {}
    for name, mod in MODULES.items():
        if isinstance(mod, program):
            for target in mod.targets:
                if not target in targets:
                    targets[target] = set()

                open_list = [name]
                while open_list:
                    depname = open_list.pop()
                    dep = MODULES[depname]
                    open_list.extend(dep.deps)
                    targets[target].add(depname)

        sources = get_sources(mod.path)
        with open(join(buildroot, name + ".ninja"), 'w') as out:
            print("Generating module", name)
            template = get_template(env, type(mod).__name__, name)
            out.write(template.render(
                name=name,
                module=mod,
                sources=sources,
                version=git_version))

    for target, mods in targets.items():
        root = join(buildroot, target)
        if not isdir(root):
            os.mkdir(root)

        with open(join(root, "target.ninja"), 'w') as out:
            print("Generating target", target)
            template = get_template(env, "target", target)
            out.write(template.render(
                target=target,
                modules=mods,
                version=git_version))

    with open(join(buildroot, 'build.ninja'), 'w') as out:
        print("Generating main build.ninja")
        template = env.get_template('build.ninja.j2')
        out.write(template.render(
            targets=targets,
            buildroot=buildroot,
            srcroot=srcroot,
            version=git_version))

if __name__ == "__main__":
    import sys
    buildroot = "build"
    if len(sys.argv) > 1:
        buildroot = sys.argv[1]
    main(buildroot)
