#include <stdarg.h>
#include <util/format.h>

namespace util {

namespace {

template <typename char_t> struct char_traits;

template <>
struct char_traits<char>
{
    static constexpr const char digits[] = "0123456789abcdef";

    static constexpr char d0 = '0';
    static constexpr char d9 = '9';

    static constexpr char per = '%';
    static constexpr char space = ' ';

    static constexpr char d = 'd';
    static constexpr char l = 'l';
    static constexpr char s = 's';
    static constexpr char u = 'u';
    static constexpr char x = 'x';
};

template <>
struct char_traits<wchar_t>
{
    static constexpr const wchar_t digits[] = L"0123456789abcdef";

    static constexpr wchar_t d0 = L'0';
    static constexpr wchar_t d9 = L'9';

    static constexpr wchar_t per = L'%';
    static constexpr wchar_t space = L' ';

    static constexpr wchar_t d = L'd';
    static constexpr wchar_t l = L'l';
    static constexpr wchar_t s = L's';
    static constexpr wchar_t u = L'u';
    static constexpr wchar_t x = L'x';
};


template <typename char_t, typename I, unsigned N> void
append_int(char_t *&out, size_t &count, size_t max, I value, unsigned min_width, char_t pad)
{
    using chars = char_traits<char_t>;
    static constexpr size_t bufsize = sizeof(I)*4;

    unsigned num_digits = 0;
    char_t buffer[bufsize];
    char_t *p = buffer + (bufsize - 1);
    do {
        if (value) {
            *p-- = chars::digits[value % N];
            value /= N;
        } else {
            *p-- = num_digits ? pad : chars::d0;
        }
        num_digits++;
    } while (value || num_digits < min_width);

    ++p;
    for (unsigned i = 0; i < num_digits; ++i) {
        *out++ = p[i];
        if (++count == max)
            return;
    }
}

template <typename char_t> void
append_string(char_t *&out, size_t &count, size_t max, unsigned width, char_t const *value)
{
    using chars = char_traits<char_t>;

    while (value && *value && count < max) {
        ++count;
        if (width) --width;
        *out++ = *value++;
    }

    while (width-- && count < max) {
        ++count;
        *out++ = chars::space;
    }
}


template <typename char_t> size_t
vformat(counted<char_t> output, char_t const *format, va_list va)
{
    using chars = char_traits<char_t>;

    char_t * out = output.pointer;
    const size_t max = output.count - 1;

    size_t count = 0;
    while (format && *format && count < max) {
        if (*format != chars::per) {
            count++;
            *out++ = *format++;
            continue;
        }

        format++; // chomp the % character
        char_t spec = *format++;

        bool long_type = false;
        if (spec == chars::per) {
            count++;
            *out++ = chars::per;
            continue;
        }

        unsigned width = 0;
        char_t pad = chars::space;

        while (spec) {
            bool done = false;

            if (spec >= chars::d0 && spec <= chars::d9) {
                if (spec == chars::d0 && width == 0)
                    pad = chars::d0;
                else
                    width = width * 10 + (spec - chars::d0);

                spec = *format++;
                continue;
            }

            switch (spec) {
                case chars::l:
                    long_type = true;
                    break;

                case chars::x:
                    if (long_type)
                        append_int<char_t, uint64_t, 16>(out, count, max, va_arg(va, uint64_t), width, pad);
                    else
                        append_int<char_t, uint32_t, 16>(out, count, max, va_arg(va, uint32_t), width, pad);
                    done = true;
                    break;

                case chars::d:
                case chars::u:
                    if (long_type)
                        append_int<char_t, uint64_t, 10>(out, count, max, va_arg(va, uint64_t), width, pad);
                    else
                        append_int<char_t, uint32_t, 10>(out, count, max, va_arg(va, uint32_t), width, pad);
                    done = true;
                    break;

                case chars::s:
                    append_string(out, count, max, width, va_arg(va, const char_t *));
                    done = true;
                    break;
            }

            if (done) break;
            spec = *format++;
        }
    }

    *out = 0;
    return count;
}

} // namespace

size_t API format(counted<char> output, const char *format, ...)
{
    va_list va;
    va_start(va, format);
    size_t result = vformat<char>(output, format, va);
    va_end(va);
    return result;
}

size_t API format(counted<wchar_t> output, const wchar_t *format, ...)
{
    va_list va;
    va_start(va, format);
    size_t result = vformat<wchar_t>(output, format, va);
    va_end(va);
    return result;
}

size_t API vformat(counted<char> output, const char *format, va_list va) { return vformat<char>(output, format, va); }
size_t API vformat(counted<wchar_t> output, const wchar_t *format, va_list va) { return vformat<wchar_t>(output, format, va); }


} //namespace util
