#include "kutil/assert.h"
#include "kutil/guid.h"
#include "kutil/memory.h"
#include "device_manager.h"
#include "fs/gpt.h"
#include "log.h"

namespace fs {

const kutil::guid efi_system_part = kutil::make_guid(0xC12A7328, 0xF81F, 0x11D2, 0xBA4B, 0x00A0C93EC93B);
const kutil::guid efi_unused_part = kutil::make_guid(0, 0, 0, 0, 0);

const uint64_t gpt_signature = 0x5452415020494645; // "EFI PART"
const size_t block_size = 512;

struct gpt_header
{
	uint64_t signature;
	uint32_t revision;
	uint32_t headersize;
	uint32_t crc32;
	uint32_t reserved;
	uint64_t my_lba;
	uint64_t alt_lba;
	uint64_t first_usable_lba;
	uint64_t last_usable_lba;
	kutil::guid disk_guid;
	uint64_t table_lba;
	uint32_t entry_count;
	uint32_t entry_length;
	uint32_t table_crc32;
} __attribute__ ((packed));

struct gpt_entry
{
	kutil::guid type;
	kutil::guid part_guid;
	uint64_t start_lba;
	uint64_t end_lba;
	uint64_t attributes;
	uint16_t name_wide[36];
} __attribute__ ((packed));


partition::partition(block_device *parent, size_t start, size_t length) :
	m_parent(parent),
	m_start(start),
	m_length(length)
{
}

size_t
partition::read(size_t offset, size_t length, void *buffer)
{
	if (offset + length > m_length)
		offset = m_length - offset;
	return m_parent->read(m_start + offset, length, buffer);
}


unsigned
partition::load(block_device *device)
{
	// Read LBA 1
	uint8_t block[block_size];
	size_t count = device->read(block_size, block_size, &block);
	kassert(count == block_size, "Short read for GPT header.");

	gpt_header *header = reinterpret_cast<gpt_header *>(&block);
	if (header->signature != gpt_signature)
		return 0;

	size_t arraysize = header->entry_length * header->entry_count;
	log::debug(logs::fs, "Found GPT header: %d paritions, size 0x%lx",
			header->entry_count, arraysize);

	uint8_t *array = new uint8_t[arraysize];
	count = device->read(block_size * header->table_lba, arraysize, array);
	kassert(count == arraysize, "Short read for GPT entry array.");

	auto &dm = device_manager::get();

	unsigned found = 0;
	gpt_entry *entry0 = reinterpret_cast<gpt_entry *>(array);
	for (uint32_t i = 0; i < header->entry_count; ++i) {
		gpt_entry *entry = kutil::offset_pointer(entry0, i * header->entry_length);
		if (entry->type == efi_unused_part) continue;

		// TODO: real UTF16->UTF8
		char name[sizeof(gpt_entry::name_wide) / 2];
		for (int i = 0; i < sizeof(name); ++i)
			name[i] = entry->name_wide[i];

		log::debug(logs::fs, "Found partition %02x at %lx-%lx", i, entry->start_lba, entry->end_lba);
		if (entry->type == efi_system_part)
			log::debug(logs::fs, "   type EFI SYSTEM PARTITION");
		else
			log::debug(logs::fs, "   type %G", entry->type);
		log::debug(logs::fs, "   name %s", name);
		log::debug(logs::fs, "   attr %016lx", entry->attributes);

		found += 1;
		partition *part = new partition(
				device,
				entry->start_lba * block_size,
				(entry->end_lba - entry->start_lba) * block_size);
		dm.register_block_device(part);
	}

	return found;
}


} // namespace fs
