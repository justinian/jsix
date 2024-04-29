#pragma once

#include <stddef.h>

namespace edit {
    class source;
}


class builtin
{
public:
    using runf = void (*)(edit::source &);

    builtin(const char *name, const char *desc, runf func) :
        m_name {name}, m_desc {desc}, m_func {func} {}

    const char * name() const { return m_name; }
    const char * description() const { return m_desc; }

    void run(edit::source &s) { m_func(s); }

private:
    const char *m_name;
    const char *m_desc;
    runf m_func;
};


extern builtin g_builtins[];
extern size_t  g_builtins_count;
