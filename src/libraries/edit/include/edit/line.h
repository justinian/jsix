#pragma once
/// \file line.h
/// Declaration of line-based editing class

#include <stddef.h>
#include <util/api.h>

namespace edit {

class API line
{
public:
    line(const char *prompt = nullptr, size_t propmt_len = 0);
    ~line();

    /// Feed input from the terminal keyboard in.
    /// \arg data  Data from the keyboard
    /// \arg len   Length of data in bytes
    /// \retruns   Number of bytes consumed
    size_t input(char const *data, size_t len);

    /// Get output to be sent to the terminal screen.
    /// \arg data  [out] Data to be written to the screen
    /// \returns   Number of bytes from data to be written
    size_t output(char const **data);

private:
    size_t parse_command(char const *data, size_t len);

    char *m_buf;

    size_t m_in;
    size_t m_out;
    size_t m_prefix;

    char const *m_tmp_out;
    size_t m_tmp_out_len;
};

} // namespace edit
