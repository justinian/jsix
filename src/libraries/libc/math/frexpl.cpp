#include <assert.h>

extern "C"
long double frexpl(long double f, int *exp) { assert(false && "NYI"); *exp = 0; return 0; }
