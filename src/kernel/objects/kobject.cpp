#include <j6/errors.h>
#include <j6/types.h>

#include "assert.h"
#include "objects/kobject.h"
#include "objects/thread.h"

namespace obj {

static constexpr unsigned types_count = static_cast<unsigned>(kobject::type::max);

static uint32_t next_oids [types_count] = { 0 };

static_assert(types_count <= (1 << kobject::koid_type_bits),
        "kobject::koid_type_bits cannot represent all kobject types");

static uint32_t
oid_generate(kobject::type t)
{
    kassert(t < kobject::type::max, "Object type out of bounds");
    unsigned type_int = static_cast<unsigned>(t);
    return __atomic_fetch_add(&next_oids[type_int], 1, __ATOMIC_RELAXED);
}

kobject::kobject(type t) :
    m_handle_count {0},
    m_type {t},
    m_obj_id {oid_generate(t)}
{}

kobject::~kobject() {}

void
kobject::on_no_handles()
{
    delete this;
}

} // namespace obj
