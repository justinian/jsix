#include <j6/errors.h>
#include <j6/types.h>

#include "kassert.h"
#include "logger.h"
#include "objects/kobject.h"
#include "objects/thread.h"

namespace obj {

static constexpr unsigned types_count = static_cast<unsigned>(kobject::type::max);

static uint32_t next_oids [types_count] = { 0 };

static_assert(types_count <= (1 << kobject::koid_type_bits),
        "kobject::koid_type_bits cannot represent all kobject types");

static const char *type_names[] = {
#define OBJECT_TYPE( name ) #name ,
#include <j6/tables/object_types.inc>
#undef OBJECT_TYPE
    nullptr
};

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
{
    log::spam(logs::objs, "%s[%02lx] created @ 0x%lx", type_name(m_type), m_obj_id, this);
}

kobject::~kobject()
{
    log::spam(logs::objs, "%s[%02lx] deleted", type_name(m_type), m_obj_id);
}

const char *
kobject::type_name(type t)
{
    return type_names[static_cast<int>(t)];
}

void
kobject::on_no_handles()
{
    log::verbose(logs::objs, "Deleting %s[%02lx] on no handles", type_name(m_type), m_obj_id);
    delete this;
}

} // namespace obj
