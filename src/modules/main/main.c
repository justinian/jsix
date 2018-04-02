#include <stddef.h>
#include <stdint.h>

void do_the_set_registers();

#pragma pack(push, 1)
struct popcorn_data {
	uint32_t magic;
	uint16_t version;
	uint16_t length;

	uint32_t _reserverd0;
	uint32_t flags;

	void *font;
	size_t font_length;

	void *data;
	size_t data_length;

	void *memory_map;
	void *runtime;

	void *acpi_table;

	void *frame_buffer;
	size_t frame_buffer_size;
	uint32_t hres;
	uint32_t vres;
	uint32_t rmask;
	uint32_t gmask;
	uint32_t bmask;
	uint32_t _reserved1;
}
__attribute__((aligned(8)));
#pragma pack(pop)

void
kernel_main(struct popcorn_data *header)
{
	uint32_t *p = header->frame_buffer;
	uint32_t *end = p + (header->frame_buffer_size / sizeof(uint32_t));
	while (p < end) *p++ = header->rmask;

	do_the_set_registers(header);
}
