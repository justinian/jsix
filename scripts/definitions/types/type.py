class Type:
    def __init__(self, name):
        self.__name = name

    name = property(lambda self: self.__name)
    reference = property(lambda self: False)

    def c_names(self, options):
        raise NotImplemented("Call to base Type.c_names")

    def cxx_names(self, options):
        raise NotImplemented("Call to base Type.c_names")

