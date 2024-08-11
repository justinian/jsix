import cog

int_widths = (8, 16, 32, 64)
int_mods = ("fast", "least")

int_types = {
    # type: abbrev
    "char": "char",
    "short": "short",
    "int": "int",
    "long": "long",
    "long long": "llong",
}

def definition(kind, name, val, width=24):
    cog.outl(f"{kind} {name:{width}} {val}")


atomic_types = {
    "_Bool": "bool",
    "signed char": "schar",
    "char16_t": "char16_t",
    "char32_t": "char32_t",
    "wchar_t": "wchar_t",
    "wchar_t": "wchar_t",
    "size_t": "size_t",
    "ptrdiff_t": "ptrdiff_t",
    "intptr_t": "intptr_t",
    "uintptr_t": "uintptr_t",
    "intmax_t": "intmax_t",
    "uintmax_t": "uintmax_t",
    }

for name, abbrev in int_types.items():
    atomic_types.update({name: abbrev, f"unsigned {name}": f"u{abbrev}"})

for width in int_widths:
    atomic_types.update({t: t for t in (
            f"int_least{width}_t",
            f"uint_least{width}_t",
            f"int_fast{width}_t",
            f"uint_fast{width}_t")})

