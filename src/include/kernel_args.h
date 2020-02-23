#pragma once

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

namespace kernel {
namespace args {

constexpr uint32_t magic = 0x600dda7a;
constexpr uint16_t version = 1;

enum class flags : uint32_t
{
	debug = 0x00000001
};

enum class type : uint32_t {
	unknown,

	kernel,
	initrd,

	memory_map,
	framebuffer,

	max
};

enum class mode : uint8_t {
	normal,
	debug
};

#pragma pack(push, 1)
struct module {
	void *location;
	size_t size;
	type type;
	flags flags;
};

struct header {
	uint32_t magic;
	uint16_t version;

	mode mode;

	uint8_t _reserved0;

	uint32_t _reserved1;
	uint32_t num_modules;
	module *modules;

	void *runtime_services;
	void *acpi_table;
}
__attribute__((aligned(alignof(max_align_t))));
#pragma pack(pop)

} // namespace args
} // namespace kernel
