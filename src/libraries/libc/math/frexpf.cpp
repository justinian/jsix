#include "float_ops.h"

extern "C"
float frexpf(float f, int *exp) { return __frexp<single_precision>(f, exp); }
