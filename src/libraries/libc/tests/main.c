#include "_PDCLIB_test.h"

DECLARE_SUITE(internal);

DECLARE_SUITE(assert);
DECLARE_SUITE(ctype);
DECLARE_SUITE(inttypes);
DECLARE_SUITE(locale);
DECLARE_SUITE(stdarg);
DECLARE_SUITE(stdio);
DECLARE_SUITE(stdlib);
DECLARE_SUITE(string);
DECLARE_SUITE(time);

int main(int argc, const char **argv)
{
	int result = 0;

	result += run_suite_internal();

	result += run_suite_assert();
	result += run_suite_ctype();
	result += run_suite_inttypes();
	result += run_suite_locale();
	result += run_suite_stdarg();
	result += run_suite_stdio();
	result += run_suite_stdlib();
	result += run_suite_string();
	result += run_suite_time();

	return result;
}
