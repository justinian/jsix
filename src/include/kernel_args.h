#pragma once

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

#define DATA_HEADER_MAGIC     0x600dda7a
#define DATA_HEADER_VERSION   1

#define POPCORN_FLAG_DEBUG    0x00000001

#pragma pack(push, 1)
struct kernel_args {
	uint32_t magic;
	uint16_t version;
	uint16_t length;

	uint16_t _reserved0;
	uint16_t scratch_pages;
	uint32_t flags;

	void *initrd;
	size_t initrd_length;

	void *data;
	size_t data_length;

	void *memory_map;
	size_t memory_map_length;
	size_t memory_map_desc_size;

	void *runtime;

	void *acpi_table;

	void *frame_buffer;
	size_t frame_buffer_length;
	uint32_t hres;
	uint32_t vres;
	uint32_t rmask;
	uint32_t gmask;
	uint32_t bmask;
	uint32_t _reserved1;
}
__attribute__((aligned(alignof(max_align_t))));
#pragma pack(pop)
