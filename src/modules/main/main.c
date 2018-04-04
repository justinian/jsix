#include <stddef.h>
#include <stdint.h>

#include "font.h"
#include "screen.h"

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
	struct screen screen;
	struct font font;
	int status = 0;

	status = font_init(header->font, &font);

	status = screen_init(
		header->frame_buffer,
		header->hres,
		header->vres,
		header->rmask,
		header->gmask,
		header->bmask,
		&screen);

	uint32_t color = header->gmask;
	uint32_t perline = header->hres / font.width;
	const char message[] = "Hello, I am text rendered by the kernel! :-D  ";
	for (int i=0; i<sizeof(message); ++i) {
		font_draw(
			&font,
			&screen,
			message[i],
			color,
			(i % perline) * font.width + 10,
			(i / perline) * font.height + 10);
	}

	do_the_set_registers(header);
}
