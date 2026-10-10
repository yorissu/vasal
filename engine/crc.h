
/// CRC-32 (IEEE 802.3, the one zip and PNG use), to detect corrupted map
/// files.

#ifndef vasal__engine__crc_h
#define vasal__engine__crc_h

#include "engine/types.h"

/// The CRC-32 of size bytes.
uint32_t vasal_engine_crc_compute(const uint8_t* const data,
	const size_t size);

#endif
