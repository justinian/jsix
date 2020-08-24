#include <stdint.h>
void *sbrk(intptr_t) __attribute__ ((weak));

void *sbrk(intptr_t i) { return 0; }
