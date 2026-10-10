
/// Writes and reads map files section by section, in the order the header
/// describes. Reading validates as it goes and stops at the first problem.

#include "engine/mapfile.h"
#include "engine/bytes.h"
#include "engine/crc.h"

static const uint8_t _vasal_engine_mapfile_magic[8] = {
	'v', 'a', 's', 'a', 'l', 'm', 'a', 'p'
};

static void _vasal_engine_mapfile_write_name(
		vasal_engine_bytes_writer_s* const writer, const char* const name) {
	bool_t ended = false;
	for (size_t i = 0U; i < vasal_engine_map_name_capacity; i++) {
		ended = ended || ('\0' == name[i]);
		const uint8_t byte = ended ? 0U : (uint8_t)name[i];
		vasal_engine_bytes_write_u8(writer, byte);
	}
}

static void _vasal_engine_mapfile_write_point(
		vasal_engine_bytes_writer_s* const writer,
		const vasal_engine_map_point_s point) {
	vasal_engine_bytes_write_u16(writer, point.x);
	vasal_engine_bytes_write_u16(writer, point.y);
}

// spec: mapfile.format
size_t vasal_engine_mapfile_write(const vasal_engine_map_s* const map,
	uint8_t* const buffer, const size_t capacity) {
	vasal_engine_bytes_writer_s writer =
		vasal_engine_bytes_writer(buffer, capacity);
	vasal_engine_bytes_write(&writer, _vasal_engine_mapfile_magic,
		sizeof(_vasal_engine_mapfile_magic));
	vasal_engine_bytes_write_u16(&writer, vasal_engine_mapfile_version);
	vasal_engine_bytes_write_u16(&writer, map->width);
	vasal_engine_bytes_write_u16(&writer, map->height);
	_vasal_engine_mapfile_write_name(&writer, map->name);

	vasal_engine_bytes_write_u8(&writer, map->terrain_count);
	for (size_t i = 0U; i < map->terrain_count; i++) {
		const vasal_engine_map_terrain_s* const terrain = &map->terrains[i];
		_vasal_engine_mapfile_write_name(&writer, terrain->name);
		vasal_engine_bytes_write_u32(&writer, terrain->color);
		vasal_engine_bytes_write_u16(&writer, terrain->speed);
		vasal_engine_bytes_write_u16(&writer, terrain->sight);
		vasal_engine_bytes_write_u8(&writer, terrain->passable ? 1U : 0U);
	}
	vasal_engine_bytes_write(&writer, map->tiles,
		(size_t)map->width * (size_t)map->height);

	vasal_engine_bytes_write_u16(&writer, map->settlement_count);
	for (size_t i = 0U; i < map->settlement_count; i++) {
		const vasal_engine_map_settlement_s* const settlement =
			&map->settlements[i];
		_vasal_engine_mapfile_write_name(&writer, settlement->name);
		_vasal_engine_mapfile_write_point(&writer, settlement->position);
		vasal_engine_bytes_write_u16(&writer, settlement->radius);
		vasal_engine_bytes_write_u16(&writer, settlement->requirement);
		vasal_engine_bytes_write_u32(&writer, settlement->capture_time);
		vasal_engine_bytes_write_u32(&writer, settlement->bonus);
		vasal_engine_bytes_write_u32(&writer, settlement->income);
		vasal_engine_bytes_write_u32(&writer, settlement->flags);
		vasal_engine_bytes_write_u8(&writer, settlement->type);
	}
	vasal_engine_bytes_write_u16(&writer, map->spawn_count);
	for (size_t i = 0U; i < map->spawn_count; i++) {
		_vasal_engine_mapfile_write_point(&writer, map->spawns[i].position);
		vasal_engine_bytes_write_u16(&writer, map->spawns[i].radius);
	}
	vasal_engine_bytes_write_u16(&writer, map->camp_count);
	for (size_t i = 0U; i < map->camp_count; i++) {
		_vasal_engine_mapfile_write_point(&writer, map->camps[i].position);
	}
	vasal_engine_bytes_write_u16(&writer, map->decoration_count);
	for (size_t i = 0U; i < map->decoration_count; i++) {
		_vasal_engine_mapfile_write_point(&writer,
			map->decorations[i].position);
		vasal_engine_bytes_write_u16(&writer, map->decorations[i].kind);
	}

	if (writer.failed) {
		return 0U;
	}
	vasal_engine_bytes_write_u32(&writer,
		vasal_engine_crc_compute(buffer, writer.size));
	return writer.failed ? 0U : writer.size;
}

