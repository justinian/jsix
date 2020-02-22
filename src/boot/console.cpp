#include <stddef.h>
#include <stdint.h>

#include <uefi/types.h>
#include <uefi/boot_services.h>
#include <uefi/graphics.h>
#include <uefi/protos/graphics_output.h>
#include <uefi/protos/simple_text_output.h>

#include "console.h"
//#include "guids.h"
#include "utility.h"

namespace boot {

size_t ROWS = 0;
size_t COLS = 0;

console *console::s_console = nullptr;

static const wchar_t digits[] = {u'0', u'1', u'2', u'3', u'4', u'5',
	u'6', u'7', u'8', u'9', u'a', u'b', u'c', u'd', u'e', u'f'};

console::console(uefi::system_table *system_table) :
	m_rows(0),
	m_cols(0),
	m_current_status_line(0),
	m_out(nullptr)
{
	s_console = this;
	m_boot = system_table->boot_services;
	m_out = system_table->con_out;
}

uefi::status
console::initialize(const wchar_t *version)
{
	uefi::status status;

	// Might not find a video device at all, so ignore not found errors
	status = pick_mode();
	if (status != uefi::status::not_found)
		CHECK_EFI_STATUS_OR_FAIL(status);

	status = m_out->query_mode(m_out->mode->mode, &m_cols, &m_rows);
	CHECK_EFI_STATUS_OR_RETURN(status, "QueryMode");

	status = m_out->clear_screen();
	CHECK_EFI_STATUS_OR_RETURN(status, "ClearScreen");

	m_out->set_attribute(uefi::attribute::light_cyan);
	m_out->output_string((wchar_t *)L"jsix loader ");

	m_out->set_attribute(uefi::attribute::light_magenta);
	m_out->output_string((wchar_t *)version);

	m_out->set_attribute(uefi::attribute::light_gray);
	m_out->output_string((wchar_t *)L" booting...\r\n\n");

	return status;
}

uefi::status
console::pick_mode()
{
	uefi::status status;
	uefi::protos::graphics_output *gfx_out_proto;
	uefi::guid guid = uefi::protos::graphics_output::guid;
	status = m_boot->locate_protocol(&guid, nullptr, (void **)&gfx_out_proto);
	CHECK_EFI_STATUS_OR_RETURN(status, "LocateProtocol gfx");

	const uint32_t modes = gfx_out_proto->mode->max_mode;
	uint32_t best = gfx_out_proto->mode->mode;

	uefi::graphics_output_mode_info *info =
		(uefi::graphics_output_mode_info *)gfx_out_proto->mode;

	uint32_t res = info->horizontal_resolution * info->vertical_resolution;
	int is_fb = info->pixel_format != uefi::pixel_format::blt_only;

	for (uint32_t i = 0; i < modes; ++i) {
		size_t size = 0;
		status = gfx_out_proto->query_mode(i, &size, &info);
		CHECK_EFI_STATUS_OR_RETURN(status, "QueryMode");

#ifdef MAX_HRES
		if (info->horizontal_resolution > MAX_HRES) continue;
#endif

		const uint32_t new_res = info->horizontal_resolution * info->vertical_resolution;
		const int new_is_fb = info->pixel_format != uefi::pixel_format::blt_only;

		if (new_is_fb > is_fb && new_res >= res) {
			best = i;
			res = new_res;
		}
	}

	status = gfx_out_proto->set_mode(best);
	CHECK_EFI_STATUS_OR_RETURN(status, "SetMode %d/%d", best, modes);
	return uefi::status::success;
}

size_t
console::print_hex(uint32_t n) const
{
	wchar_t buffer[9];
	wchar_t *p = buffer;
	for (int i = 7; i >= 0; --i) {
		uint8_t nibble = (n & (0xf << (i*4))) >> (i*4);
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
		uint8_t nibble = (n & (0xf << (i*4))) >> (i*4);
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

void
console::status_begin(const wchar_t *message)
{
	m_current_status_line = m_out->mode->cursor_row;
	m_out->set_cursor_position(0, m_current_status_line);
	m_out->set_attribute(uefi::attribute::light_gray);
	m_out->output_string((wchar_t *)message);
	m_out->output_string((wchar_t *)L"\r\n");
}

void
console::status_ok() const
{
	int row = m_out->mode->cursor_row;
	int col = m_out->mode->cursor_column;

	m_out->set_cursor_position(50, m_current_status_line);

	m_out->set_attribute(uefi::attribute::light_gray);
	m_out->output_string((wchar_t *)L"[");
	m_out->set_attribute(uefi::attribute::green);
	m_out->output_string((wchar_t *)L"  ok  ");
	m_out->set_attribute(uefi::attribute::light_gray);
	m_out->output_string((wchar_t *)L"]\r\n");

	m_out->set_cursor_position(col, row);
}

void
console::status_fail(const wchar_t *error) const
{
	int row = m_out->mode->cursor_row;
	m_out->set_cursor_position(50, m_current_status_line);

	m_out->set_attribute(uefi::attribute::light_gray);
	m_out->output_string((wchar_t *)L"[");
	m_out->set_attribute(uefi::attribute::light_red);
	m_out->output_string((wchar_t *)L"failed");
	m_out->set_attribute(uefi::attribute::light_gray);
	m_out->output_string((wchar_t *)L"]");

	m_out->set_cursor_position(4, row);

	m_out->set_attribute(uefi::attribute::red);
	m_out->output_string((wchar_t *)error);
	m_out->set_attribute(uefi::attribute::light_gray);
	m_out->output_string((wchar_t *)L"\r\n");
}

void
console::status_warn(const wchar_t *error) const
{
	int row = m_out->mode->cursor_row;
	m_out->set_cursor_position(50, m_current_status_line);

	m_out->set_attribute(uefi::attribute::light_gray);
	m_out->output_string((wchar_t *)L"[");
	m_out->set_attribute(uefi::attribute::brown);
	m_out->output_string((wchar_t *)L" warn ");
	m_out->set_attribute(uefi::attribute::light_gray);
	m_out->output_string((wchar_t *)L"]");

	m_out->set_cursor_position(4, row);

	m_out->set_attribute(uefi::attribute::yellow);
	m_out->output_string((wchar_t *)error);
	m_out->set_attribute(uefi::attribute::light_gray);
	m_out->output_string((wchar_t *)L"\r\n");
}

/*
uefi::status
con_get_framebuffer(
	EFI_BOOT_SERVICES *bootsvc,
	void **buffer,
	size_t *buffer_size,
	uint32_t *hres,
	uint32_t *vres,
	uint32_t *rmask,
	uint32_t *gmask,
	uint32_t *bmask)
{
	uefi::status status;

	uefi::protos::graphics_output *gop;
	status = bootsvc->LocateProtocol(&guid_gfx_out, NULL, (void **)&gop);
	if (status != EFI_NOT_FOUND) {
		CHECK_EFI_STATUS_OR_RETURN(status, "LocateProtocol gfx");

		*buffer = (void *)gop->Mode->FrameBufferBase;
		*buffer_size = gop->Mode->FrameBufferSize;
		*hres = gop->Mode->Info->horizontal_resolution;
		*vres = gop->Mode->Info->vertical_resolution;

		switch (gop->Mode->Info->PixelFormat) {
			case PixelRedGreenBlueReserved8BitPerColor:
				*rmask = 0x0000ff;
				*gmask = 0x00ff00;
				*bmask = 0xff0000;
				return EFI_SUCCESS;

			case PixelBlueGreenRedReserved8BitPerColor:
				*bmask = 0x0000ff;
				*gmask = 0x00ff00;
				*rmask = 0xff0000;
				return EFI_SUCCESS;

			case PixelBitMask:
				*rmask = gop->Mode->Info->PixelInformation.RedMask;
				*gmask = gop->Mode->Info->PixelInformation.GreenMask;
				*bmask = gop->Mode->Info->PixelInformation.BlueMask;
				return EFI_SUCCESS;

			default:
				// Not a framebuffer, fall through to zeroing out
				// values below.
				break;
		}
	}

	*buffer = NULL;
	*buffer_size = *hres = *vres = 0;
	*rmask = *gmask = *bmask = 0;
	return EFI_SUCCESS;
}
*/

} // namespace boot
