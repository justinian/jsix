class Sysconf:
    from collections import namedtuple
    Var = namedtuple("Var", ("name", "section", "type"))

    def __init__(self, path):
        from yaml import safe_load

        sys_vars = []

        with open(path, 'r') as infile:
            data = safe_load(infile.read())
            self.address = data["address"]

            for v in data["vars"]:
                sys_vars.append(Sysconf.Var(v["name"], v["section"], v["type"]))

        self.vars = tuple(sys_vars)