// Reads a name: printable ASCII, then zeros to the end of its field, with
// at least one zero.
static bool_t _vasal_engine_mapfile_read_name(
		vasal_engine_bytes_reader_s* const reader, char* const name) {
	uint8_t bytes[vasal_engine_map_name_capacity];
	vasal_engine_bytes_read(reader, bytes, sizeof(bytes));
	bool_t ended = false;
	bool_t valid = true;
	for (size_t i = 0U; i < sizeof(bytes); i++) {
		const uint8_t byte = bytes[i];
		if (0U == byte) {
			ended = true;
		} else {
			valid = valid && !ended && (0x20U <= byte) && (0x7eU >= byte);
		}
		name[i] = (char)byte;
	}
	return valid && ended;
}

// Reads a point and checks it is on the map.
static bool_t _vasal_engine_mapfile_read_point(
		vasal_engine_bytes_reader_s* const reader,
		const vasal_engine_map_s* const map,
		vasal_engine_map_point_s* const point) {
	point->x = vasal_engine_bytes_read_u16(reader);
	point->y = vasal_engine_bytes_read_u16(reader);
	return vasal_engine_map_contains(map, point->x, point->y);
}

static bool_t _vasal_engine_mapfile_read_terrains(
		vasal_engine_bytes_reader_s* const reader,
		vasal_engine_map_s* const map) {
	map->terrain_count = vasal_engine_bytes_read_u8(reader);
	if ((0U == map->terrain_count)
			|| (vasal_engine_map_terrains_max < map->terrain_count)) {
		return false;
	}
	for (size_t i = 0U; i < map->terrain_count; i++) {
		vasal_engine_map_terrain_s* const terrain = &map->terrains[i];
		const bool_t named =
			_vasal_engine_mapfile_read_name(reader, terrain->name);
		terrain->color = vasal_engine_bytes_read_u32(reader);
		terrain->speed = vasal_engine_bytes_read_u16(reader);
		terrain->sight = vasal_engine_bytes_read_u16(reader);
		const uint8_t passable = vasal_engine_bytes_read_u8(reader);
		terrain->passable = 1U == passable;
		// A passable terrain has a speed, an impassable one none.
		bool_t speed_valid = (0U == terrain->speed);
		if (terrain->passable) {
			speed_valid = (0U < terrain->speed)
				&& (vasal_engine_map_speed_max >= terrain->speed);
		}
		if (!named || (1U < passable) || !speed_valid
				|| (vasal_engine_map_sight_max < terrain->sight)) {
			return false;
		}
	}
	return true;
}

static bool_t _vasal_engine_mapfile_read_tiles(
		vasal_engine_bytes_reader_s* const reader,
		vasal_engine_map_s* const map) {
	const size_t count = (size_t)map->width * (size_t)map->height;
	vasal_engine_bytes_read(reader, map->tiles, count);
	for (size_t i = 0U; i < count; i++) {
		if (map->tiles[i] >= map->terrain_count) {
			return false;
		}
	}
	return true;
}

static bool_t _vasal_engine_mapfile_read_settlements(
		vasal_engine_bytes_reader_s* const reader,
		vasal_engine_map_s* const map) {
	map->settlement_count = vasal_engine_bytes_read_u16(reader);
	if (vasal_engine_map_settlements_max < map->settlement_count) {
		return false;
	}
	for (size_t i = 0U; i < map->settlement_count; i++) {
		vasal_engine_map_settlement_s* const settlement =
			&map->settlements[i];
		const bool_t named =
			_vasal_engine_mapfile_read_name(reader, settlement->name);
		const bool_t placed = _vasal_engine_mapfile_read_point(reader, map,
			&settlement->position);
		settlement->radius = vasal_engine_bytes_read_u16(reader);
		settlement->requirement = vasal_engine_bytes_read_u16(reader);
		settlement->capture_time = vasal_engine_bytes_read_u32(reader);
		settlement->bonus = vasal_engine_bytes_read_u32(reader);
		settlement->income = vasal_engine_bytes_read_u32(reader);
		settlement->flags = vasal_engine_bytes_read_u32(reader);
		settlement->type = vasal_engine_bytes_read_u8(reader);
		if (!named || !placed || (0U == settlement->radius)
				|| (vasal_engine_map_side_max < settlement->radius)
				|| (0U == settlement->requirement)
				|| (0U == settlement->capture_time)
				|| ((uint8_t)vasal_engine_map_settlement_types
					<= settlement->type)
				|| (0U != (settlement->flags & ~vasal_engine_map_flags))) {
			return false;
		}
	}
	return true;
}

