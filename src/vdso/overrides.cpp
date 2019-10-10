#include <j6/errors.h>
#include <j6/types.h>
#include "vdso_internal.h"

extern "C" {

j6_status_t j6_object_noop() {
	// Skip the kernel
	return j6_status_ok;
}

// An example of overriding a weak default symbol with a custom function
j6_status_t j6_object_wait(j6_handle_t target, j6_signal_t signals, j6_signal_t *triggered) {
	return __sys_j6_object_wait(target, signals, triggered);
}

}
