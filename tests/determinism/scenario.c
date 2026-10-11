
/// A fixed scenario for comparing builds: several vasals march across a
/// map of water, slow ground and roads, with orders and pace changes along
/// the way. Every 20 ticks it prints the tick and a checksum of the whole
/// world, so the native build and the WebAssembly build can be compared
/// line by line.

#include "engine/bytes.h"
#include "engine/crc.h"
#include "engine/map.h"
#include "engine/world.h"

#include <stdio.h>

static vasal_engine_map_s _vasal_tests_determinism_map;
static vasal_engine_world_s _vasal_tests_determinism_world;
static uint8_t _vasal_tests_determinism_state[1U << 18];

// 120 by 120 tiles: grass, a lake with one gap, a band of tall grass and
// a road across it.
static void _vasal_tests_determinism_map_make(
		vasal_engine_map_s* const map) {
	(void)vasal_engine_map_clear(map, 120U, 120U);
	const vasal_engine_map_terrain_s water = {
		.name = "water", .color = 0U, .speed = 0U, .passable = false
	};
	const vasal_engine_map_terrain_s tall_grass = {
		.name = "tall grass", .color = 0U, .speed = 45U, .passable = true
	};
	const vasal_engine_map_terrain_s road = {
		.name = "road", .color = 0U, .speed = 100U, .passable = true
	};
	map->terrains[0].speed = 80U;
	map->terrains[1] = water;
	map->terrains[2] = tall_grass;
	map->terrains[3] = road;
	map->terrain_count = 4U;
	for (size_t y = 0U; y < 120U; y++) {
		for (size_t x = 0U; x < 120U; x++) {
			uint8_t terrain = 0U;
			if ((x >= 50U) && (x < 60U) && ((y < 70U) || (y > 75U))) {
				terrain = 1U;
			} else if ((y >= 30U) && (y < 45U)) {
				terrain = 2U;
			} else {
				terrain = 0U;
			}
			if (x == 20U) {
				terrain = 3U;
			}
			map->tiles[(y * 120U) + x] = terrain;
		}
	}
}

// The world's state, in id order, as bytes, and its checksum.
static uint32_t _vasal_tests_determinism_checksum(
		const vasal_engine_world_s* const world) {
	vasal_engine_bytes_writer_s writer = vasal_engine_bytes_writer(
		_vasal_tests_determinism_state,
		sizeof(_vasal_tests_determinism_state));
	vasal_engine_bytes_write_u32(&writer, world->tick);
	for (size_t v = 0U; v < world->vasal_count; v++) {
		for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
			const vasal_engine_world_group_s* const group =
				&world->vasals[v].groups[g];
			vasal_engine_bytes_write_u32(&writer, (uint32_t)group->anchor.x);
			vasal_engine_bytes_write_u32(&writer, (uint32_t)group->anchor.y);
			vasal_engine_bytes_write_u16(&writer, group->waypoint_next);
			vasal_engine_bytes_write_u8(&writer, group->moving ? 1U : 0U);
			vasal_engine_bytes_write_u8(&writer, group->queue_count);
			for (size_t t = 0U; t < group->troop_count; t++) {
				const vasal_engine_world_troop_s* const troop =
					&group->troops[t];
				vasal_engine_bytes_write_u32(&writer, troop->id);
				vasal_engine_bytes_write_u32(&writer,
					(uint32_t)troop->position.x);
				vasal_engine_bytes_write_u32(&writer,
					(uint32_t)troop->position.y);
			}
		}
	}
	return vasal_engine_crc_compute(_vasal_tests_determinism_state,
		writer.size);
}

// Gives the orders of a tick, if it has any.
static void _vasal_tests_determinism_orders(
		vasal_engine_world_s* const world, const uint32_t tick) {
	const vasal_engine_map_point_s across = { 100U, 100U };
	const vasal_engine_map_point_s north = { 30U, 5U };
	const vasal_engine_map_point_s road = { 20U, 110U };
	const vasal_engine_map_point_s lake = { 55U, 20U };
	switch (tick) {
	case 1U:
		(void)vasal_engine_world_order_move(world, 0U, 0x3fU, across);
		(void)vasal_engine_world_order_move(world, 1U, 0x07U, road);
		(void)vasal_engine_world_order_move(world, 2U, 0x01U, lake);
		(void)vasal_engine_world_queue_move(world, 2U, 0x01U, road);
		(void)vasal_engine_world_queue_move(world, 2U, 0x01U, north);
		break;
	case 150U:
		vasal_engine_world_set_pace(world, 0U, 0x05U,
			vasal_engine_world_pace_at_the_run);
		(void)vasal_engine_world_order_move(world, 1U, 0x02U, north);
		break;
	case 400U:
		(void)vasal_engine_world_order_move(world, 2U, 0x1fU, across);
		vasal_engine_world_set_pace(world, 2U, 0x1fU,
			vasal_engine_world_pace_at_the_run);
		break;
	default:
		break;
	}
}

// spec: simulation.determinism#2
int main(void) {
	vasal_engine_map_s* const map = &_vasal_tests_determinism_map;
	vasal_engine_world_s* const world = &_vasal_tests_determinism_world;
	_vasal_tests_determinism_map_make(map);
	vasal_engine_world_start(world, map, NULL, 20261011U);
	static const uint16_t spawns[3][2] = {
		{ 10U, 10U }, { 90U, 15U }, { 30U, 90U }
	};
	for (size_t v = 0U; v < 3U; v++) {
		const vasal_engine_map_point_s spawn = { spawns[v][0], spawns[v][1] };
		const size_t vasal = vasal_engine_world_add_vasal(world, spawn);
		for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
			for (size_t t = 0U; t < (20U - (3U * g) - v); t++) {
				(void)vasal_engine_world_add_troop(world, vasal, g,
					(uint16_t)(14U + ((t * 7U) % 13U)));
			}
		}
	}
	for (uint32_t tick = 1U; tick <= 1200U; tick++) {
		_vasal_tests_determinism_orders(world, tick);
		vasal_engine_world_tick(world);
		if (0U == (tick % 20U)) {
			(void)printf("%u %08x\n", (unsigned int)tick,
				(unsigned int)_vasal_tests_determinism_checksum(world));
		}
	}
	return 0;
}
