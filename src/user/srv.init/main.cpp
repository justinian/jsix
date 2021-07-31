#include "j6/syscalls.h"
#include "modules.h"

extern "C" {
	int main(int, const char **);
}

uintptr_t _arg_modules_phys;   // This gets filled in in _start

j6_handle_t handle_self   = 1; // Self program handle is always 1
j6_handle_t handle_system = 2; // boot protocol is that init gets the system as handle 2

int
main(int argc, const char **argv)
{
	j6_system_log("srv.init starting");
	modules::load_all(_arg_modules_phys);

	return 0;
}
