class Target(dict):
    __targets = {}

    @classmethod
    def load(cls, root, name, config=None):
        from . import load_config

        if (name, config) in cls.__targets:
            return cls.__targets[(name, config)]

        configs = root / "assets/build"

        dicts = []
        depfiles = []
        basename = name
        if config:
            basename += f"-{config}"

        while basename is not None:
            filename = str(configs / (basename + ".yaml"))
            depfiles.append(filename)
            desc = load_config(filename)
            basename = desc.get("extends")
            dicts.append(desc.get("variables", dict()))

        t = Target(name, config, depfiles)
        for d in reversed(dicts):
            for k, v in d.items():
                if isinstance(v, (list, tuple)):
                    t[k] = t.get(k, list()) + list(v)
                elif isinstance(v, dict):
                    t[k] = t.get(k, dict())
                    t[k].update(v)
                else:
                    t[k] = v

        cls.__targets[(name, config)] = t
        return t

    def __init__(self, name, config, depfiles):
        self.__name = name
        self.__config = config
        self.__depfiles = tuple(depfiles)

    def __hash__(self):
        return hash((self.__name, self.__config))

    name = property(lambda self: self.__name)
    config = property(lambda self: self.__config)
    depfiles = property(lambda self: self.__depfiles)