static bool_t _vasal_engine_mapfile_read_places(
		vasal_engine_bytes_reader_s* const reader,
		vasal_engine_map_s* const map) {
	map->spawn_count = vasal_engine_bytes_read_u16(reader);
	if (vasal_engine_map_spawns_max < map->spawn_count) {
		return false;
	}
	for (size_t i = 0U; i < map->spawn_count; i++) {
		vasal_engine_map_spawn_s* const spawn = &map->spawns[i];
		const bool_t placed = _vasal_engine_mapfile_read_point(reader, map,
			&spawn->position);
		spawn->radius = vasal_engine_bytes_read_u16(reader);
		if (!placed || (0U == spawn->radius)
				|| (vasal_engine_map_side_max < spawn->radius)) {
			return false;
		}
	}

	map->camp_count = vasal_engine_bytes_read_u16(reader);
	if (vasal_engine_map_camps_max < map->camp_count) {
		return false;
	}
	for (size_t i = 0U; i < map->camp_count; i++) {
		if (!_vasal_engine_mapfile_read_point(reader, map,
				&map->camps[i].position)) {
			return false;
		}
	}

	map->decoration_count = vasal_engine_bytes_read_u16(reader);
	if (vasal_engine_map_decorations_max < map->decoration_count) {
		return false;
	}
	for (size_t i = 0U; i < map->decoration_count; i++) {
		vasal_engine_map_decoration_s* const decoration =
			&map->decorations[i];
		const bool_t placed = _vasal_engine_mapfile_read_point(reader, map,
			&decoration->position);
		decoration->kind = vasal_engine_bytes_read_u16(reader);
		if (!placed) {
			return false;
		}
	}
	return true;
}

// spec: mapfile.format mapfile.validation
vasal_engine_mapfile_read_e vasal_engine_mapfile_read(
	const uint8_t* const data, const size_t size,
	vasal_engine_map_s* const map, uint16_t* const version) {
	vasal_engine_bytes_reader_s reader = vasal_engine_bytes_reader(data, size);
	uint8_t magic[sizeof(_vasal_engine_mapfile_magic)];
	vasal_engine_bytes_read(&reader, magic, sizeof(magic));
	for (size_t i = 0U; i < sizeof(magic); i++) {
		if (magic[i] != _vasal_engine_mapfile_magic[i]) {
			return vasal_engine_mapfile_read_magic;
		}
	}
	*version = vasal_engine_bytes_read_u16(&reader);
	if (reader.failed) {
		return vasal_engine_mapfile_read_length;
	}
	if (vasal_engine_mapfile_version != *version) {
		return vasal_engine_mapfile_read_version;
	}
	if ((4U > size) || (vasal_engine_mapfile_capacity < size)) {
		return vasal_engine_mapfile_read_length;
	}

	// The checksum covers everything before its own four bytes.
	vasal_engine_bytes_reader_s tail =
		vasal_engine_bytes_reader(&data[size - 4U], 4U);
	if (vasal_engine_bytes_read_u32(&tail)
			!= vasal_engine_crc_compute(data, size - 4U)) {
		return vasal_engine_mapfile_read_checksum;
	}
	reader.size = size - 4U;

	map->width = vasal_engine_bytes_read_u16(&reader);
	map->height = vasal_engine_bytes_read_u16(&reader);
	const bool_t sized = (0U < map->width) && (0U < map->height)
		&& (vasal_engine_map_side_max >= map->width)
		&& (vasal_engine_map_side_max >= map->height);
	if (!sized) {
		return reader.failed ? vasal_engine_mapfile_read_length
			: vasal_engine_mapfile_read_value;
	}
	const bool_t valid =
		_vasal_engine_mapfile_read_name(&reader, map->name)
		&& _vasal_engine_mapfile_read_terrains(&reader, map)
		&& _vasal_engine_mapfile_read_tiles(&reader, map)
		&& _vasal_engine_mapfile_read_settlements(&reader, map)
		&& _vasal_engine_mapfile_read_places(&reader, map);
	// Reading stops at the first invalid section, so a value problem is
	// only a length problem when the reader ran out of bytes.
	if (reader.failed) {
		return vasal_engine_mapfile_read_length;
	}
	if (!valid) {
		return vasal_engine_mapfile_read_value;
	}
	return (reader.at == reader.size) ? vasal_engine_mapfile_read_ok
		: vasal_engine_mapfile_read_length;
}
