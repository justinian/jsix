#include <stdarg.h>
#include <util/format.h>

namespace util {

namespace {

const char digits[] = "0123456789abcdef";

template <typename I, unsigned N> void
append_int(char *&out, size_t &count, size_t max, I value, unsigned min_width, char pad)
{
    static constexpr size_t bufsize = sizeof(I)*3;

    unsigned num_digits = 0;
    char buffer[bufsize];
    char *p = buffer + (bufsize - 1);
    do {
        if (value) {
            *p-- = digits[value % N];
            value /= N;
        } else {
            *p-- = num_digits ? pad : '0';
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

void
append_string(char *&out, size_t &count, size_t max, char const *value)
{
    while (value && *value && count < max) {
        count++;
        *out++ = *value++;
    }
}

} // namespace

size_t
vformat(stringbuf output, char const *format, va_list va)
{
    char * out = output.pointer;
    const size_t max = output.count - 1;

    size_t count = 0;
    while (format && *format && count < max) {
        if (*format != '%') {
            count++;
            *out++ = *format++;
            continue;
        }

        format++; // chomp the % character
        char spec = *format++;

        bool long_type = false;
        if (spec == '%') {
            count++;
            *out++ = '%';
            continue;
        }

        unsigned width = 0;
        char pad = ' ';

        while (spec) {
            bool done = false;

            if (spec >= '0' && spec <= '9') {
                if (spec == '0' && width == 0)
                    pad = '0';
                else
                    width = width * 10 + (spec - '0');

                spec = *format++;
                continue;
            }

            switch (spec) {
                case 'l':
                    long_type = true;
                    break;

                case 'x':
                    if (long_type)
                        append_int<uint64_t, 16>(out, count, max, va_arg(va, uint64_t), width, pad);
                    else
                        append_int<uint32_t, 16>(out, count, max, va_arg(va, uint32_t), width, pad);
                    done = true;
                    break;

                case 'd':
                case 'u':
                    if (long_type)
                        append_int<uint64_t, 10>(out, count, max, va_arg(va, uint64_t), width, pad);
                    else
                        append_int<uint32_t, 10>(out, count, max, va_arg(va, uint32_t), width, pad);
                    done = true;
                    break;

                case 's':
                    append_string(out, count, max, va_arg(va, const char *));
                    done = true;
                    break;
            }

            if (done) break;
            spec = *format++;
        }
    }

    return count;
}

size_t
format(stringbuf output, const char *format, ...)
{
    va_list va;
    va_start(va, format);
    size_t result = vformat(output, format, va);
    va_end(va);
    return result;
}

} //namespace util
