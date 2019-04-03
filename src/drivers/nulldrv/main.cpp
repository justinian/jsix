#include <stdint.h>
#include <stdlib.h>

extern "C" {
	int32_t getpid();
	int32_t fork();
	void sleep(uint64_t til);
	void debug();
	void message(const char *msg);

	int main(int, const char **);
}


int
main(int argc, const char **argv)
{
	int32_t pid = getpid();
	int32_t child = fork();
	message("hello from nulldrv!");
	for (int i = 1; i < 5; ++i)
		sleep(i*10);
	return 0;
}
