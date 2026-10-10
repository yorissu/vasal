
/// The demonstration map, drawn from simple shapes so it is the same every
/// time: a river running north to south with a ford where the road crosses
/// it, two forests, patches of tall and short grass, a settlement of every
/// type and a few lone structures.

#include "client/demo.h"

#include <string.h>

enum {
	_vasal_client_demo_grass = vasal_engine_map_standard_grass,
	_vasal_client_demo_short = vasal_engine_map_standard_short_grass,
	_vasal_client_demo_tall = vasal_engine_map_standard_tall_grass,
	_vasal_client_demo_forest = vasal_engine_map_standard_forest,
	_vasal_client_demo_road = vasal_engine_map_standard_road,
	_vasal_client_demo_water = vasal_engine_map_standard_water
};

static void _vasal_client_demo_set(vasal_engine_map_s* const map,
		const int32_t x, const int32_t y, const uint8_t terrain) {
	if ((x >= 0) && (y >= 0) && (x < (int32_t)map->width)
			&& (y < (int32_t)map->height)) {
		map->tiles[((size_t)y * map->width) + (size_t)x] = terrain;
	}
}

// Fills an ellipse around (cx, cy) with radii rx and ry.
static void _vasal_client_demo_patch(vasal_engine_map_s* const map,
		const int32_t cx, const int32_t cy, const int32_t rx, const int32_t ry,
		const uint8_t terrain) {
	for (int32_t y = cy - ry; y <= (cy + ry); y++) {
		for (int32_t x = cx - rx; x <= (cx + rx); x++) {
			const int64_t dx = ((int64_t)x - (int64_t)cx) * ry;
			const int64_t dy = ((int64_t)y - (int64_t)cy) * rx;
			if (((dx * dx) + (dy * dy)) <= ((int64_t)rx * rx * ry * ry)) {
				_vasal_client_demo_set(map, x, y, terrain);
			}
		}
	}
}

// A settlement of a type, with its defaults, named, at a tile.
static void _vasal_client_demo_settlement(vasal_engine_map_s* const map,
		const char* const name, const vasal_engine_map_settlement_type_e type,
		const uint16_t x, const uint16_t y) {
	vasal_engine_map_settlement_s* const settlement =
		&map->settlements[map->settlement_count];
	vasal_engine_map_settlement_defaults(settlement, type);
	size_t length = strlen(name);
	if (length > (vasal_engine_map_name_capacity - 1U)) {
		length = vasal_engine_map_name_capacity - 1U;
	}
	(void)memcpy(settlement->name, name, length);
	settlement->name[length] = '\0';
	settlement->position.x = x;
	settlement->position.y = y;
	map->settlement_count++;
}

// A few settlements of every type, and lone structures: watchtowers by the
// river, a bridge where the road crosses it, a windmill, a chapel, a ruin.
// spec: client.render#7
static void _vasal_client_demo_settlements(vasal_engine_map_s* const map) {
	_vasal_client_demo_settlement(map, "Ashford",
		vasal_engine_map_settlement_village, 80U, 106U);
	_vasal_client_demo_settlement(map, "Bramwell",
		vasal_engine_map_settlement_town, 45U, 152U);
	_vasal_client_demo_settlement(map, "Corvin",
		vasal_engine_map_settlement_castle, 170U, 96U);
	_vasal_client_demo_settlement(map, "Dunmere",
		vasal_engine_map_settlement_city, 205U, 135U);
	// Kinds: 0 to 3 watchtowers, 4 to 6 chapels, 7 a windmill, 8 a bridge,
	// 9 a ruin (engine/sprites.h).
	static const uint16_t structures[6][3] = {
		{ 112U, 116U, 0U }, { 140U, 60U, 2U }, { 124U, 123U, 8U },
		{ 40U, 128U, 7U }, { 95U, 95U, 4U }, { 150U, 200U, 9U }
	};
	for (size_t i = 0U; i < 6U; i++) {
		map->decorations[i].position.x = structures[i][0];
		map->decorations[i].position.y = structures[i][1];
		map->decorations[i].kind = structures[i][2];
	}
	map->decoration_count = 6U;
}

void vasal_client_demo_make(vasal_engine_map_s* const map) {
	(void)vasal_engine_map_clear(map, vasal_client_demo_side,
		vasal_client_demo_side);
	(void)strcpy(map->name, "demonstration");
	vasal_engine_map_standard_terrains(map);

	_vasal_client_demo_patch(map, 90, 60, 22, 14, _vasal_client_demo_tall);
	_vasal_client_demo_patch(map, 70, 180, 18, 24, _vasal_client_demo_short);
	_vasal_client_demo_patch(map, 180, 70, 30, 20, _vasal_client_demo_short);
	_vasal_client_demo_patch(map, 50, 80, 14, 10, _vasal_client_demo_forest);
	_vasal_client_demo_patch(map, 100, 160, 12, 18, _vasal_client_demo_forest);
	_vasal_client_demo_patch(map, 200, 190, 26, 16, _vasal_client_demo_forest);
	_vasal_client_demo_patch(map, 170, 140, 10, 8, _vasal_client_demo_tall);

	// The river winds north to south; the road crosses it at a ford.
	for (int32_t y = 0; y < (int32_t)vasal_client_demo_side; y++) {
		const int32_t bend = ((y % 64) < 32) ? (y % 32) : (32 - (y % 32));
		const int32_t x = 120 + (bend / 2);
		for (int32_t w = 0; w < 5; w++) {
			_vasal_client_demo_set(map, x + w, y, _vasal_client_demo_water);
		}
	}
	for (int32_t x = 0; x < (int32_t)vasal_client_demo_side; x++) {
		for (int32_t w = 0; w < 3; w++) {
			_vasal_client_demo_set(map, x, 122 + w, _vasal_client_demo_road);
		}
	}

	_vasal_client_demo_settlements(map);

	vasal_engine_map_spawn_s spawn;
	spawn.position.x = vasal_client_demo_spawn_x;
	spawn.position.y = vasal_client_demo_spawn_y;
	spawn.radius = 6U;
	map->spawns[0] = spawn;
	map->spawn_count = 1U;
}
