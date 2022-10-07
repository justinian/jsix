import sys
from pathlib import Path
from definitions.context import Context

root = Path(sys.argv[0]).parent.parent
defs_path = (root / "definitions").resolve()
print(f"Definitions: {defs_path}")
ctx = Context(str(defs_path))
ctx.parse("syscalls.def")

