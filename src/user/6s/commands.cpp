#include <edit/line.h>
#include <util/format.h>
#include "commands.h"
#include "shell.h"

static void
fslist(shell &s)
{
    char line[100];
    util::counted<char> output { line, sizeof(line) };

    for (auto &f : s.filesystems()) {
        size_t len = util::format(output, "%s\r\n", f.tag);
        s.write(line, len);
    }
}

static void
help(shell &s)
{
    char line[100];
    util::counted<char> output { line, sizeof(line) };

    for (unsigned i = 0; i < g_builtins_count; ++i) {
        builtin &cmd = g_builtins[i];
        size_t len = util::format(output, "%20s - %s\r\n", cmd.name(), cmd.description());
        s.write(line, len);
    }
}

static void
pwd(shell &s)
{
    char line[100];
    util::counted<char> output { line, sizeof(line) };

    size_t len = util::format(output, "%s:%s\r\n", s.cfs(), s.cwd());
    s.write(line, len);
}

static void
version(shell &s)
{
    static const char *gv = GIT_VERSION;

    char line[100];
    util::counted<char> output { line, sizeof(line) };

    size_t len = util::format(output, "jsix version %s\r\n", gv);
    s.write(line, len);
}

builtin g_builtins[] = {
    { "fslist",     "list available filesystems",           fslist },
    { "help",       "list available commands",              help },
    { "pwd",        "print current working directory",      pwd },
    { "version",    "print current jsix version",           version },
};
size_t g_builtins_count = sizeof(g_builtins) / sizeof(g_builtins[0]);
