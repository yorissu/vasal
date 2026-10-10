
/// Tests of the player's side of the game: selection, focus, sight and
/// orders. The starting centaine has troops in vintaines 1 to 4 and the
/// centenar; vintaine 5 (group 4) is empty.

#include "client/game.h"
#include "client/keys.h"

#include "cairo.h"

static vasal_engine_map_s _vasal_tests_client_game_map;
static vasal_client_game_s _vasal_tests_client_game_game;

// A 200 by 200 map of grass with a spawn in the middle, and a game on it.
static vasal_client_game_s* _vasal_tests_client_game_start(void) {
	vasal_engine_map_s* const map = &_vasal_tests_client_game_map;
	(void)vasal_engine_map_clear(map, 200U, 200U);
	const vasal_engine_map_spawn_s spawn = { { 100U, 100U }, 5U };
	map->spawns[0] = spawn;
	map->spawn_count = 1U;
	vasal_client_game_start(&_vasal_tests_client_game_game, map);
	return &_vasal_tests_client_game_game;
}

static uint32_t _vasal_tests_client_game_bit(const size_t group) {
	return (uint32_t)1U << (uint32_t)group;
}

// Selects vintaines 1, 3 and 4 (groups 0, 2 and 3), the focused one last.
static void _vasal_tests_client_game_three(vasal_client_game_s* const game,
		const size_t focus) {
	static const size_t groups[3] = { 0U, 2U, 3U };
	bool_t first = true;
	for (size_t i = 0U; i < 3U; i++) {
		if (groups[i] != focus) {
			(void)vasal_client_game_select(game, groups[i], !first);
			first = false;
		}
	}
	(void)vasal_client_game_select(game, focus, true);
}

// spec: vintaines.select#1
cairo_test_new(client_game, key_selects_one_and_focuses_it) {
	vasal_client_game_s* const game = _vasal_tests_client_game_start();
	(void)vasal_client_game_select(game, 0U, true);
	cairo_assert_true(vasal_client_game_select(game, 2U, false));
	cairo_assert_eq(game->selection, _vasal_tests_client_game_bit(2U));
	cairo_assert_eq(game->focus, 2U);
}

// spec: vintaines.select#2 vintaines.focus#3
cairo_test_new(client_game, shift_toggles_and_focuses_the_added) {
	vasal_client_game_s* const game = _vasal_tests_client_game_start();
	(void)vasal_client_game_select(game, 0U, false);
	(void)vasal_client_game_select(game, 3U, true);
	cairo_assert_eq(game->selection, _vasal_tests_client_game_bit(0U)
		| _vasal_tests_client_game_bit(3U));
	cairo_assert_eq(game->focus, 3U);
	(void)vasal_client_game_select(game, 3U, true);
	cairo_assert_eq(game->selection, _vasal_tests_client_game_bit(0U));
	cairo_assert_eq(game->focus, 0U);
}

// spec: vintaines.select#3
cairo_test_new(client_game, selecting_keeps_orders) {
	vasal_client_game_s* const game = _vasal_tests_client_game_start();
	(void)vasal_client_game_select(game, 0U, false);
	vasal_client_game_order(game, 150, 100, vasal_client_game_action_move,
		false);
	vasal_client_game_update(game, 0.5f);
	const vasal_engine_world_group_s* const group =
		vasal_client_game_group(game, 0U);
	const uint16_t count = group->waypoint_count;
	const uint16_t next = group->waypoint_next;
	cairo_assert_true(group->moving);
	(void)vasal_client_game_select(game, 1U, false);
	(void)vasal_client_game_select(game, 0U, true);
	(void)vasal_client_game_select(game, 0U, true);
	cairo_assert_true(group->moving);
	cairo_assert_eq(group->waypoint_count, count);
	cairo_assert_eq(group->waypoint_next, next);
}

// spec: vintaines.select#4
cairo_test_new(client_game, empty_vintaine_is_not_selected) {
	vasal_client_game_s* const game = _vasal_tests_client_game_start();
	(void)vasal_client_game_select(game, 1U, false);
	cairo_assert_false(vasal_client_game_select(game, 4U, false));
	cairo_assert_false(vasal_client_game_select(game, 4U, true));
	cairo_assert_eq(game->selection, _vasal_tests_client_game_bit(1U));
	cairo_assert_eq(game->focus, 1U);
}

// spec: vintaines.select#5
cairo_test_new(client_game, centenar_is_selected_like_a_vintaine) {
	vasal_client_game_s* const game = _vasal_tests_client_game_start();
	cairo_assert_true(vasal_client_game_select(game,
		vasal_engine_world_centenar, false));
	cairo_assert_eq(game->selection,
		_vasal_tests_client_game_bit(vasal_engine_world_centenar));
	cairo_assert_eq(game->focus, vasal_engine_world_centenar);
}

