#pragma once
/// \file line.h
/// Declaration of line-based editing class

#include <stddef.h>
#include <util/api.h>

namespace edit {

class API source
{
public:
    virtual size_t read(char const **data) = 0;
    virtual void consume(size_t size) = 0;
    virtual void write(char const *data, size_t len) = 0;
};


class API line
{
public:
    line(source &s);
    ~line();

    /// Get a finished line from the editor.
    size_t read(char *buffer, size_t size, char const *prompt = nullptr, size_t prompt_len = 0);

private:
    size_t parse_command(char const *data, size_t len);

    source &m_source;
};

} // namespace edit
