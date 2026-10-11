
/// Selection, focus and orders on top of the simulated world.

#include "client/game.h"

#include <math.h>

// Troops of the starting centaine per vintaine, a placeholder until the
// starting army is decided; the last vintaine starts empty, so the rules of
// empty vintaines can be tried.
static const uint8_t _vasal_client_game_start_troops
	[vasal_engine_world_groups] = { 20U, 20U, 16U, 12U, 0U, 1U };

// Troop speeds in 1/256 of a tile per tick (placeholders until tuning):
// about 1.6 tiles a second, a little faster for some.
static const uint16_t _vasal_client_game_speeds[3] = { 20U, 22U, 26U };

static vasal_engine_world_s _vasal_client_game_world;

// Remembers every vasal's positions before a tick moves them.
static void _vasal_client_game_remember(vasal_client_game_s* const game) {
	const vasal_engine_world_s* const world = game->world;
	for (size_t v = 0U; v < world->vasal_count; v++) {
		for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
			const vasal_engine_world_group_s* const group =
				&world->vasals[v].groups[g];
			game->previous_anchors[v][g] = group->anchor;
			for (size_t t = 0U; t < group->troop_count; t++) {
				game->previous_troops[v][g][t] = group->troops[t].position;
			}
		}
	}
}

// A point between two positions, in tiles, by the part of the tick passed.
static void _vasal_client_game_between(const vasal_client_game_s* const game,
		const vasal_engine_world_position_s before,
		const vasal_engine_world_position_s now, float* const x,
		float* const y) {
	const float part = fminf(game->pending
		* (float)vasal_engine_world_tick_rate, 1.0f);
	const float precision = (float)vasal_engine_map_precision;
	*x = ((float)before.x + (((float)now.x - (float)before.x) * part))
		/ precision;
	*y = ((float)before.y + (((float)now.y - (float)before.y) * part))
		/ precision;
}

void vasal_client_game_position_at(const vasal_client_game_s* const game,
	const size_t vasal, const size_t group, const size_t troop,
	float* const x, float* const y) {
	_vasal_client_game_between(game,
		game->previous_troops[vasal][group][troop],
		game->world->vasals[vasal].groups[group].troops[troop].position, x,
		y);
}

void vasal_client_game_troop_at(const vasal_client_game_s* const game,
	const size_t group, const size_t troop, float* const x, float* const y) {
	vasal_client_game_position_at(game, game->vasal, group, troop, x, y);
}

void vasal_client_game_anchor_at(const vasal_client_game_s* const game,
	const size_t group, float* const x, float* const y) {
	_vasal_client_game_between(game,
		game->previous_anchors[game->vasal][group],
		vasal_client_game_group(game, group)->anchor, x, y);
}

void vasal_client_game_start(vasal_client_game_s* const game,
	const vasal_engine_map_s* const map) {
	game->world = &_vasal_client_game_world;
	vasal_engine_world_start(game->world, map, NULL, 1U);
	vasal_engine_map_point_s spawn = { 0U, 0U };
	if (0U < map->spawn_count) {
		spawn = map->spawns[0].position;
	}
	game->vasal = vasal_engine_world_add_vasal(game->world, spawn);
	for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
		for (size_t t = 0U; t < _vasal_client_game_start_troops[g]; t++) {
			(void)vasal_engine_world_add_troop(game->world, game->vasal, g,
				_vasal_client_game_speeds[t % 3U]);
		}
	}
	game->selection = 1U;
	game->focus = 0U;
	game->action = vasal_client_game_action_move;
	game->notice = NULL;
	game->notice_seconds = 0.0f;
	game->pending = 0.0f;
	_vasal_client_game_remember(game);
}

const vasal_engine_world_group_s* vasal_client_game_group(
	const vasal_client_game_s* const game, const size_t group) {
	return &game->world->vasals[game->vasal].groups[group];
}

static bool_t _vasal_client_game_selected(
		const vasal_client_game_s* const game, const size_t group) {
	return 0U != (game->selection & ((uint32_t)1U << group));
}

// Where the focus goes when its group can't keep it: the next selected
// group; when none is selected, it stays if its group has troops, or else
// goes to the first group with troops. This rule is likely to change, so
// it lives here alone.
// spec: vintaines.focus#4 vintaines.focus#5 vintaines.focus#6
// spec: vintaines.select#7
static void _vasal_client_game_refocus(vasal_client_game_s* const game) {
	for (size_t step = 1U; step <= vasal_engine_world_groups; step++) {
		const size_t group = (game->focus + step) % vasal_engine_world_groups;
		if (_vasal_client_game_selected(game, group)) {
			game->focus = group;
			return;
		}
	}
	if (0U < vasal_client_game_group(game, game->focus)->troop_count) {
		return;
	}
	for (size_t group = 0U; group < vasal_engine_world_groups; group++) {
		if (0U < vasal_client_game_group(game, group)->troop_count) {
			game->focus = group;
			return;
		}
	}
}

void vasal_client_game_update(vasal_client_game_s* const game,
	const float seconds) {
	const float tick = 1.0f / (float)vasal_engine_world_tick_rate;
	game->pending += seconds;
	// Never more than a second at once, so a hidden tab does not stall.
	if (game->pending > 1.0f) {
		game->pending = 1.0f;
	}
	while (game->pending >= tick) {
		_vasal_client_game_remember(game);
		vasal_engine_world_tick(game->world);
		game->pending -= tick;
	}
	for (size_t group = 0U; group < vasal_engine_world_groups; group++) {
		if (0U == vasal_client_game_group(game, group)->troop_count) {
			game->selection &= ~((uint32_t)1U << group);
		}
	}
	if (0U == vasal_client_game_group(game, game->focus)->troop_count) {
		_vasal_client_game_refocus(game);
	}
	if (game->notice_seconds > 0.0f) {
		game->notice_seconds -= seconds;
	}
}

