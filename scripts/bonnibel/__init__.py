from os.path import join

class BonnibelError(Exception):
    def __init__(self, message):
        self.message = message

def load_config(filename):
    from yaml import safe_load
    with open(filename, 'r') as infile:
        return safe_load(infile.read())

def mod_rel(path):
    return join("${module_dir}", path)

def target_rel(path, module=None):
    if module:
        return join("${target_dir}", module + ".dir", path)
    else:
        return join("${target_dir}", path)

def include_rel(path, module=None):
    if module:
        return join("${build_root}", "include", module, path)
    else:
        return join("${build_root}", "include", path)