// spec: vintaines.select#7
cairo_test_new(client_game, unselecting_focused_moves_focus) {
	vasal_client_game_s* const game = _vasal_tests_client_game_start();
	(void)vasal_client_game_select(game, 0U, false);
	(void)vasal_client_game_select(game, 2U, true);
	(void)vasal_client_game_select(game, 2U, true);
	cairo_assert_eq(game->selection, _vasal_tests_client_game_bit(0U));
	cairo_assert_eq(game->focus, 0U);
}

// spec: vintaines.select#8
cairo_test_new(client_game, double_tap_selects_all) {
	vasal_client_game_s* const game = _vasal_tests_client_game_start();
	vasal_client_keys_s keys;
	vasal_client_keys_start(&keys);
	vasal_client_keys_group(&keys, game, 2U, false);
	vasal_client_keys_wait(&keys, 0.1f);
	vasal_client_keys_group(&keys, game, 2U, false);
	cairo_assert_eq(game->selection, _vasal_tests_client_game_bit(0U)
		| _vasal_tests_client_game_bit(1U) | _vasal_tests_client_game_bit(2U)
		| _vasal_tests_client_game_bit(3U)
		| _vasal_tests_client_game_bit(vasal_engine_world_centenar));
	cairo_assert_eq(game->focus, 2U);
}

// spec: vintaines.select#8
cairo_test_new(client_game, slow_second_press_selects_one) {
	vasal_client_game_s* const game = _vasal_tests_client_game_start();
	vasal_client_keys_s keys;
	vasal_client_keys_start(&keys);
	vasal_client_keys_group(&keys, game, 2U, false);
	vasal_client_keys_wait(&keys, 1.0f);
	vasal_client_keys_group(&keys, game, 2U, false);
	cairo_assert_eq(game->selection, _vasal_tests_client_game_bit(2U));
	vasal_client_keys_group(&keys, game, 1U, false);
	cairo_assert_eq(game->selection, _vasal_tests_client_game_bit(1U));
}

// spec: vintaines.select#2
cairo_test_new(client_game, shift_twice_toggles_back) {
	vasal_client_game_s* const game = _vasal_tests_client_game_start();
	vasal_client_keys_s keys;
	vasal_client_keys_start(&keys);
	vasal_client_keys_group(&keys, game, 0U, false);
	vasal_client_keys_group(&keys, game, 3U, true);
	vasal_client_keys_group(&keys, game, 3U, true);
	cairo_assert_eq(game->selection, _vasal_tests_client_game_bit(0U));
}

// spec: vintaines.focus#1
cairo_test_new(client_game, tab_focuses_next_selected) {
	vasal_client_game_s* const game = _vasal_tests_client_game_start();
	_vasal_tests_client_game_three(game, 2U);
	const uint32_t selection = game->selection;
	vasal_client_game_cycle_focus(game);
	cairo_assert_eq(game->focus, 3U);
	cairo_assert_eq(game->selection, selection);
}

// spec: vintaines.focus#2
cairo_test_new(client_game, tab_wraps_around) {
	vasal_client_game_s* const game = _vasal_tests_client_game_start();
	_vasal_tests_client_game_three(game, 3U);
	vasal_client_game_cycle_focus(game);
	cairo_assert_eq(game->focus, 0U);
}

// spec: vintaines.focus#4
cairo_test_new(client_game, emptied_focus_moves_to_next_selected) {
	vasal_client_game_s* const game = _vasal_tests_client_game_start();
	_vasal_tests_client_game_three(game, 2U);
	game->world->vasals[game->vasal].groups[2].troop_count = 0U;
	vasal_client_game_update(game, 0.0f);
	cairo_assert_eq(game->selection, _vasal_tests_client_game_bit(0U)
		| _vasal_tests_client_game_bit(3U));
	cairo_assert_eq(game->focus, 3U);
}

// spec: vintaines.focus#4
cairo_test_new(client_game, emptied_focus_without_selection) {
	vasal_client_game_s* const game = _vasal_tests_client_game_start();
	(void)vasal_client_game_select(game, 0U, false);
	game->world->vasals[game->vasal].groups[0].troop_count = 0U;
	vasal_client_game_update(game, 0.0f);
	cairo_assert_eq(game->selection, 0U);
	cairo_assert_eq(game->focus, 1U);
	for (size_t g = 0U; g < vasal_engine_world_centenar; g++) {
		game->world->vasals[game->vasal].groups[g].troop_count = 0U;
	}
	vasal_client_game_update(game, 0.0f);
	cairo_assert_eq(game->focus, vasal_engine_world_centenar);
}

// spec: vintaines.focus#5
cairo_test_new(client_game, unselecting_all_keeps_focus) {
	vasal_client_game_s* const game = _vasal_tests_client_game_start();
	(void)vasal_client_game_select(game, 2U, false);
	(void)vasal_client_game_select(game, 2U, true);
	cairo_assert_eq(game->selection, 0U);
	cairo_assert_eq(game->focus, 2U);
}

