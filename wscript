top = '.'
out = 'build'


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


def common_configure(ctx):
    from os import listdir
    from os.path import join, exists
    from subprocess import check_output

    version = check_output("git describe --always", shell=True).strip()
    git_sha = check_output("git rev-parse --short HEAD", shell=True).strip()

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
        join(ctx.path.abspath(), "src", "libraries"),
        join(ctx.path.abspath(), "src", "drivers"),
    ])

    libraries = []
    mod_root = join("src", "libraries")
    for module in listdir(mod_root):
        mod_path = join(mod_root, module)
        if exists(join(mod_path, "wscript")):
            libraries.append(mod_path)
    ctx.env.LIBRARIES = libraries

    tools = []
    mod_root = join("src", "tools")
    for module in listdir(mod_root):
        mod_path = join(mod_root, module)
        if exists(join(mod_path, "wscript")):
            tools.append(mod_path)
    ctx.env.TOOLS = tools

    drivers = []
    mod_root = join("src", "drivers")
    for module in listdir(mod_root):
        mod_path = join(mod_root, module)
        if exists(join(mod_path, "wscript")):
            drivers.append(mod_path)
    ctx.env.DRIVERS = drivers

    ctx.env.append_value('DEFINES', [
        'GIT_VERSION="{}"'.format(version),
        'GIT_VERSION_WIDE=L"{}"'.format(version),
        "VERSION_MAJOR={}".format(major),
        "VERSION_MINOR={}".format(minor),
        "VERSION_PATCH={}".format(patch),
        "VERSION_GITSHA=0x{}{}".format({True:1}.get(dirty, 0), git_sha),
    ])

    ctx.env.append_value('QEMUOPTS', [
        '-smp', '1',
        '-m', '512',
        '-d', 'mmu,int,guest_errors',
        '-D', 'popcorn.log',
        '-cpu', 'Broadwell',
        '-M', 'q35',
        '-no-reboot',
        '-nographic',
    ])

    if exists('/dev/kvm'):
        ctx.env.append_value('QEMUOPTS', ['--enable-kvm'])


