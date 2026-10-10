
/// The map file: one binary file, little-endian, written and read by this
/// module for the editor, the server and the client alike. Map files come
/// from users and travel over the network, so reading checks every field
/// and refuses anything out of place.
///
/// Layout: the magic word "vasalmap", the format version (u16), width and
/// height (u16 each), the map's name, the terrain types (a u8 count, then
/// name, color u32, speed u16, passable u8 each), one byte per tile, then
/// the settlements, spawn areas, bandit camps and decorations (a u16 count
/// each, then their fields), and last a CRC-32 (u32) of everything before
/// it. Names take vasal_engine_map_name_capacity bytes, zero-padded.

#ifndef vasal__engine__mapfile_h
#define vasal__engine__mapfile_h

#include "engine/map.h"

/// The format version this code writes and reads.
#define vasal_engine_mapfile_version 1U

/// The largest map file, in bytes.
#define vasal_engine_mapfile_capacity                                         \
	(64U + (vasal_engine_map_terrains_max * 40U)                               \
		+ (vasal_engine_map_side_max * vasal_engine_map_side_max)             \
		+ (vasal_engine_map_settlements_max * 64U)                            \
		+ (vasal_engine_map_spawns_max * 8U)                                  \
		+ (vasal_engine_map_camps_max * 8U)                                   \
		+ (vasal_engine_map_decorations_max * 8U))

/// The outcome of reading a map file.
typedef enum vasal_engine_mapfile_read_e {
	/// The map was read.
	vasal_engine_mapfile_read_ok,
	/// The file does not start with the magic word.
	vasal_engine_mapfile_read_magic,
	/// The file has another format version.
	vasal_engine_mapfile_read_version,
	/// The file ends early or has bytes after its checksum.
	vasal_engine_mapfile_read_length,
	/// The checksum does not match: the file is corrupted.
	vasal_engine_mapfile_read_checksum,
	/// The size, a count or a value is out of its range.
	vasal_engine_mapfile_read_value
} vasal_engine_mapfile_read_e;

/// Writes the map into buffer. Returns the file's size, or 0 when it does
/// not fit into capacity bytes.
size_t vasal_engine_mapfile_write(const vasal_engine_map_s* const map,
	uint8_t* const buffer, const size_t capacity);

/// Reads a map file of size bytes into map. Unless the outcome is ok, the
/// map's content is undefined. version receives the file's version, for
/// messages about a version mismatch.
vasal_engine_mapfile_read_e vasal_engine_mapfile_read(
	const uint8_t* const data, const size_t size,
	vasal_engine_map_s* const map, uint16_t* const version);

#endif
