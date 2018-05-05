top = '.'
out = 'build'


def options(opt):
    opt.load("nasm gcc g++")

    opt.add_option(
            '--arch',
            action='store',
            default='x86_64',
            help='Target architecture')

    opt.add_option('--kernel_filename',
            action='store',
            default='popcorn.elf',
            help='Kernel filename on disk')

    opt.add_option('--font',
            action='store',
            default='tamsyn8x16r.psf',
            help='Font for the console')


def configure(ctx):
    import os
    import subprocess
    from os.path import join, exists

    ctx.find_program("ld", var="LINK_CC")
    ctx.env.LINK_CXX = ctx.env.LINK_CC

    ctx.load("nasm gcc g++")
    ctx.find_program("objcopy", var="objcopy")
    ctx.find_program("objdump", var="objdump")
    ctx.find_program("mcopy", var="mcopy")

    # Override the gcc/g++ tools setting these assuming LD is gcc/g++
    ctx.env.SHLIB_MARKER = '-Bdynamic'
    ctx.env.STLIB_MARKER = '-Bstatic'
    ctx.env.LINKFLAGS_cstlib = ['-Bstatic']

    version = subprocess.check_output("git describe --always", shell=True).strip()
    git_sha = subprocess.check_output("git rev-parse --short HEAD", shell=True).strip()

    major, minor, patch_dirty = version.split(".")
    dirty = 'dirty' in patch_dirty
    patch = patch_dirty.split('-')[0]

    ctx.env.POPCORN_ARCH = ctx.options.arch
    ctx.env.KERNEL_FILENAME = ctx.options.kernel_filename
    ctx.env.FONT_NAME = ctx.options.font

    ctx.env.EXTERNAL = join(ctx.path.abspath(), "external")

    baseflags = [
        '-nostdlib',
        '-ffreestanding',
        '-nodefaultlibs',
        '-fno-builtin',
        '-fomit-frame-pointer',
        '-mno-red-zone',
    ]

    warnflags = ['-W{}'.format(opt) for opt in [
        'format=2',
        'init-self',
        'float-equal',
        'inline',
        'missing-format-attribute',
        'missing-include-dirs',
        'switch',
        'undef',
        'disabled-optimization',
        'pointer-arith',
        'no-attributes',
        'no-sign-compare',
        'no-multichar',
        'no-div-by-zero',
        'no-endif-labels',
        'no-pragmas',
        'no-format-extra-args',
        'no-unused-result',
        'no-deprecated-declarations',
        'no-unused-function',

        'error'
    ]]

    ctx.env.append_value('DEFINES', [
        'GIT_VERSION="{}"'.format(version),
        'GIT_VERSION_WIDE=L"{}"'.format(version),
        "VERSION_MAJOR={}".format(major),
        "VERSION_MINOR={}".format(minor),
        "VERSION_PATCH={}".format(patch),
        "VERSION_GITSHA=0x{}{}".format({True:1}.get(dirty, 0), git_sha),
    ])

    ctx.env.append_value('CFLAGS', baseflags)
    ctx.env.append_value('CFLAGS', warnflags)
    ctx.env.append_value('CFLAGS', ['-ggdb', '-std=c11'])

    ctx.env.append_value('CXXFLAGS', baseflags)
    ctx.env.append_value('CXXFLAGS', warnflags)
    ctx.env.append_value('CXXFLAGS', [
        '-ggdb',
        '-std=c++14',
        '-fno-exceptions',
        '-fno-rtti',
    ])

    ctx.env.append_value('ASFLAGS', ['-felf64'])

    ctx.env.append_value('INCLUDES', [
        join(ctx.path.abspath(), "src", "include"),
        join(ctx.path.abspath(), "src", "include", ctx.env.POPCORN_ARCH),
        join(ctx.path.abspath(), "src", "modules"),
    ])

    ctx.env.append_value('LINKFLAGS', [
        '-g',
        '-nostdlib',
        '-znocombreloc',
        '-Bsymbolic',
        '-nostartfiles',
    ])

    ctx.env.ARCH_D = join(str(ctx.path), "src", "arch",
            ctx.env.POPCORN_ARCH)

    env = ctx.env
    ctx.setenv('boot', env=env)
    ctx.recurse(join("src", "boot"))

    ctx.setenv('kernel', env=env)
    ctx.env.append_value('CFLAGS', ['-mcmodel=large'])
    ctx.env.append_value('CXXFLAGS', ['-mcmodel=large'])

    mod_root = join("src", "modules")
    for module in os.listdir(mod_root):
        mod_path = join(mod_root, module)
        if exists(join(mod_path, "wscript")):
            ctx.env.append_value('MODULES', mod_path)
            ctx.recurse(mod_path)

    ctx.recurse(join("src", "kernel"))


def build(bld):
    from os.path import join

    bld.env = bld.all_envs['boot']
    bld.recurse(join("src", "boot"))

    bld.env = bld.all_envs['kernel']
    for mod_path in bld.env.MODULES:
        bld.recurse(mod_path)

    bld.recurse(join("src", "kernel"))

    src = bld.path
    out = bld.root.make_node(bld.out_dir)
    kernel_name = bld.env.KERNEL_FILENAME

    bld(
        source = src.make_node(join("assets", "floppy.img")),
        target = out.make_node("popcorn.img"),
        rule = "cp ${SRC} ${TGT}",
    )

    bld(
        source = src.make_node(join("assets", "ovmf", "x64", "OVMF.fd")),
        target = out.make_node("flash.img"),
        rule = "cp ${SRC} ${TGT}",
    )

    bld(
        source = [
            out.make_node(join("src", "boot", "boot.efi")),
            out.make_node(join("src", "kernel", kernel_name)),
            src.make_node(join("assets", "fonts", bld.env.FONT_NAME)),
        ],
        rule = "; ".join([
            "${mcopy} -i popcorn.img ${SRC[0]} ::/efi/boot/bootx64.efi",
            "${mcopy} -i popcorn.img ${SRC[1]} ::/",
            "${mcopy} -i popcorn.img ${SRC[2]} ::/screenfont.psf",
        ]),
    )


def qemu(ctx):
    import subprocess
    subprocess.call("rm popcorn.log", shell=True)
    subprocess.call([
        'qemu-system-x86_64',
        '-drive', 'if=pflash,format=raw,file={}/flash.img'.format(out),
        '-drive', 'if=floppy,format=raw,file={}/popcorn.img'.format(out),
        '-smp', '1',
        '-m', '512',
        '-d', 'mmu,int,guest_errors',
        '-D', 'popcorn.log',
        '-cpu', 'Broadwell',
        '-M', 'q35',
        '-no-reboot',
        '-nographic',
    ])


def vbox(ctx):
    import os
    from shutil import copy
    from subprocess import call

    dest = os.getenv("VBOX_DEST")
    copy("{}/popcorn.img".format(out), "{}/popcorn.img".format(dest))
    call("nc -l -p 5555", shell=True)


# vim: ft=python et
