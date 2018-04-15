#include <stddef.h>
#include <stdint.h>

#include "device_manager.h"
#include "console.h"
#include "util.h"

static const char expected_signature[] = "RSD PTR ";

struct Acpi10RSDPDescriptor
{
	char signature[8];
	uint8_t checksum;
	char oem_id[6];
	uint8_t revision;
	uint32_t rsdt_address;
} __attribute__ ((packed));

struct Acpi20RSDPDescriptor
{
	char signature[8];
	uint8_t checksum10;
	char oem_id[6];
	uint8_t revision;
	uint32_t rsdt_address;

	uint32_t length;
	uint64_t xsdt_address;
	uint8_t checksum20;
	uint8_t reserved[3];
} __attribute__ ((packed));


template <typename T>
uint8_t acpi_checksum(const T *p, size_t off = 0)
{
	uint8_t sum = 0;
	const uint8_t *c = reinterpret_cast<const uint8_t *>(p);
	for (int i = off; i < sizeof(T); ++i) sum += c[i];
	return sum;
}

device_manager::device_manager(const void *root_table)
{
	console *cons = console::get();

	kassert(root_table != 0, "ACPI root table pointer is null.");

	const Acpi10RSDPDescriptor *acpi1 =
		reinterpret_cast<const Acpi10RSDPDescriptor *>(root_table);

	for (int i = 0; i < sizeof(acpi1->signature); ++i)
		kassert(acpi1->signature[i] == expected_signature[i],
				"ACPI RSDP table signature mismatch");

	uint8_t sum = acpi_checksum(acpi1, 0);
	kassert(sum == 0, "ACPI 1.0 RSDP checksum mismatch.");

	kassert(acpi1->revision > 1, "ACPI 1.0 not supported.");

	const Acpi20RSDPDescriptor *acpi2 =
		reinterpret_cast<const Acpi20RSDPDescriptor *>(acpi1);

	sum = acpi_checksum(acpi2, sizeof(Acpi10RSDPDescriptor));
	kassert(sum == 0, "ACPI 2.0 RSDP checksum mismatch.");

	cons->puts("ACPI 2.0 tables loading...\n");
}
