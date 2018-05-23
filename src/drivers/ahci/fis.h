#pragma once
/// \file fis.h
/// Definitions for Frame Information Structure types. (Not for pescatarians.)
#include <stdint.h>


namespace ahci {

enum class ata_cmd : uint8_t;

enum class fis_type : uint8_t
{
	register_h2d	= 0x27,
	register_d2h	= 0x34,
	dma_activate	= 0x39,
	dma_setup		= 0x41,
	data			= 0x46,
	bist			= 0x58,
	pio_setup		= 0x5f,
	device_bits		= 0xa1
};


struct fis_register_h2d
{
	fis_type type;
	uint8_t pm_port;  // high bit (0x80) is set for the command register flag
	ata_cmd command;
	uint8_t features;

	uint8_t lba0;
	uint8_t lba1;
	uint8_t lba2;
	uint8_t device;

	uint8_t lba3;
	uint8_t lba4;
	uint8_t lba5;
	uint8_t features2;

	uint8_t count0;
	uint8_t count1;
	uint8_t icc;
	uint8_t control;

	uint32_t reserved;
};

} // namespace ahci
