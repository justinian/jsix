class Action:
    _action_map = {
        '.c': Action_c,
        '.C': Action_cxx,
        '.cc': Action_cxx,
        '.cpp': Action_cxx,
        '.cxx': Action_cxx,
        '.c++': Action_cxx,
        '.s': Action_asm,
        '.S': Action_asm,
        '.cog': Action_cog,
    }

    @classmethod
    def find(cls, ext):
        return cls._action_map.get(ext)

class Action_c:
