#include <stdlib.h>
#include <stdio.h>

#include "j6/errors.h"
#include "j6/syscalls.h"
#include "init_args.h"
#include "pointer_manipulation.h"

#include "modules.h"

using namespace kernel::init;

extern j6_handle_t handle_self;
extern j6_handle_t handle_system;

namespace modules {

static const modules_page *
load_page(uintptr_t address)
{
	j6_handle_t mods_vma = j6_handle_invalid;
	j6_status_t s = j6_system_map_phys(handle_system, &mods_vma, address, 0x1000, 0);
	if (s != j6_status_ok)
		exit(s);

	s = j6_vma_map(mods_vma, handle_self, address);
	if (s != j6_status_ok)
		exit(s);

	return reinterpret_cast<modules_page*>(address);
}

void
load_all(uintptr_t address)
{
	module_framebuffer const *framebuffer = nullptr;

	while (address) {
		const modules_page *page = load_page(address);

		char message[100];
		sprintf(message, "srv.init loading %d modules from page at 0x%lx", page->count, address);
		j6_system_log(message);

		module *mod = page->modules;
		size_t count = page->count;
		while (count--) {
			switch (mod->mod_type) {
				case module_type::framebuffer:
					framebuffer = reinterpret_cast<const module_framebuffer*>(mod);
					break;

				case module_type::program:
					if (mod->mod_flags == module_flags::no_load)
						j6_system_log("Loaded program module");
					else
						j6_system_log("Non-loaded program module");
					break;

				default:
					j6_system_log("Unknown module");
			}
			mod = offset_ptr<module>(mod, mod->mod_length);
		}

		address = page->next;
	}
}

} // namespace modules
