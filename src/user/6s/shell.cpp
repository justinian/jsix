#include <string.h>
#include <j6/errors.h>
#include <j6/syslog.hh>
#include "commands.h"
#include "shell.h"

void
shell::add_filesystem(j6_handle_t mb)
{
    char tag_buf [100];
    size_t size = sizeof(tag_buf);
    j6::proto::vfs::client c {mb};
    j6_status_t s = c.get_tag(tag_buf, size);
    if (s != j6_status_ok)
        return;

    char *name = new char [size + 1];
    memcpy(name, tag_buf, size);
    name[size] = 0;

    m_fs.push_back({name, c});
    if (!m_cfs)
        m_cfs = name;
}

void
shell::handle_command(const char *command, size_t len)
{
    j6::syslog(j6::logs::app, j6::log_level::info, "Command: %s", command);
    if (len == 0)
        return;

    for (unsigned i = 0; i < g_builtins_count; ++i) {
        builtin &cmd = g_builtins[i];
        if (strncmp(command, cmd.name(), len) == 0) {
            cmd.run(*this);
            return;
        }
    }

    static const char unknown[] = "\x1b[1;33mUnknown command.\x1b[0m\r\n";
    write(unknown, sizeof(unknown)-1);
}