// spec: vintaines.orders#2
cairo_test_new(client_game, right_click_orders_current_action) {
	vasal_client_game_s* const game = _vasal_tests_client_game_start();
	cairo_assert_eq(game->action, vasal_client_game_action_move);
	(void)vasal_client_game_select(game, 0U, false);
	(void)vasal_client_game_select(game, 1U, true);
	vasal_client_game_order(game, 120, 90, game->action, false);
	cairo_assert_true(vasal_client_game_group(game, 0U)->moving);
	cairo_assert_true(vasal_client_game_group(game, 1U)->moving);
	cairo_assert_false(vasal_client_game_group(game, 2U)->moving);
	cairo_assert_true(NULL == game->notice);
}

// spec: map.space#3
cairo_test_new(client_game, order_off_the_map_gets_notice) {
	vasal_client_game_s* const game = _vasal_tests_client_game_start();
	(void)vasal_client_game_select(game, 0U, false);
	vasal_client_game_order(game, -3, 50, vasal_client_game_action_move,
		false);
	cairo_assert_false(vasal_client_game_group(game, 0U)->moving);
	cairo_assert_true(NULL != game->notice);
	cairo_assert_gt(game->notice_seconds, 0.0f);
}

// spec: map.sight#2
cairo_test_new(client_game, sight_inside_and_outside) {
	vasal_client_game_s* const game = _vasal_tests_client_game_start();
	cairo_assert_nreq(vasal_client_game_sight_at(game, 100.0f, 100.0f),
		1.0f);
	cairo_assert_nreq(vasal_client_game_sight_at(game, 100.0f,
		100.0f + vasal_client_game_sight + 2.0f), 0.0f);
	cairo_assert_nreq(vasal_client_game_sight_at(game, 10.0f, 10.0f), 0.0f);
}

// spec: map.sight#2
cairo_test_new(client_game, sight_fades_over_the_last_tile) {
	vasal_client_game_s* const game = _vasal_tests_client_game_start();
	float x = 0.0f;
	float y = 0.0f;
	vasal_client_game_troop_at(game, 0U, 0U, &x, &y);
	const float edge = vasal_client_game_sight_at(game,
		x + vasal_client_game_sight - 0.5f, y);
	cairo_assert_nreq(edge, 0.5f);
}

// spec: map.sight#1
cairo_test_new(client_game, sight_is_the_union_of_troops) {
	vasal_client_game_s* const game = _vasal_tests_client_game_start();
	vasal_engine_world_group_s* const far =
		&game->world->vasals[game->vasal].groups[1];
	const int32_t precision = (int32_t)vasal_engine_map_precision;
	for (size_t t = 0U; t < far->troop_count; t++) {
		far->troops[t].position.x = 160 * precision;
		game->previous_troops[game->vasal][1][t] = far->troops[t].position;
	}
	cairo_assert_nreq(vasal_client_game_sight_at(game, 100.0f, 100.0f),
		1.0f);
	cairo_assert_nreq(vasal_client_game_sight_at(game, 160.0f, 100.5f),
		1.0f);
	cairo_assert_nreq(vasal_client_game_sight_at(game, 130.0f, 100.0f),
		0.0f);
}

// spec: vintaines.queue#2 vintaines.queue#6
cairo_test_new(client_game, shift_queues_until_full) {
	vasal_client_game_s* const game = _vasal_tests_client_game_start();
	(void)vasal_client_game_select(game, 0U, false);
	vasal_client_game_order(game, 150, 100, vasal_client_game_action_move,
		false);
	for (size_t i = 0U; i < vasal_engine_world_queue_max; i++) {
		vasal_client_game_order(game, 150, 150,
			vasal_client_game_action_move, true);
	}
	cairo_assert_true(NULL == game->notice);
	cairo_assert_eq(vasal_client_game_group(game, 0U)->queue_count,
		vasal_engine_world_queue_max);
	vasal_client_game_order(game, 150, 150, vasal_client_game_action_move,
		true);
	cairo_assert_true(NULL != game->notice);
	vasal_client_game_order(game, 120, 100, vasal_client_game_action_move,
		false);
	cairo_assert_eq(vasal_client_game_group(game, 0U)->queue_count, 0U);
}

