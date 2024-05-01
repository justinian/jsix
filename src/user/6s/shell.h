#pragma once

#include <vector>
#include <j6/protocols/vfs.hh>
#include <edit/line.h>

/// Shell state
class shell
{
public:
    struct fs {
        const char *tag;
        j6::proto::vfs::client client;
    };
    using fs_list = std::vector<fs>;

    shell(edit::source &source) : m_source {source}, m_cwd {"/"}, m_cfs {nullptr} {}

    const char * cwd() const { return m_cwd; }
    const char * cfs() const { return m_cfs; }

    // Transparently use source read/write functions
    inline void consume(size_t len) { m_source.consume(len); }
    inline void write(char const *data, size_t len) { m_source.write(data, len); }
    inline size_t read(char const **data) { return m_source.read(data); }

    void add_filesystem(j6_handle_t mb);
    inline const fs_list & filesystems() const { return m_fs; }

    void handle_command(const char *command, size_t len);

private:
    edit::source &m_source;

    char m_cwd [256];
    char const *m_cfs;
    fs_list m_fs;
};
