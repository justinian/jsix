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

    "nulldrv":   program('src/drivers/nulldrv', [], "nulldrv", ["host"]),

    "boot":      program('src/boot', ["elf"], "boot.elf", ["host"]),
    "kernel":    program('src/kernel', ["elf", "initrd", "kutil"], "popcorn.elf", ["host"]),
}


def get_template(env, typename, name):
    from jinja2.exceptions import TemplateNotFound
    try:
        return env.get_template("{}.{}.ninja.j2".format(typename, name))
    except TemplateNotFound:
        return env.get_template("{}.default.ninja.j2".format(typename))


def get_sources(path, srcroot):
    import os
    from os.path import abspath, join, relpath, splitext

    actions = {'.c': 'cc', '.cpp': 'cxx', '.s': 'nasm'}

    sources = []
    for root, dirs, files in os.walk(path):
        for f in files:
            base, ext = splitext(f)
            if not ext in actions: continue
            name = join(root, f)
            sources.append(
                source(
                    relpath(name, srcroot),
                    abspath(name),
                    f + ".o",
                    actions[ext]))

    return sources


def get_git_version():
    from subprocess import run
    cp = run(['git', 'describe', '--always'],
            check=True, capture_output=True)
    full_version = cp.stdout.decode('utf-8').strip()

    parts1 = full_version.split('-')
    parts2 = parts1[0].split('.')

    return version(
        parts2[0],
        parts2[1],
        parts2[2],
        parts1[-1])


def main(buildroot):
    import os
    from os.path import abspath, dirname, isdir, join

    generator = abspath(__file__)
    srcroot = dirname(generator)

    if buildroot is None:
        buildroot = join(srcroot, "build")

    if not isdir(buildroot):
        os.mkdir(buildroot)

    git_version = get_git_version()
    print("Generating build files for Popcorn {}.{}.{}-{}...".format(
        git_version.major, git_version.minor, git_version.patch, git_version.sha))

    from jinja2 import Environment, FileSystemLoader
    env = Environment(loader=FileSystemLoader(join(srcroot, "scripts", "templates")))

    targets = {}
    templates = set()
    buildfiles = []
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

        sources = get_sources(join(srcroot, mod.path), join(srcroot, "src"))
        buildfile = join(buildroot, name + ".ninja")
        buildfiles.append(buildfile)
        with open(buildfile, 'w') as out:
            #print("Generating module", name)
            template = get_template(env, type(mod).__name__, name)
            templates.add(template.filename)
            out.write(template.render(
                name=name,
                module=mod,
                sources=sources,
                buildfile=buildfile,
                version=git_version))

    for target, mods in targets.items():
        root = join(buildroot, target)
        if not isdir(root):
            os.mkdir(root)

        buildfile = join(root, "target.ninja")
        buildfiles.append(buildfile)
        with open(buildfile, 'w') as out:
            #print("Generating target", target)
            template = get_template(env, "target", target)
            templates.add(template.filename)
            out.write(template.render(
                target=target,
                modules=mods,
                buildfile=buildfile,
                version=git_version))

    buildfile = join(buildroot, "build.ninja")
    buildfiles.append(buildfile)
    with open(buildfile, 'w') as out:
        #print("Generating main build.ninja")
        template = env.get_template('build.ninja.j2')
        templates.add(template.filename)

        out.write(template.render(
            targets=targets,
            buildroot=buildroot,
            srcroot=srcroot,
            buildfile=buildfile,
            buildfiles=buildfiles,
            templates=[abspath(f) for f in templates],
            generator=generator,
            version=git_version))

if __name__ == "__main__":
    import sys
    buildroot = None
    if len(sys.argv) > 1:
        buildroot = sys.argv[1]
    main(buildroot)
