
/// The demonstration map, drawn from simple shapes so it is the same every
/// time: a river running north to south with a ford where the road crosses
/// it, two forests, and patches of tall and short grass.

#include "client/demo.h"

#include <string.h>

enum {
	_vasal_client_demo_grass,
	_vasal_client_demo_water,
	_vasal_client_demo_forest,
	_vasal_client_demo_tall,
	_vasal_client_demo_short,
	_vasal_client_demo_road
};

static void _vasal_client_demo_terrain(vasal_engine_map_s* const map,
		const size_t index, const char* const name, const uint32_t color,
		const uint16_t speed) {
	vasal_engine_map_terrain_s* const terrain = &map->terrains[index];
	size_t length = strlen(name);
	if (length > (vasal_engine_map_name_capacity - 1U)) {
		length = vasal_engine_map_name_capacity - 1U;
	}
	for (size_t i = 0U; i < length; i++) {
		terrain->name[i] = name[i];
	}
	terrain->name[length] = '\0';
	terrain->color = color;
	terrain->speed = speed;
	terrain->passable = 0U < speed;
}

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

void vasal_client_demo_make(vasal_engine_map_s* const map) {
	(void)vasal_engine_map_clear(map, vasal_client_demo_side,
		vasal_client_demo_side);
	(void)strcpy(map->name, "demonstration");
	_vasal_client_demo_terrain(map, 0U, "grass", 0x6a9a3cffU, 100U);
	_vasal_client_demo_terrain(map, 1U, "water", 0x3d6fb0ffU, 0U);
	_vasal_client_demo_terrain(map, 2U, "forest", 0x2f5a2affU, 0U);
	_vasal_client_demo_terrain(map, 3U, "tall grass", 0x9cad45ffU, 45U);
	_vasal_client_demo_terrain(map, 4U, "short grass", 0x82a845ffU, 80U);
	_vasal_client_demo_terrain(map, 5U, "road", 0xb59a6bffU, 100U);
	map->terrain_count = 6U;

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

	const vasal_engine_map_spawn_s spawn = {
		{ vasal_client_demo_spawn_x, vasal_client_demo_spawn_y }, 6U
	};
	map->spawns[0] = spawn;
	map->spawn_count = 1U;
}