def configure(ctx):
    from os.path import join, exists

    ctx.find_program("ld", var="LINK_CC")
    ctx.env.LINK_CXX = ctx.env.LINK_CC

    ctx.load("nasm clang clang++")
    ctx.find_program("objcopy", var="objcopy")
    ctx.find_program("objdump", var="objdump")

    # Override the gcc/g++ tools setting these assuming LD is gcc/g++
    ctx.env.SHLIB_MARKER = '-Bdynamic'
    ctx.env.STLIB_MARKER = '-Bstatic'
    ctx.env.LINKFLAGS_cstlib = ['-Bstatic']

    baseflags = [
        '-nostdlib',
        '-ffreestanding',
        '-nodefaultlibs',
        '-fno-builtin',
        '-mno-sse',
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

    common_configure(ctx)
    base_bare = ctx.env
    ctx.setenv('boot', env=base_bare)
    ctx.recurse(join("src", "boot"))

    ctx.setenv('kernel', env=base_bare)
    ctx.env.append_value('CFLAGS', ['-mcmodel=large'])
    ctx.env.append_value('CXXFLAGS', ['-mcmodel=large'])

    for mod_path in ctx.env.LIBRARIES:
        ctx.recurse(mod_path)

    for mod_path in ctx.env.DRIVERS:
        ctx.recurse(mod_path)

    ctx.recurse(join("src", "kernel"))

    ## Tools configuration
    ##
    from waflib.ConfigSet import ConfigSet
    ctx.setenv('tools', env=ConfigSet())
    ctx.load('clang++')
    ctx.find_program("mcopy", var="mcopy")
    ctx.find_program("dd", var="dd")
    common_configure(ctx)

    ctx.env.CXXFLAGS = ['-g', '-std=c++14', '-fno-rtti']
    ctx.env.LINKFLAGS = ['-g']

    for mod_path in ctx.env.LIBRARIES:
        ctx.recurse(mod_path)

    for mod_path in ctx.env.LIBRARIES:
        ctx.recurse(mod_path)

    ## Image configuration
    ##
    ctx.setenv('image', env=ConfigSet())
    ctx.find_program("mcopy", var="mcopy")
    ctx.find_program("dd", var="dd")
    common_configure(ctx)

    ## Testing configuration
    ##
    from waflib.ConfigSet import ConfigSet
    ctx.setenv('tests', env=ConfigSet())
    ctx.load('clang++')
    common_configure(ctx)

    ctx.env.CXXFLAGS = ['-g', '-std=c++14', '-fno-rtti']
    ctx.env.LINKFLAGS = ['-g']

    for mod_path in ctx.env.LIBRARIES:
        ctx.recurse(mod_path)
    ctx.recurse(join("src", "tests"))


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

class makerd(Task):
    color = 'YELLOW'
    def keyword(self):
        return "Creating"
    def __str__(self):
        node = self.outputs[0]
        return node.path_from(node.ctx.launch_node())
    def run(self):
        from os.path import join
        from subprocess import check_call as call

        args = [
            self.inputs[0].abspath(),
            self.inputs[1].abspath(),
            self.outputs[0].abspath(),
            ]
        call(args)


def build(bld):
    from os.path import join

    ## Boot
    #
    if bld.variant == 'boot':
        bld.recurse(join("src", "boot"))

    ## Tools
    #
    elif bld.variant == 'tools':
        for mod_path in bld.env.LIBRARIES:
            bld.recurse(mod_path)
        for mod_path in bld.env.TOOLS:
            bld.recurse(mod_path)

    ## Kernel
    #
    elif bld.variant == 'kernel':
        for mod_path in bld.env.LIBRARIES:
            bld.recurse(mod_path)
        for mod_path in bld.env.DRIVERS:
            bld.recurse(mod_path)

        bld.recurse(join("src", "kernel"))

    ## Image
    #
    elif bld.variant == 'image':
        src = bld.path
        root = bld.path.get_bld().parent

        out = {
            'boot': root.make_node('boot'),
            'tools': root.make_node('tools'),
            'kernel': root.make_node('kernel'),
        }

        kernel_name = bld.env.KERNEL_FILENAME

        disk = root.make_node("popcorn.img")
        disk1 = root.make_node("popcorn.fat")
        initrd = root.make_node("initrd.img")

        bld(
            source = src.make_node(join("assets", "ovmf", "x64", "OVMF.fd")),
            target = root.make_node("flash.img"),
            rule = "cp ${SRC} ${TGT}",
        )

        make_rd = makerd(env = bld.env)
        make_rd.set_inputs([
            out['tools'].make_node(join("src", "tools", "makerd", "makerd")),
            src.make_node(join("assets", "initrd.manifest")),
            ])
        make_rd.set_outputs([initrd])
        bld.add_to_group(make_rd)

        copy_img = mcopy(env = bld.env)
        copy_img.set_inputs([
            src.make_node(join("assets", "disk.fat")),
            out['boot'].make_node(join("src", "boot", "boot.efi")),
            out['kernel'].make_node(join("src", "kernel", kernel_name)),
            initrd,
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


def test(bld):
    from os.path import join
    for mod_path in bld.env.LIBRARIES:
        bld.recurse(mod_path)

    bld.recurse(join("src", "tests"))


def build_all(ctx):
    from waflib import Options
    Options.commands = ['tools', 'kernel', 'boot', 'image'] + Options.commands


from waflib.Build import BuildContext
class TestContext(BuildContext):
    cmd = 'test'
    variant = 'tests'

class ToolsContext(BuildContext):
    cmd = 'tools'
    variant = 'tools'

class KernelContext(BuildContext):
    cmd = 'kernel'
    variant = 'kernel'

class BootContext(BuildContext):
    cmd = 'boot'
    variant = 'boot'

class ImageContext(BuildContext):
    cmd = 'image'
    variant = 'image'

class BuildAllContext(BuildContext):
    cmd = 'build'
    fun = 'build_all'


class QemuContext(BuildContext):
    cmd = 'qemu'
    fun = 'qemu'

class DebugQemuContext(QemuContext):
    cmd = 'debug'
    fun = 'qemu'

def qemu(ctx):
    import subprocess
    subprocess.call("rm popcorn.log", shell=True)
    subprocess.call([
        'qemu-system-x86_64',
        '-drive', 'if=pflash,format=raw,file={}/flash.img'.format(out),
        '-drive', 'format=raw,file={}/popcorn.img'.format(out),
        ] + ctx.env.QEMUOPTS)

def debug(ctx):
    import subprocess
    subprocess.call("rm popcorn.log", shell=True)
    subprocess.call([
        'qemu-system-x86_64',
        '-drive', 'if=pflash,format=raw,file={}/flash.img'.format(out),
        '-drive', 'format=raw,file={}/popcorn.img'.format(out),
        '-s',
        ] + ctx.env.QEMUOPTS)


def vbox(ctx):
    import os
    from os.path import join
    from shutil import copy
    from subprocess import call

    vbox_dest = os.getenv("VBOX_DEST")

    ext = 'qed'
    dest = join(vbox_dest, "popcorn.{}".format(ext))
    call("qemu-img convert -f raw -O {} build/popcorn.img {}".format(ext, dest), shell=True)
    copy(join("build", "src", "kernel", "popcorn.elf"), vbox_dest);

def listen(ctx):
    from subprocess import call
    call("nc -l -p 5555", shell=True)


# vim: ft=python et
