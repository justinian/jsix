#include "float_ops.h"

extern "C"
double frexp(double f, int *exp) { return __frexp<double_precision>(f, exp); }
