#include <stdint.h>
#include <stdlib.h>

#include "j6/types.h"
#include "j6/errors.h"

extern "C" {
	j6_status_t get_process_koid(j6_koid_t *koid);
	j6_status_t sleep(uint64_t til);
	j6_status_t debug();
	j6_status_t message(const char *msg);

	int main(int, const char **);
}


int
main(int argc, const char **argv)
{
	uint64_t pid = 0;
	uint64_t child = 0;
	j6_koid_t process = 0;

	j6_status_t result = get_process_koid(&process);
	if (result != j6_status_ok)
		return result;

	message("hello from nulldrv!");

	for (int i = 1; i < 5; ++i)
		sleep(i*10);

	return pid;
}