// spec: vintaines.queue#4
cairo_test_new(client_game, queue_stays_with_its_vintaine) {
	vasal_client_game_s* const game = _vasal_tests_client_game_start();
	vasal_client_keys_s keys;
	vasal_client_keys_start(&keys);
	vasal_client_keys_group(&keys, game, 0U, false);
	vasal_client_game_order(game, 150, 100, vasal_client_game_action_move,
		false);
	vasal_client_game_order(game, 150, 150, vasal_client_game_action_move,
		true);
	vasal_client_game_order(game, 100, 150, vasal_client_game_action_move,
		true);
	vasal_client_keys_wait(&keys, 1.0f);
	vasal_client_keys_group(&keys, game, 1U, false);
	cairo_assert_eq(game->selection, _vasal_tests_client_game_bit(1U));
	vasal_client_game_order(game, 50, 50, vasal_client_game_action_move,
		false);
	vasal_client_game_order(game, 60, 50, vasal_client_game_action_move,
		true);
	cairo_assert_eq(vasal_client_game_group(game, 0U)->queue_count, 2U);
	cairo_assert_eq(vasal_client_game_group(game, 0U)->queue[1].x, 100U);
	cairo_assert_eq(vasal_client_game_group(game, 1U)->queue_count, 1U);
	for (size_t i = 0U; i < 6000U; i++) {
		vasal_client_game_update(game, 0.05f);
	}
	float x = 0.0f;
	float y = 0.0f;
	vasal_client_game_anchor_at(game, 0U, &x, &y);
	cairo_assert_nr(x, 100.5f, 0.6f);
	cairo_assert_nr(y, 150.5f, 0.6f);
	vasal_client_game_anchor_at(game, 1U, &x, &y);
	cairo_assert_nr(x, 60.5f, 0.6f);
	cairo_assert_nr(y, 50.5f, 0.6f);
}

// spec: vintaines.queue#9
cairo_test_new(client_game, shift_held_selects_and_queues) {
	vasal_client_game_s* const game = _vasal_tests_client_game_start();
	vasal_client_keys_s keys;
	vasal_client_keys_start(&keys);
	(void)vasal_client_game_select(game, 3U, false);
	vasal_client_keys_group(&keys, game, 3U, true);
	vasal_client_keys_group(&keys, game, 0U, true);
	cairo_assert_eq(game->selection, _vasal_tests_client_game_bit(0U));
	vasal_client_game_order(game, 150, 100, vasal_client_game_action_move,
		true);
	vasal_client_game_order(game, 150, 150, vasal_client_game_action_move,
		true);
	vasal_client_keys_wait(&keys, 1.0f);
	vasal_client_keys_group(&keys, game, 1U, true);
	cairo_assert_eq(game->selection, _vasal_tests_client_game_bit(0U)
		| _vasal_tests_client_game_bit(1U));
	for (size_t i = 0U; i < 3U; i++) {
		vasal_client_game_order(game, 100 + (int32_t)(10U * i), 50,
			vasal_client_game_action_move, true);
	}
	const vasal_engine_world_group_s* const first =
		vasal_client_game_group(game, 0U);
	const vasal_engine_world_group_s* const second =
		vasal_client_game_group(game, 1U);
	cairo_assert_true(first->moving);
	cairo_assert_eq(first->queue_count, 4U);
	cairo_assert_true(second->moving);
	cairo_assert_eq(second->queue_count, 2U);
}

// spec: vintaines.pace#4 vintaines.queue#8
cairo_test_new(client_game, z_switches_the_pace_of_the_selection) {
	vasal_client_game_s* const game = _vasal_tests_client_game_start();
	const vasal_engine_world_group_s* const first =
		vasal_client_game_group(game, 0U);
	const vasal_engine_world_group_s* const second =
		vasal_client_game_group(game, 1U);
	(void)vasal_client_game_select(game, 0U, false);
	vasal_client_game_order(game, 150, 100, vasal_client_game_action_move,
		false);
	vasal_client_game_order(game, 150, 150, vasal_client_game_action_move,
		true);
	vasal_client_game_toggle_pace(game);
	cairo_assert_eq(first->pace, vasal_engine_world_pace_at_the_run);
	cairo_assert_eq(second->pace, vasal_engine_world_pace_in_step);
	// The order and its queue stay.
	cairo_assert_true(first->moving);
	cairo_assert_eq(first->queue_count, 1U);
	// One at the run and one in step: both march in step.
	(void)vasal_client_game_select(game, 1U, true);
	vasal_client_game_toggle_pace(game);
	cairo_assert_eq(first->pace, vasal_engine_world_pace_in_step);
	cairo_assert_eq(second->pace, vasal_engine_world_pace_in_step);
	vasal_client_game_toggle_pace(game);
	cairo_assert_eq(first->pace, vasal_engine_world_pace_at_the_run);
	cairo_assert_eq(second->pace, vasal_engine_world_pace_at_the_run);
}

// spec: vintaines.pace#4
cairo_test_new(client_game, z_without_a_selection_gets_a_notice) {
	vasal_client_game_s* const game = _vasal_tests_client_game_start();
	game->selection = 0U;
	vasal_client_game_toggle_pace(game);
	cairo_assert_gt(game->notice_seconds, 0.0f);
}
