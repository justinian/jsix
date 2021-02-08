#include "kutil/assert.h"
#include "kutil/memory.h"
#include "kutil/no_construct.h"
#include "cpu.h"
#include "tss.h"

// The BSP's TSS is initialized _before_ global constructors are called,
// so we don't want it to have a global constructor, lest it overwrite
// the previous initialization.
static kutil::no_construct<TSS> __g_bsp_tss_storage;
TSS &g_bsp_tss = __g_bsp_tss_storage.value;


TSS::TSS()
{
	kutil::memset(this, 0, sizeof(TSS));
	m_iomap_offset = sizeof(TSS);
}

TSS &
TSS::current()
{
	return *current_cpu().tss;
}

uintptr_t &
TSS::ring_stack(unsigned ring)
{
	kassert(ring < 3, "Bad ring passed to TSS::ring_stack.");
	return m_rsp[ring];
}

uintptr_t &
TSS::ist_stack(unsigned ist)
{
	kassert(ist > 0 && ist < 7, "Bad ist passed to TSS::ist_stack.");
	return m_ist[ist];
}

