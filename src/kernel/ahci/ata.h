#pragma once
/// \file ata.h
/// Definitions for ATA codes
#include <stdint.h>
#include "kutil/enum_bitfields.h"

namespace ahci {


enum class ata_status : uint8_t
{
	error		= 0x01,
	index		= 0x02,
	corrected	= 0x04,
	data_ready	= 0x08,
	seek_done	= 0x10,
	fault		= 0x20,
	ready		= 0x40,
	busy		= 0x80
};


enum class ata_error : uint8_t
{
	amnf	= 0x01, // Address mark not found
	tkznf	= 0x02, // Track 0 not found
	abort	= 0x04, // Command abort
	mcr		= 0x08, // No media
	idnf	= 0x10, // Id not found
	mc		= 0x20, // No media
	unc		= 0x40, // Uncorrectable
	bbk		= 0x80, // Bad sector
};


enum class ata_cmd : uint8_t
{
	read_pio		= 0x20,
	read_pio_ext	= 0x24,
	read_dma		= 0xC8,
	read_dma_ext	= 0x25,
	write_pio		= 0x30,
	write_pio_ext	= 0x34,
	write_dma		= 0xCA,
	write_dma_ext	= 0x35,
	cache_flush		= 0xE7,
	cache_flush_ext	= 0xEA,
	packet			= 0xA0,
	identify_packet	= 0xA1,
	identify		= 0xEC
};


} // namespace ahci

IS_BITFIELD(ahci::ata_status);
IS_BITFIELD(ahci::ata_error);
