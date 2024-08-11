import cog

supported_architectures = {
    "amd64": "__amd64__",
}

def arch_includes(header, root=""):
    prefix = "if"
    for arch, define in supported_architectures.items():
        cog.outl(f"#{prefix} defined({define})")
        cog.outl(f"#include <{root}arch/{arch}/{header}>")
        prefix = "elif"
    cog.outl("#else")
    cog.outl('#error "Unsupported platform"')
    cog.outl("#endif")

