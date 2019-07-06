#include <stdint.h>
#include <stdlib.h>

#include "j6/types.h"
#include "j6/errors.h"

extern "C" {
	j6_status_t getpid(uint64_t *);
	j6_status_t fork(uint64_t *);
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

	j6_status_t result = fork(&child);
	if (result != j6_status_ok)
		return result;

	message("hello from nulldrv!");

	result = getpid(&pid);
	if (result != j6_status_ok)
		return result;

	for (int i = 1; i < 5; ++i)
		sleep(i*10);

	return pid;
}
