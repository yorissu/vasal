
/// CRC-32 with a 16-entry table, one lookup per 4 bits: small, and fast
/// enough for map files of a few megabytes.

#include "engine/crc.h"

// The CRC of each 4-bit value, for the reflected polynomial 0xedb88320.
static const uint32_t _vasal_engine_crc_table[16] = {
	0x00000000U, 0x1db71064U, 0x3b6e20c8U, 0x26d930acU,
	0x76dc4190U, 0x6b6b51f4U, 0x4db26158U, 0x5005713cU,
	0xedb88320U, 0xf00f9344U, 0xd6d6a3e8U, 0xcb61b38cU,
	0x9b64c2b0U, 0x86d3d2d4U, 0xa00ae278U, 0xbdbdf21cU
};

uint32_t vasal_engine_crc_compute(const uint8_t* const data,
	const size_t size) {
	uint32_t crc = 0xffffffffU;
	for (size_t i = 0U; i < size; i++) {
		crc ^= (uint32_t)data[i];
		crc = (crc >> 4U) ^ _vasal_engine_crc_table[crc & 0x0fU];
		crc = (crc >> 4U) ^ _vasal_engine_crc_table[crc & 0x0fU];
	}
	return crc ^ 0xffffffffU;
}
