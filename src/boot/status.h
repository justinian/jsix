/// \file status.h
/// Status message and indicator handling
#pragma once

#include <stdint.h>
#include <uefi/types.h>

namespace kernel {
namespace args {
	class framebuffer;
}
}

namespace boot {

// Abstract base class for status reporters.
class status
{
public:
	virtual void do_warn(const wchar_t *message, uefi::status status) = 0;
	virtual void do_fail(const wchar_t *message, uefi::status status) = 0;

	/// Set the state to warning, and print a message. If the state is already at
	/// warning or error, the state is unchanged but the message is still printed.
	/// \arg message  The warning message to print, if text is supported
	/// \arg status   If set, the error or warning code that should be represented
	/// \returns      True if there was a status handler to display the warning
	inline static bool warn(const wchar_t *message, uefi::status status = uefi::status::success) {
		if (!s_current) return false;
		s_current->do_warn(message, status);
		return true;
	}

	/// Set the state to error, and print a message. If the state is already at
	/// error, the state is unchanged but the message is still printed.
	/// \arg message  The error message to print, if text is supported
	/// \arg status   The error or warning code that should be represented
	/// \returns      True if there was a status handler to display the failure
	inline static bool fail(const wchar_t *message, uefi::status status) {
		if (!s_current) return false;
		s_current->do_fail(message, status);
		return true;
	}

protected:
	static status *s_current;
	static unsigned s_current_type;
};

/// Scoped status line reporter. Prints a message and an "OK" if no errors
/// or warnings were reported before destruction, otherwise reports the
/// error or warning.
class status_line :
	public status
{
public:
	constexpr static unsigned type = 1;

	/// Constructor.
	/// \arg message  Description of the operation in progress
	/// \arg context  If non-null, printed after `message` and a colon
	status_line(const wchar_t *message, const wchar_t *context = nullptr);
	~status_line();

	virtual void do_warn(const wchar_t *message, uefi::status status) override;
	virtual void do_fail(const wchar_t *message, uefi::status status) override;

private:
	void print_status_tag();

	size_t m_line;
	int m_level;
	int m_depth;

	status_line *m_outer;
};

/// Scoped status bar reporter. Draws a row of boxes along the bottom of
/// the screen, turning one red if there's an error in that step.
class status_bar :
	public status
{
public:
	constexpr static unsigned type = 2;

	using framebuffer = kernel::args::framebuffer;

	/// Constructor.
	/// \arg fb  The framebuffer descriptor to draw to
	status_bar(kernel::args::framebuffer const &fb);
	~status_bar();

	virtual void do_warn(const wchar_t *message, uefi::status status) override;
	virtual void do_fail(const wchar_t *message, uefi::status status) override;

	inline void next() {
		m_current = s_count++;
		m_level = 0;
		m_status = uefi::status::success;
		draw_box();
	}

private:
	void draw_box();

	uint32_t *m_fb;
	uint32_t m_size;
	uint32_t m_top;
	uint32_t m_horiz;

	int m_level;
	uefi::status m_status;

	uint16_t m_type;
	uint16_t m_current;

	status_bar *m_outer;

	static unsigned s_count;
};

} // namespace boot
