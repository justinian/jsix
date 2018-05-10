top = '.'
out = 'build'


from waflib.Build import BuildContext
class TestContext(BuildContext):
    cmd = 'test'
    variant = 'tests'


def options(opt):
    opt.load("nasm clang clang++")

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

    ctx.load("nasm clang clang++")
    ctx.find_program("objcopy", var="objcopy")
    ctx.find_program("objdump", var="objdump")
    ctx.find_program("mcopy", var="mcopy")
    ctx.find_program("dd", var="dd")

    # Override the gcc/g++ tools setting these assuming LD is gcc/g++
    ctx.env.SHLIB_MARKER = '-Bdynamic'
    ctx.env.STLIB_MARKER = '-Bstatic'
    ctx.env.LINKFLAGS_cstlib = ['-Bstatic']

    version = subprocess.check_output("git describe --always", shell=True).strip()
    git_sha = subprocess.check_output("git rev-parse --short HEAD", shell=True).strip()

    env = ctx.env
    major, minor, patch_dirty = version.split(".")
    dirty = 'dirty' in patch_dirty
    patch = patch_dirty.split('-')[0]

    ctx.env.POPCORN_ARCH = ctx.options.arch
    ctx.env.KERNEL_FILENAME = ctx.options.kernel_filename
    ctx.env.FONT_NAME = ctx.options.font

    ctx.env.ARCH_D = join(str(ctx.path), "src", "arch",
            ctx.env.POPCORN_ARCH)

    ctx.env.append_value('INCLUDES', [
        join(ctx.path.abspath(), "src", "include"),
        join(ctx.path.abspath(), "src", "include", ctx.env.POPCORN_ARCH),
        join(ctx.path.abspath(), "src", "modules"),
    ])

    modules = []
    mod_root = join("src", "modules")
    for module in os.listdir(mod_root):
        mod_path = join(mod_root, module)
        if exists(join(mod_path, "wscript")):
            modules.append(mod_path)

    baseflags = [
        '-nostdlib',
        '-ffreestanding',
        '-nodefaultlibs',
        '-fno-builtin',
        '-fno-omit-frame-pointer',
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
        '-g',
        '-std=c++14',
        '-fno-exceptions',
        '-fno-rtti',
    ])

    ctx.env.append_value('ASFLAGS', ['-felf64'])

    ctx.env.append_value('LINKFLAGS', [
        '-g',
        '-nostdlib',
        '-znocombreloc',
        '-Bsymbolic',
        '-nostartfiles',
    ])

    env = ctx.env
    ctx.setenv('boot', env=env)
    ctx.recurse(join("src", "boot"))

    ctx.setenv('kernel', env=env)
    ctx.env.append_value('CFLAGS', ['-mcmodel=large'])
    ctx.env.append_value('CXXFLAGS', ['-mcmodel=large'])

    ctx.env.MODULES = modules
    for mod_path in ctx.env.MODULES:
        ctx.recurse(mod_path)

    ctx.recurse(join("src", "kernel"))

    ## Testing configuration
    ##
    from waflib.ConfigSet import ConfigSet
    ctx.setenv('tests', env=ConfigSet())
    ctx.load("clang++")

    ctx.env.append_value('INCLUDES', [
        join(ctx.path.abspath(), "src", "include"),
        join(ctx.path.abspath(), "src", "modules"),
    ])

    ctx.env.CXXFLAGS = ['-g', '-std=c++14', '-fno-rtti']
    ctx.env.LINKFLAGS = ['-g']

    ctx.env.MODULES = modules
    for mod_path in ctx.env.MODULES:
        ctx.recurse(mod_path)
    ctx.recurse(join("src", "tests"))


def build(bld):
    from os.path import join

    if not bld.variant:
        bld.env = bld.all_envs['boot']
        bld.recurse(join("src", "boot"))

        bld.env = bld.all_envs['kernel']
        for mod_path in bld.env.MODULES:
            bld.recurse(mod_path)

        bld.recurse(join("src", "kernel"))

        src = bld.path
        out = bld.path.get_bld()
        kernel_name = bld.env.KERNEL_FILENAME

        disk = out.make_node("popcorn.img")
        disk1 = out.make_node("popcorn.fat")
        font = out.make_node("screenfont.psf")

        bld(
            source = src.make_node(join("assets", "ovmf", "x64", "OVMF.fd")),
            target = out.make_node("flash.img"),
            rule = "cp ${SRC} ${TGT}",
        )

        bld(
            source = src.make_node(join("assets", "fonts", bld.env.FONT_NAME)),
            target = font,
            rule = "cp ${SRC} ${TGT}",
        )

        from waflib.Task import Task
        class mcopy(Task):
            color = 'YELLOW'
            def keyword(self):
                return "Updating"
            def __str__(self):
                node = self.inputs[0]
                return node.path_from(node.ctx.launch_node())
            def run(self):
                from subprocess import check_call as call
                from shutil import copy
                copy(self.inputs[0].abspath(), self.outputs[0].abspath())
                args = self.env.mcopy + ["-i", self.outputs[0].abspath(), "-D", "o"]
                b_args = args + [self.inputs[1].abspath(), "::/efi/boot/bootx64.efi"]
                call(b_args)
                for inp in self.inputs[2:]:
                    call(args + [inp.abspath(), "::/"])

        class addpart(Task):
            color = 'YELLOW'
            def keyword(self):
                return "Updating"
            def __str__(self):
                node = self.inputs[0]
                return node.path_from(node.ctx.launch_node())
            def run(self):
                from subprocess import check_call as call
                from shutil import copy
                copy(self.inputs[0].abspath(), self.outputs[0].abspath())
                args = self.env.dd + [
                    "of={}".format(self.outputs[0].abspath()),
                    "if={}".format(self.inputs[1].abspath()),
                    "bs=512", "count=91669", "seek=2048", "conv=notrunc"]
                call(args)

        copy_img = mcopy(env = bld.env)
        copy_img.set_inputs([
            src.make_node(join("assets", "disk.fat")),
            out.make_node(join("src", "boot", "boot.efi")),
            out.make_node(join("src", "kernel", kernel_name)),
            font,
        ])
        copy_img.set_outputs([disk1])
        bld.add_to_group(copy_img)

        copy_part = addpart(env = bld.env)
        copy_part.set_inputs([
            src.make_node(join("assets", "disk.img")),
            disk1,
        ])
        copy_part.set_outputs([disk])
        bld.add_to_group(copy_part)

    elif bld.variant == 'tests':
        for mod_path in bld.env.MODULES:
            bld.recurse(mod_path)

        bld.recurse(join("src", "tests"))


def qemu(ctx):
    import subprocess
    subprocess.call("rm popcorn.log", shell=True)
    subprocess.call([
        'qemu-system-x86_64',
        '-drive', 'if=pflash,format=raw,file={}/flash.img'.format(out),
        '-drive', 'format=raw,file={}/popcorn.img'.format(out),
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
