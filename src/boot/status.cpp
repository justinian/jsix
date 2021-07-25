#include <uefi/types.h>
#include <uefi/graphics.h>
#include <uefi/protos/simple_text_output.h>

#include "console.h"
#include "error.h"
#include "kernel_args.h"
#include "status.h"

constexpr int num_boxes = 30;

namespace boot {

static constexpr int level_ok = 0;
static constexpr int level_warn = 1;
static constexpr int level_fail = 2;

static const wchar_t *level_tags[] = {
	L"  ok  ",
	L" warn ",
	L"failed"
};

static const uefi::attribute level_colors[] = {
	uefi::attribute::green,
	uefi::attribute::brown,
	uefi::attribute::light_red
};

status *status::s_current = nullptr;
unsigned status::s_current_type = 0;
unsigned status_bar::s_count = 0;

status_line::status_line(const wchar_t *message, const wchar_t *context, bool fails_clean) :
	status(fails_clean),
	m_level(level_ok),
	m_depth(0),
	m_outer(nullptr)
{
	if (status::s_current_type == status_line::type) {
		m_outer = static_cast<status_line*>(s_current);
		m_depth = (m_outer ? 1 + m_outer->m_depth : 0);
	}
	s_current = this;
	s_current_type = status_line::type;

	auto out = console::get().m_out;
	m_line = out->mode->cursor_row;

	int indent = 2 * m_depth;
	out->set_cursor_position(indent, m_line);
	out->set_attribute(uefi::attribute::light_gray);
	out->output_string(message);

	if (context) {
		out->output_string(L": ");
		out->output_string(context);
	}

	out->output_string(L"\r\n");
	print_status_tag();
}

status_line::~status_line()
{
	if (s_current != this)
		error::raise(uefi::status::unsupported, L"Destroying non-current status_line");

	if (m_outer && m_level > m_outer->m_level) {
		m_outer->m_level = m_level;
		m_outer->print_status_tag();
	}
	s_current = m_outer;
}

void
status_line::print_status_tag()
{
	auto out = console::get().m_out;
	int row = out->mode->cursor_row;
	int col = out->mode->cursor_column;

	uefi::attribute color = level_colors[m_level];
	const wchar_t *tag = level_tags[m_level];

	out->set_cursor_position(50, m_line);

	out->set_attribute(uefi::attribute::light_gray);
	out->output_string(L"[");
	out->set_attribute(color);
	out->output_string(tag);
	out->set_attribute(uefi::attribute::light_gray);
	out->output_string(L"]\r\n");

	out->set_cursor_position(col, row);
}

void
status_line::do_warn(const wchar_t *message, uefi::status status)
{
	auto out = console::get().m_out;
	int row = out->mode->cursor_row;

	if (m_level < level_warn) {
		m_level = level_warn;
		print_status_tag();
	}

	int indent = 2 + 2 * m_depth;
	out->set_cursor_position(indent, row);
	out->set_attribute(uefi::attribute::yellow);
	out->output_string(message);

	const wchar_t *error = error::message(status);
	if (error) {
		out->output_string(L": ");
		out->output_string(error);
	}

	out->set_attribute(uefi::attribute::light_gray);
	out->output_string(L"\r\n");
}

void
status_line::do_fail(const wchar_t *message, uefi::status status)
{
	auto out = console::get().m_out;
	int row = out->mode->cursor_row;

	if (m_level < level_fail) {
		m_level = level_fail;
		print_status_tag();
	}

	int indent = 2 + 2 * m_depth;
	out->set_cursor_position(indent, row);
	out->set_attribute(uefi::attribute::red);
	out->output_string(message);

	const wchar_t *error = error::message(status);
	if (error) {
		out->output_string(L": ");
		out->output_string(error);
	}

	out->set_attribute(uefi::attribute::light_gray);
	out->output_string(L"\r\n");
}



status_bar::status_bar(kernel::init::framebuffer const &fb) :
	status(fb.size),
	m_outer(nullptr)
{
	m_size = (fb.vertical / num_boxes) - 1;
	m_top = fb.vertical - m_size;
	m_horiz = fb.horizontal;
	m_fb = reinterpret_cast<uint32_t*>(fb.phys_addr);
	m_type = static_cast<uint16_t>(fb.type);
	next();

	if (status::s_current_type == status_bar::type)
		m_outer = static_cast<status_bar*>(s_current);
	s_current = this;
	s_current_type = status_bar::type;
}

status_bar::~status_bar()
{
	if (s_current != this)
		error::raise(uefi::status::unsupported, L"Destroying non-current status_bar");
	draw_box();
	s_current = m_outer;
}

void
status_bar::do_warn(const wchar_t *message, uefi::status status)
{
	m_status = status;
	if (m_level < level_warn) {
		m_level = level_warn;
		draw_box();
	}
}

void
status_bar::do_fail(const wchar_t *message, uefi::status status)
{
	m_status = status;
	if (m_level < level_fail) {
		m_level = level_fail;
		draw_box();
	}
}

static uint32_t
make_color(uint8_t r, uint8_t g, uint8_t b, uint16_t type)
{
	switch (static_cast<kernel::init::fb_type>(type)) {
	case kernel::init::fb_type::bgr8:
		return
			(static_cast<uint32_t>(b) <<  0) |
			(static_cast<uint32_t>(g) <<  8) |
			(static_cast<uint32_t>(r) << 16);

	case kernel::init::fb_type::rgb8:
		return
			(static_cast<uint32_t>(r) <<  0) |
			(static_cast<uint32_t>(g) <<  8) |
			(static_cast<uint32_t>(b) << 16);
	
	default:
		return 0;
	}
}

void
status_bar::draw_box()
{
	static const uint32_t colors[] = {0x909090, 0xf0f0f0};
	constexpr unsigned ncolors = sizeof(colors) / sizeof(uint32_t);

	if (m_fb == nullptr)
		return;

	unsigned x0 = m_current * m_size;
	unsigned x1 = x0 + m_size - 3;
	unsigned y0 = m_top;
	unsigned y1 = m_top + m_size - 3;

	uint32_t color = 0;
	switch (m_level) {
	case level_ok:
		color = colors[m_current % ncolors];
		break;
	case level_warn:
		color = make_color(0xff, 0xb2, 0x34, m_type);
		break;
	case level_fail:
		color = make_color(0xfb, 0x0a, 0x1e, m_type);
		break;
	default:
		color = 0;
	}

	for (unsigned y = y0; y < y1; ++y)
		for (unsigned x = x0; x < x1; ++x)
			m_fb[y * m_horiz + x] = color;

	if (m_level > level_ok) {
		unsigned nbars = static_cast<uint64_t>(m_status) & 0xffff;
		constexpr unsigned bar_height = 4;

		for (unsigned i = 1; i <= nbars; ++i) {
			y0 = m_top - 2 * i * bar_height;
			y1 = y0 + bar_height;

			for (unsigned y = y0; y < y1; ++y)
				for (unsigned x = x0; x < x1; ++x)
					m_fb[y * m_horiz + x] = color;
		}
	}
}

} // namespace boot
