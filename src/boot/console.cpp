#include <stddef.h>
#include <stdint.h>

#include <uefi/boot_services.h>
#include <uefi/graphics.h>
#include <uefi/protos/graphics_output.h>
#include <uefi/protos/simple_text_output.h>
#include <uefi/types.h>

#include "console.h"
#include "error.h"

#ifndef GIT_VERSION_WIDE
#define GIT_VERSION_WIDE L"no version"
#endif

namespace boot {

size_t ROWS = 0;
size_t COLS = 0;

console *console::s_console = nullptr;

static const wchar_t digits[] = {u'0', u'1', u'2', u'3', u'4', u'5',
	u'6', u'7', u'8', u'9', u'a', u'b', u'c', u'd', u'e', u'f'};


static size_t
wstrlen(const wchar_t *s)
{
	size_t count = 0;
	while (s && *s++) count++;
	return count;
}


console::console(uefi::boot_services *bs, uefi::protos::simple_text_output *out) :
	m_rows {0},
	m_cols {0},
	m_out {out},
	m_fb {0, 0}
{
	pick_mode(bs);

	try_or_raise(
		m_out->query_mode(m_out->mode->mode, &m_cols, &m_rows),
		L"Failed to get text output mode.");

	try_or_raise(
		m_out->clear_screen(),
		L"Failed to clear screen");

	m_out->set_attribute(uefi::attribute::light_cyan);
	m_out->output_string(L"jsix loader ");

	m_out->set_attribute(uefi::attribute::light_magenta);
	m_out->output_string(GIT_VERSION_WIDE);

	m_out->set_attribute(uefi::attribute::light_gray);
	m_out->output_string(L" booting...\r\n");

	if (m_fb.type != kernel::init::fb_type::none) {
		wchar_t const * type = nullptr;
		switch (m_fb.type) {
		case kernel::init::fb_type::rgb8:
			type = L"rgb8";
			break;
		case kernel::init::fb_type::bgr8:
			type = L"bgr8";
			break;
		default:
			type = L"unknown";
		}

		printf(L"Found framebuffer: %dx%d[%d] type %s @0x%x\r\n",
			m_fb.horizontal, m_fb.vertical, m_fb.scanline, type, m_fb.phys_addr);
	} else {
		printf(L"No framebuffer found.\r\n");
	}

	s_console = this;
}

void
console::pick_mode(uefi::boot_services *bs)
{
	uefi::status status;
	uefi::protos::graphics_output *gfx_out_proto;
	uefi::guid guid = uefi::protos::graphics_output::guid;

	m_fb.type = kernel::init::fb_type::none;

	uefi::status has_gop = bs->locate_protocol(&guid, nullptr,
		(void **)&gfx_out_proto);

	if (has_gop != uefi::status::success)
		// No video output found, skip it
		return;

	const uint32_t modes = gfx_out_proto->mode->max_mode;
	uint32_t best = gfx_out_proto->mode->mode;

	uefi::graphics_output_mode_info *info =
		(uefi::graphics_output_mode_info *)gfx_out_proto->mode;

	uint32_t res = info->horizontal_resolution * info->vertical_resolution;
	int pixmode = static_cast<int>(info->pixel_format);

	for (uint32_t i = 0; i < modes; ++i) {
		size_t size = 0;

		try_or_raise(
			gfx_out_proto->query_mode(i, &size, &info),
			L"Failed to find a graphics mode the driver claimed to support");

#ifdef MAX_HRES
		if (info->horizontal_resolution > MAX_HRES) continue;
#endif

		const uint32_t new_res = info->horizontal_resolution * info->vertical_resolution;
		int new_pixmode = static_cast<int>(info->pixel_format);

		if (new_pixmode <= pixmode && new_res >= res) {
			best = i;
			res = new_res;
			pixmode = new_pixmode;
		}
	}

	try_or_raise(
		gfx_out_proto->set_mode(best),
		L"Failed to set graphics mode");

	if (pixmode <= static_cast<int>(uefi::pixel_format::bgr8)) {
		m_fb.phys_addr = gfx_out_proto->mode->frame_buffer_base;
		m_fb.size = gfx_out_proto->mode->frame_buffer_size;
		m_fb.vertical = gfx_out_proto->mode->info->vertical_resolution;
		m_fb.horizontal = gfx_out_proto->mode->info->horizontal_resolution;
		m_fb.scanline = gfx_out_proto->mode->info->pixels_per_scanline;

		switch (gfx_out_proto->mode->info->pixel_format) {
		case uefi::pixel_format::rgb8:
			m_fb.type = kernel::init::fb_type::rgb8;
			break;
		case uefi::pixel_format::bgr8:
			m_fb.type = kernel::init::fb_type::bgr8;
			break;
		default:
			m_fb.type = kernel::init::fb_type::none;
		}
	}
}

size_t
console::print_hex(uint32_t n) const
{
	wchar_t buffer[9];
	wchar_t *p = buffer;
	for (int i = 7; i >= 0; --i) {
		uint8_t nibble = (n >> (i*4)) & 0xf;
		*p++ = digits[nibble];
	}
	*p = 0;
	m_out->output_string(buffer);
	return 8;
}

size_t
console::print_long_hex(uint64_t n) const
{
	wchar_t buffer[17];
	wchar_t *p = buffer;
	for (int i = 15; i >= 0; --i) {
		uint8_t nibble = (n >> (i*4)) & 0xf;
		*p++ = digits[nibble];
	}
	*p = 0;
	m_out->output_string(buffer);
	return 16;
}

size_t
console::print_dec(uint32_t n) const
{
	wchar_t buffer[11];
	wchar_t *p = buffer + 10;
	*p-- = 0;
	do {
		*p-- = digits[n % 10];
		n /= 10;
	} while (n != 0);

	m_out->output_string(++p);
	return 10 - (p - buffer);
}

size_t
console::print_long_dec(uint64_t n) const
{
	wchar_t buffer[21];
	wchar_t *p = buffer + 20;
	*p-- = 0;
	do {
		*p-- = digits[n % 10];
		n /= 10;
	} while (n != 0);

	m_out->output_string(++p);
	return 20 - (p - buffer);
}

size_t
console::vprintf(const wchar_t *fmt, va_list args) const
{
	wchar_t buffer[256];
	const wchar_t *r = fmt;
	wchar_t *w = buffer;
	size_t count = 0;

	while (r && *r) {
		if (*r != L'%') {
			count++;
			*w++ = *r++;
			continue;
		}

		*w = 0;
		m_out->output_string(buffer);
		w = buffer;

		r++; // chomp the %

		switch (*r++) {
			case L'%':
				m_out->output_string(const_cast<wchar_t*>(L"%"));
				count++;
				break;

			case L'x':
				count += print_hex(va_arg(args, uint32_t));
				break;

			case L'd':
			case L'u':
				count += print_dec(va_arg(args, uint32_t));
				break;

			case L's':
				{
					wchar_t *s = va_arg(args, wchar_t*);
					count += wstrlen(s);
					m_out->output_string(s);
				}
				break;

			case L'l':
				switch (*r++) {
					case L'x':
						count += print_long_hex(va_arg(args, uint64_t));
						break;

					case L'd':
					case L'u':
						count += print_long_dec(va_arg(args, uint64_t));
						break;

					default:
						break;
				}
				break;

			default:
				break;
		}
	}

	*w = 0;
	m_out->output_string(buffer);
	return count;
}

size_t
console::printf(const wchar_t *fmt, ...) const
{
	va_list args;
	va_start(args, fmt);

	size_t result = vprintf(fmt, args);

	va_end(args);
	return result;
}

size_t
console::print(const wchar_t *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	size_t result = get().vprintf(fmt, args);

	va_end(args);
	return result;
}

} // namespace boot
