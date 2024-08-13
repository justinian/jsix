import cog

supported_architectures = {
    "amd64": "__amd64__",
}

def arch_includes(header, root=""):
    from pathlib import Path
    root = Path(root)
    header = Path(header)

    prefix = "if"
    for arch, define in supported_architectures.items():
        path = root / "arch" / arch / header
        cog.outl(f"#{prefix} defined({define})")
        cog.outl(f"#include <{path}>")
        prefix = "elif"
    cog.outl("#else")
    cog.outl('#error "Unsupported platform"')
    cog.outl("#endif")

