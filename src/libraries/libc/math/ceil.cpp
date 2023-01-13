#include "float_ops.h"

extern "C"
double ceil(double f) { return __ceil<double_precision>(f); }
