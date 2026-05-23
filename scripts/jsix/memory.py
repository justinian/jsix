from os import PathLike
from collections import namedtuple

Region = namedtuple("Region", ("name", "desc", "start", "size", "shared"))

_UNITS = ['B', 'KiB', 'MiB', 'GiB', 'TiB', 'PiB']
_MULTS = {_UNITS[i]: 1024**i for i in range(len(_UNITS))}


def parse_size(size: str | int) -> int:
    """Parse a size value into a number of bytes.

    The argument can be either an integer (in which case it is
    just returned directly), or a string containing a number and
    an optional SI binary unit (B, KiB, MiB, etc) separated by
    whitespace.

    Integers are supported in any integer literal format supported
    by Python. Floating point numbers are supported in decimal only,
    and the resulting value will be truncated to an integer number
    of bytes.
    """
    if isinstance(size, int): return size

    mult = 1
    parts = size.split(maxsplit=1)
    if len(parts) > 1:
        try:
            mult = _MULTS[parts[1]]
        except KeyError:
            raise RuntimeError(f"No size unit named {parts[1]}")

    try:
        return int(float(parts[0]) * mult)
    except ValueError:
        return int(parts[0], base=0) * mult


def to_units(size: int) -> str:
    """Convert an integer size into a number of units."""

    for unit in reversed(_UNITS):
        m = _MULTS[unit]
        if size < m: continue
        return f"{size / m:.3g} {unit}"

    return f"{size} B"


def layout(path: PathLike) -> tuple[Region]:
    from tomllib import load

    regions = []
    addr = 1 << 64  # Start of upper half

    with open(path, 'rb') as infile:
        data = load(infile)
        for r in data['region']:
            size = parse_size(r["size"])
            addr -= size
            regions.append(Region(r["name"], r["desc"], addr, size,
                r.get("shared", False)))

    return tuple(regions)
