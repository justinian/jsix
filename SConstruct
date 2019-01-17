from os.path import join

import SCons.Node
def target_from_source_nosplit(node, prefix, suffix, splitext):
    return node.dir.Entry(prefix + node.name + suffix)

SCons.Node._target_from_source_map['nosplit'] = target_from_source_nosplit
SCons.Node._target_from_source_map[1] = target_from_source_nosplit

def RGlob(path, pattern):
    from os import walk
    from itertools import chain
    return list(chain.from_iterable([Glob(join(d, pattern)) for d, _, _ in walk(path)]))

def subdirs(path):
    from os import listdir
    from os.path import isdir, join
    return [d for d in listdir(path) if isdir(join(path, d))]

SetOption('implicit_cache', 1)
Decider('MD5-timestamp')
VariantDir('src', 'build', duplicate=0)

env = SConscript('scons/default_env.scons')
target = SConscript('scons/target_env.scons', 'env')

libs = {}
for lib in subdirs('src/libraries'):
    libs[lib] = SConscript(join('src/libraries', lib, 'SConscript'), 'target RGlob')

kernel = target.Clone()
kernel.Append(CCFLAGS = [
    '-mno-sse',
    '-mno-red-zone',
])
kernel.Append(
    CPPPATH = [
        'src/kernel',
        'src/libraries/elf/include',
        'src/libraries/initrd/include',
        'src/libraries/kutil/include',
    ],
    ASFLAGS = ['-I', 'src/kernel/'],
    LIBS = libs.values() + ['c'],
    LINKFLAGS = ['-T', File('#src/arch/x86_64/kernel.ld').abspath],
)

kernel.Program(
    'popcorn.elf',
    RGlob('src/kernel', '*.cpp') +
    RGlob('src/kernel', '*.s'))

# vim: ft=python et
