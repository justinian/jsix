from os import PathLike
from collections import namedtuple
Var = namedtuple("Var", ("name", "section", "type"))


def load_sysconf(path: PathLike) -> tuple[int, tuple[Var]]:
    from tomllib import load

    with open(path, 'rb') as infile:
        data = load(infile)
        address = data["address"]

        vars = []
        for s in data['sections']:
            for v in data[s]['var']:
                vars.append(Var(v["name"], s, v["type"]))

        return address, tuple(vars)
