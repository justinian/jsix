class BonnibelError(Exception):
    def __init__(self, message):
        self.message = message

def load_config(filename):
    from yaml import safe_load
    with open(filename, 'r') as infile:
        return safe_load(infile.read())
