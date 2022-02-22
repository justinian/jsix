#include <j6/errors.h>
#include <j6/types.h>

#include "assert.h"
#include "objects/kobject.h"
#include "objects/thread.h"

namespace obj {

static j6_koid_t next_koids [static_cast<size_t>(kobject::type::max)] = { 0 };

kobject::kobject(type t) :
    m_koid(koid_generate(t)),
    m_handle_count(0)
{}

kobject::~kobject() {}

j6_koid_t
kobject::koid_generate(type t)
{
    kassert(t < type::max, "Object type out of bounds");
    uint64_t type_int = static_cast<uint64_t>(t);
    uint64_t id = __atomic_fetch_add(&next_koids[type_int], 1, __ATOMIC_SEQ_CST);
    return (type_int << 48) | id;
}

kobject::type
kobject::koid_type(j6_koid_t koid)
{
    return static_cast<type>((koid >> 48) & 0xffffull);
}

void
kobject::on_no_handles()
{
    delete this;
}

} // namespace obj
