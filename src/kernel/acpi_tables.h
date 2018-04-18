#pragma once

#include <stddef.h>
#include <stdint.h>

#include "kutil/coord.h"
#include "kutil/misc.h"

struct acpi_table_header
{
	uint32_t type;
	uint32_t length;
	uint8_t revision;
	uint8_t checksum;
	char oem_id[6];
	char oem_table[8];
	uint32_t oem_revision;
	uint32_t creator_id;
	uint32_t creator_revision;

	bool validate(uint32_t expected_type = 0) const;
} __attribute__ ((packed));

#define TABLE_HEADER(signature) \
	static const uint32_t type_id = kutil::byteswap(signature); \
	acpi_table_header header;


template <typename T>
bool acpi_validate(const T *t) { return t->header.validate(T::type_id); }

template <typename T>
size_t acpi_table_entries(const T *t, size_t size)
{
	return (t->header.length - sizeof(T)) / size;
}


struct acpi_xsdt
{
	TABLE_HEADER('XSDT');
	acpi_table_header *headers[0];
} __attribute__ ((packed));

struct acpi_apic
{
	TABLE_HEADER('APIC');
	uint32_t local_address;
	uint32_t flags;
	uint8_t controller_data[0];
} __attribute__ ((packed));

