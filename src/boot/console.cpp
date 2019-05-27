#include <stddef.h>
#include <stdint.h>

#include "console.h"
#include "guids.h"
#include "utility.h"

size_t ROWS = 0;
size_t COLS = 0;

static EFI_SIMPLE_TEXT_OUT_PROTOCOL *m_out = 0;

static const wchar_t digits[] = {u'0', u'1', u'2', u'3', u'4', u'5',
	u'6', u'7', u'8', u'9', u'a', u'b', u'c', u'd', u'e', u'f'};

console::console(EFI_SYSTEM_TABLE *system_table) :
	m_rows(0),
	m_cols(0),
	m_out(nullptr)
{
	s_console = this;
	m_boot = system_table->BootServices;
	m_out = system_table->ConOut;
}

EFI_STATUS
console::initialize(const wchar_t *version)
{
	EFI_STATUS status;

	// Might not find a video device at all, so ignore not found errors
	status = pick_mode();
	if (status != EFI_NOT_FOUND)
		CHECK_EFI_STATUS_OR_FAIL(status);

	status = m_out->QueryMode(m_out, m_out->Mode->Mode, &m_cols, &m_rows);
	CHECK_EFI_STATUS_OR_RETURN(status, "QueryMode");

	status = m_out->ClearScreen(m_out);
	CHECK_EFI_STATUS_OR_RETURN(status, "ClearScreen");

	m_out->SetAttribute(m_out, EFI_LIGHTCYAN);
	m_out->OutputString(m_out, (wchar_t *)L"jsix loader ");

	m_out->SetAttribute(m_out, EFI_LIGHTMAGENTA);
	m_out->OutputString(m_out, (wchar_t *)version);

	m_out->SetAttribute(m_out, EFI_LIGHTGRAY);
	m_out->OutputString(m_out, (wchar_t *)L" booting...\r\n\n");

	return status;
}

EFI_STATUS
console::pick_mode()
{
	EFI_STATUS status;
	EFI_GRAPHICS_OUTPUT_PROTOCOL *gfx_out_proto;
	status = m_boot->LocateProtocol(&guid_gfx_out, NULL, (void **)&gfx_out_proto);
	CHECK_EFI_STATUS_OR_RETURN(status, "LocateProtocol gfx");

	const uint32_t modes = gfx_out_proto->Mode->MaxMode;
	uint32_t best = gfx_out_proto->Mode->Mode;

	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info =
		(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *)gfx_out_proto->Mode;

	uint32_t res = info->HorizontalResolution * info->VerticalResolution;
	int is_fb = info->PixelFormat != PixelBltOnly;

	for (uint32_t i = 0; i < modes; ++i) {
		size_t size = 0;
		status = gfx_out_proto->QueryMode(gfx_out_proto, i, &size, &info);
		CHECK_EFI_STATUS_OR_RETURN(status, "QueryMode");

#ifdef MAX_HRES
		if (info->HorizontalResolution > MAX_HRES) continue;
#endif

		const uint32_t new_res = info->HorizontalResolution * info->VerticalResolution;
		const int new_is_fb = info->PixelFormat == PixelBltOnly;

		if (new_is_fb > is_fb && new_res >= res) {
			best = i;
			res = new_res;
		}
	}

	status = gfx_out_proto->SetMode(gfx_out_proto, best);
	CHECK_EFI_STATUS_OR_RETURN(status, "SetMode %d/%d", best, modes);
	return EFI_SUCCESS;
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
	m_out->OutputString(m_out, buffer);
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
	m_out->OutputString(m_out, buffer);
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

	m_out->OutputString(m_out, ++p);
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

	m_out->OutputString(m_out, ++p);
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
		m_out->OutputString(m_out, buffer);
		w = buffer;

		r++; // chomp the %

		switch (*r++) {
			case L'%':
				m_out->OutputString(m_out, const_cast<wchar_t*>(L"%"));
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
					m_out->OutputString(m_out, s);
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
	m_out->OutputString(m_out, buffer);
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
console::status_begin(const wchar_t *message) const
{
	m_out->SetAttribute(m_out, EFI_LIGHTGRAY);
	m_out->OutputString(m_out, (wchar_t *)message);
}

void
console::status_ok() const
{
	m_out->SetAttribute(m_out, EFI_LIGHTGRAY);
	m_out->OutputString(m_out, (wchar_t *)L"[");
	m_out->SetAttribute(m_out, EFI_GREEN);
	m_out->OutputString(m_out, (wchar_t *)L"  ok  ");
	m_out->SetAttribute(m_out, EFI_LIGHTGRAY);
	m_out->OutputString(m_out, (wchar_t *)L"]\r\n");
}

void
console::status_fail(const wchar_t *error) const
{
	m_out->SetAttribute(m_out, EFI_LIGHTGRAY);
	m_out->OutputString(m_out, (wchar_t *)L"[");
	m_out->SetAttribute(m_out, EFI_LIGHTRED);
	m_out->OutputString(m_out, (wchar_t *)L"failed");
	m_out->SetAttribute(m_out, EFI_LIGHTGRAY);
	m_out->OutputString(m_out, (wchar_t *)L"]\r\n");

	m_out->SetAttribute(m_out, EFI_RED);
	m_out->OutputString(m_out, (wchar_t *)error);
	m_out->SetAttribute(m_out, EFI_LIGHTGRAY);
	m_out->OutputString(m_out, (wchar_t *)L"\r\n");
}

EFI_STATUS
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
	EFI_STATUS status;

	EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
	status = bootsvc->LocateProtocol(&guid_gfx_out, NULL, (void **)&gop);
	if (status != EFI_NOT_FOUND) {
		CHECK_EFI_STATUS_OR_RETURN(status, "LocateProtocol gfx");

		*buffer = (void *)gop->Mode->FrameBufferBase;
		*buffer_size = gop->Mode->FrameBufferSize;
		*hres = gop->Mode->Info->HorizontalResolution;
		*vres = gop->Mode->Info->VerticalResolution;

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
