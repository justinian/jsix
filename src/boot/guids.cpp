#include "guids.h"

#define GUID(dw, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, name) \
	EFI_GUID name __attribute__((section(".rodata"))) = {dw, w1, w2, {b1, b2, b3, b4, b5, b6, b7, b8}}
#include "guids.inc"
#undef GUID

int is_guid(EFI_GUID *a, EFI_GUID *b)
{
	uint64_t *ai = (uint64_t *)a;
	uint64_t *bi = (uint64_t *)b;
	return ai[0] == bi[0] && ai[1] == bi[1];
}
