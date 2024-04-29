#include <edit/line.h>
#include <util/format.h>
#include "commands.h"

static void
help(edit::source &s)
{
    for (unsigned i = 0; i < g_builtins_count; ++i) {
        char line[100];
        util::counted<char> output { line, sizeof(line) };

        builtin &cmd = g_builtins[i];

        size_t len = util::format(output, "%20s - %s\r\n", cmd.name(), cmd.description());
        s.write(line, len);
    }
}

static void
version(edit::source &s)
{
    static const char *gv = GIT_VERSION;

    char line[100];
    util::counted<char> output { line, sizeof(line) };

    size_t len = util::format(output, "jsix version %s\r\n", gv);
    s.write(line, len);
}

builtin g_builtins[] = {
    { "help",       "list availble commands",               help },
    { "version",    "print current jsix version",           version },
};
size_t g_builtins_count = sizeof(g_builtins) / sizeof(g_builtins[0]);
