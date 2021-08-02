#pragma once
/// \file scrollback.h

class screen;
class font;

class scrollback
{
public:
    scrollback(unsigned lines, unsigned cols, unsigned margin = 2);

    void add_line(const char *line, size_t len);

    char * get_line(unsigned i);

    void render(screen &scr, font &fnt);

private:
    char *m_data;
    unsigned m_rows, m_cols;
    unsigned m_start;
    unsigned m_count;
    unsigned m_margin;
};

