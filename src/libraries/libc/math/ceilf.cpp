#include "float_ops.h"

extern "C"
float ceilf(float f) { return __ceil<single_precision>(f); }
