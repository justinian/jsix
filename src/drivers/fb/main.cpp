#include <stdint.h>
#include <stdlib.h>

#include "j6/init.h"
#include "j6/errors.h"
#include "j6/signals.h"
#include "j6/types.h"

#include <j6libc/syscalls.h>

extern "C" {
	int main(int, const char **);
	void _get_init(size_t *initc, struct j6_init_value **initv);
}

int
main(int argc, const char **argv)
{
	_syscall_system_log("fb driver starting");

	size_t initc = 0;
	j6_init_value *initv = nullptr;
	_get_init(&initc, &initv);

	j6_init_framebuffer *fb = nullptr;
	for (unsigned i = 0; i < initc; ++i) {
		if (initv[i].type == j6_init_desc_framebuffer) {
			fb = reinterpret_cast<j6_init_framebuffer*>(initv[i].value);
			break;
		}
	}

	if (!fb)
		return 1;

	uint32_t *fbp = reinterpret_cast<uint32_t*>(fb->addr);
	size_t size = fb->size;
	for (size_t i=0; i < size/4; ++i) {
		fbp[i] = 0xff;
	}

	_syscall_system_log("fb driver done, exiting");
	return 0;
}

