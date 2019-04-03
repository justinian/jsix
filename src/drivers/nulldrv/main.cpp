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
	message("hello from nulldrv!");
	//int32_t child = fork();
	//debug();
	for (int i = 1; i < 5; ++i)
		sleep(i*10);
	debug();
	return 0;
}
