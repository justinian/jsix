#include <stdint.h>
#include <j6/init.h>
#include <j6/types.h>

static size_t __initc = 0;
static struct j6_init_value *__initv = 0;

j6_handle_t __handle_sys = j6_handle_invalid;

void
_get_init(size_t *initc, struct j6_init_value **initv)
{
	if (!initc)
		return;

	*initc = __initc;
	if (initv)
		*initv = __initv;
}

void
_init_libc(uint64_t *rsp)
{
	uint64_t argc = *rsp++;
	rsp += argc;

	__initc = *rsp++;
	__initv = (struct j6_init_value *)rsp;

	for (unsigned i = 0; i < __initc; ++i) {
		if (__initv[i].type == j6_init_handle_system) {
			__handle_sys = __initv[i].value;
			break;
		}
	}
}
