#include <j6/errors.h>
#include <j6/types.h>
#include <util/vector.h>

#include "kassert.h"
#include "logger.h"
#include "objects/thread.h"
#include "syscalls/helpers.h"

using namespace obj;

namespace syscalls {

j6_status_t
object_koid(kobject *self, j6_koid_t *koid)
{
    *koid = self->koid();
    return j6_status_ok;
}

} // namespace syscalls