// spec: vintaines.select#1 vintaines.select#2 vintaines.select#4
// spec: vintaines.select#5 vintaines.focus#3
bool_t vasal_client_game_select(vasal_client_game_s* const game,
	const size_t group, const bool_t adding) {
	if (0U == vasal_client_game_group(game, group)->troop_count) {
		return false;
	}
	const uint32_t bit = (uint32_t)1U << (uint32_t)group;
	if (!adding) {
		game->selection = bit;
		game->focus = group;
	} else if (0U == (game->selection & bit)) {
		game->selection |= bit;
		game->focus = group;
	} else {
		game->selection &= ~bit;
		if (game->focus == group) {
			_vasal_client_game_refocus(game);
		}
	}
	return true;
}

// spec: map.sight#1 map.terrain#9
float vasal_client_game_eye(const vasal_client_game_s* const game,
	const size_t group, const size_t troop, float* const x, float* const y) {
	vasal_client_game_troop_at(game, group, troop, x, y);
	const vasal_engine_map_s* const map = game->world->map;
	const float tile_x = fminf(fmaxf(*x, 0.0f), (float)map->width - 1.0f);
	const float tile_y = fminf(fmaxf(*y, 0.0f), (float)map->height - 1.0f);
	const vasal_engine_map_terrain_s* const terrain = vasal_engine_map_terrain(
		map, (uint16_t)tile_x, (uint16_t)tile_y);
	return vasal_client_game_sight * (float)terrain->sight / 100.0f;
}

// spec: map.sight#1 map.sight#2 map.terrain#9
float vasal_client_game_sight_at(const vasal_client_game_s* const game,
	const float x, const float y) {
	float seen = 0.0f;
	for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
		const vasal_engine_world_group_s* const group =
			vasal_client_game_group(game, g);
		for (size_t t = 0U; t < group->troop_count; t++) {
			float tx = 0.0f;
			float ty = 0.0f;
			const float radius = vasal_client_game_eye(game, g, t, &tx, &ty);
			const float dx = tx - x;
			const float dy = ty - y;
			seen = fmaxf(seen, radius - sqrtf((dx * dx) + (dy * dy)));
		}
	}
	return fminf(fmaxf(seen, 0.0f), 1.0f);
}

// spec: vintaines.select#8
void vasal_client_game_select_all(vasal_client_game_s* const game,
	const size_t focus) {
	game->selection = 0U;
	for (size_t group = 0U; group < vasal_engine_world_groups; group++) {
		if (0U < vasal_client_game_group(game, group)->troop_count) {
			game->selection |= (uint32_t)1U << (uint32_t)group;
		}
	}
	if (0U < vasal_client_game_group(game, focus)->troop_count) {
		game->focus = focus;
	}
}

// spec: vintaines.focus#1 vintaines.focus#2
void vasal_client_game_cycle_focus(vasal_client_game_s* const game) {
	for (size_t step = 1U; step < vasal_engine_world_groups; step++) {
		const size_t group = (game->focus + step) % vasal_engine_world_groups;
		if (_vasal_client_game_selected(game, group)) {
			game->focus = group;
			return;
		}
	}
}

// spec: vintaines.pace#4 vintaines.queue#8
void vasal_client_game_toggle_pace(vasal_client_game_s* const game) {
	if (0U == game->selection) {
		vasal_client_game_notify(game, "Select a vintaine first.");
		return;
	}
	bool_t in_step = true;
	for (size_t group = 0U; group < vasal_engine_world_groups; group++) {
		if (_vasal_client_game_selected(game, group)
				&& (vasal_engine_world_pace_in_step
					!= vasal_client_game_group(game, group)->pace)) {
			in_step = false;
		}
	}
	vasal_engine_world_set_pace(game->world, game->vasal, game->selection,
		in_step ? vasal_engine_world_pace_at_the_run
			: vasal_engine_world_pace_in_step);
}

void vasal_client_game_notify(vasal_client_game_s* const game,
	const char* const text) {
	game->notice = text;
	game->notice_seconds = 3.0f;
}

// spec: map.space#3 vintaines.orders#2 vintaines.queue#6
void vasal_client_game_order(vasal_client_game_s* const game,
	const int32_t x, const int32_t y,
	const vasal_client_game_action_e action, const bool_t queued) {
	if (0U == game->selection) {
		vasal_client_game_notify(game, "Select a vintaine first.");
		return;
	}
	// Every action is a move for now.
	(void)action;
	const vasal_engine_map_s* const map = game->world->map;
	const bool_t inside = (x >= 0) && (y >= 0) && (x < (int32_t)map->width)
		&& (y < (int32_t)map->height);
	const vasal_engine_map_point_s to = {
		inside ? (uint16_t)x : 0U, inside ? (uint16_t)y : 0U
	};
	if (!inside) {
		vasal_client_game_notify(game, "That's outside the map.");
	} else if (queued && vasal_engine_world_queue_full(game->world,
			game->vasal, game->selection)) {
		vasal_client_game_notify(game, "No more orders can be queued.");
	} else if (queued) {
		(void)vasal_engine_world_queue_move(game->world, game->vasal,
			game->selection, to);
	} else {
		(void)vasal_engine_world_order_move(game->world, game->vasal,
			game->selection, to);
	}
}
