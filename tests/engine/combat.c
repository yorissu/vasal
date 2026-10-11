
/// Tests of combat: duels, strikes, shots, wounds, modes and camps, with
/// the standard rules on open grass.

#include "engine/world.h"

#include "cairo.h"

static vasal_engine_map_s _vasal_tests_engine_combat_map;
static vasal_engine_world_s _vasal_tests_engine_combat_world;
static vasal_engine_world_s _vasal_tests_engine_combat_twin;
static vasal_engine_rules_s _vasal_tests_engine_combat_rules;

// A 80 by 80 map of grass, the standard rules (which tests may change
// first), and a world with two vasals at (20, 40) and (x, 40).
static vasal_engine_world_s* _vasal_tests_engine_combat_start(
		vasal_engine_world_s* const world, const uint16_t x) {
	vasal_engine_map_s* const map = &_vasal_tests_engine_combat_map;
	(void)vasal_engine_map_clear(map, 80U, 80U);
	vasal_engine_world_start(world, map, &_vasal_tests_engine_combat_rules,
		99U);
	const vasal_engine_map_point_s a = { 20U, 40U };
	const vasal_engine_map_point_s b = { x, 40U };
	(void)vasal_engine_world_add_vasal(world, a);
	(void)vasal_engine_world_add_vasal(world, b);
	return world;
}

static void _vasal_tests_engine_combat_add(vasal_engine_world_s* const world,
		const size_t vasal, const size_t unit, const size_t count) {
	for (size_t i = 0U; i < count; i++) {
		(void)vasal_engine_world_add_unit(world, vasal, 0U, unit);
	}
}

static void _vasal_tests_engine_combat_run(vasal_engine_world_s* const world,
		const uint32_t ticks) {
	for (uint32_t i = 0U; i < ticks; i++) {
		vasal_engine_world_tick(world);
	}
}

static size_t _vasal_tests_engine_combat_duels(
		const vasal_engine_world_s* const world, const size_t vasal) {
	size_t count = 0U;
	const vasal_engine_world_group_s* const group =
		&world->vasals[vasal].groups[0];
	for (size_t t = 0U; t < group->troop_count; t++) {
		count += (0U != group->troops[t].duel) ? 1U : 0U;
	}
	return count;
}

// spec: combat.duel#1 combat.duel#5 vintaines.modes#1
cairo_test_new(engine_combat, aggressive_troops_attack_and_duel) {
	vasal_engine_rules_standard(&_vasal_tests_engine_combat_rules);
	vasal_engine_world_s* const world = _vasal_tests_engine_combat_start(
		&_vasal_tests_engine_combat_world, 30U);
	_vasal_tests_engine_combat_add(world, 0U, 0U, 10U);
	_vasal_tests_engine_combat_add(world, 1U, 1U, 10U);
	vasal_engine_world_set_mode(world, 0U, 1U,
		vasal_engine_world_mode_aggressive);
	bool_t dueled = false;
	for (uint32_t i = 0U; (i < 400U) && !dueled; i++) {
		vasal_engine_world_tick(world);
		dueled = 0U < _vasal_tests_engine_combat_duels(world, 0U);
	}
	cairo_assert_true(dueled);
	cairo_assert_true(world->vasals[0].groups[0].fighting);
	// A troop in a duel stands still.
	const vasal_engine_world_group_s* const group = &world->vasals[0].groups[0];
	size_t still = 0U;
	for (size_t t = 0U; t < group->troop_count; t++) {
		if (0U != group->troops[t].duel) {
			still = t;
		}
	}
	const vasal_engine_world_position_s before = group->troops[still].position;
	const uint32_t id = group->troops[still].id;
	vasal_engine_world_tick(world);
	if ((still < group->troop_count) && (id == group->troops[still].id)
			&& (0U != group->troops[still].duel)) {
		cairo_assert_eq(group->troops[still].position.x, before.x);
		cairo_assert_eq(group->troops[still].position.y, before.y);
	}
	// Fights end: one side or both lose troops.
	_vasal_tests_engine_combat_run(world, 2000U);
	cairo_assert_lt(world->vasals[0].groups[0].troop_count
		+ world->vasals[1].groups[0].troop_count, 20U);
	cairo_assert_gt(world->vasals[0].kills + world->vasals[1].kills, 0U);
}

// spec: combat.enemies#1
cairo_test_new(engine_combat, allies_never_fight) {
	vasal_engine_rules_standard(&_vasal_tests_engine_combat_rules);
	vasal_engine_world_s* const world = _vasal_tests_engine_combat_start(
		&_vasal_tests_engine_combat_world, 21U);
	world->vasals[1].alliance = world->vasals[0].alliance;
	_vasal_tests_engine_combat_add(world, 0U, 0U, 10U);
	_vasal_tests_engine_combat_add(world, 1U, 2U, 10U);
	vasal_engine_world_set_mode(world, 0U, 1U,
		vasal_engine_world_mode_aggressive);
	vasal_engine_world_set_mode(world, 1U, 1U,
		vasal_engine_world_mode_aggressive);
	for (uint32_t i = 0U; i < 300U; i++) {
		vasal_engine_world_tick(world);
		cairo_assert_eq(world->shot_count, 0U);
		cairo_assert_eq(_vasal_tests_engine_combat_duels(world, 0U), 0U);
	}
	cairo_assert_eq(world->vasals[0].groups[0].troop_count, 10U);
	cairo_assert_eq(world->vasals[1].groups[0].troop_count, 10U);
}

// spec: combat.duel#2
cairo_test_new(engine_combat, a_troop_is_in_one_duel_at_most) {
	vasal_engine_rules_standard(&_vasal_tests_engine_combat_rules);
	vasal_engine_world_s* const world = _vasal_tests_engine_combat_start(
		&_vasal_tests_engine_combat_world, 26U);
	_vasal_tests_engine_combat_add(world, 0U, 0U, 3U);
	_vasal_tests_engine_combat_add(world, 1U, 1U, 1U);
	vasal_engine_world_set_mode(world, 0U, 1U,
		vasal_engine_world_mode_aggressive);
	for (uint32_t i = 0U; i < 300U; i++) {
		vasal_engine_world_tick(world);
		cairo_assert_le(_vasal_tests_engine_combat_duels(world, 0U), 1U);
		cairo_assert_le(_vasal_tests_engine_combat_duels(world, 1U), 1U);
	}
}

// spec: combat.ranged#1 combat.ranged#2 combat.duel#4
cairo_test_new(engine_combat, archers_shoot_within_range_after_reloading) {
	vasal_engine_rules_standard(&_vasal_tests_engine_combat_rules);
	vasal_engine_world_s* const world = _vasal_tests_engine_combat_start(
		&_vasal_tests_engine_combat_world, 30U);
	_vasal_tests_engine_combat_add(world, 0U, 2U, 1U);
	_vasal_tests_engine_combat_add(world, 1U, 2U, 5U);
	vasal_engine_world_set_mode(world, 0U, 1U,
		vasal_engine_world_mode_aggressive);
	vasal_engine_world_set_mode(world, 1U, 1U,
		vasal_engine_world_mode_aggressive);
	const uint32_t reload = vasal_engine_rules_get(
		&_vasal_tests_engine_combat_rules, 2U, vasal_engine_rules_unit_reload);
	const vasal_engine_world_group_s* const lone = &world->vasals[0].groups[0];
	uint32_t shots = 0U;
	uint32_t last = 0U;
	for (uint32_t i = 1U; (i <= 600U) && (0U < lone->troop_count); i++) {
		const vasal_engine_world_position_s at = lone->troops[0].position;
		vasal_engine_world_tick(world);
		for (size_t s = 0U; s < world->shot_count; s++) {
			if ((world->shots[s].from.x == at.x)
					&& (world->shots[s].from.y == at.y)) {
				if (0U < shots) {
					cairo_assert_ge(i - last, reload);
				}
				shots++;
				last = i;
			}
		}
		// Ranged troops never start duels.
		cairo_assert_eq(_vasal_tests_engine_combat_duels(world, 0U), 0U);
		cairo_assert_eq(_vasal_tests_engine_combat_duels(world, 1U), 0U);
	}
	cairo_assert_gt(shots, 0U);
	cairo_assert_gt(world->vasals[0].kills + world->vasals[1].kills, 0U);
}

// spec: combat.wound#1 combat.strike#5
cairo_test_new(engine_combat, a_second_wound_kills) {
	vasal_engine_rules_standard(&_vasal_tests_engine_combat_rules);
	// Hits never kill outright: only second wounds do.
	for (size_t u = 0U; u < _vasal_tests_engine_combat_rules.unit_count; u++) {
		_vasal_tests_engine_combat_rules.units[u].values
			[vasal_engine_rules_unit_kill] = 0U;
	}
	vasal_engine_world_s* const world = _vasal_tests_engine_combat_start(
		&_vasal_tests_engine_combat_world, 28U);
	_vasal_tests_engine_combat_add(world, 0U, 0U, 6U);
	_vasal_tests_engine_combat_add(world, 1U, 0U, 6U);
	vasal_engine_world_set_mode(world, 0U, 1U,
		vasal_engine_world_mode_aggressive);
	vasal_engine_world_set_mode(world, 1U, 1U,
		vasal_engine_world_mode_aggressive);
	bool_t wounded = false;
	for (uint32_t i = 0U; i < 3000U; i++) {
		vasal_engine_world_tick(world);
		for (size_t v = 0U; v < 2U; v++) {
			const vasal_engine_world_group_s* const group =
				&world->vasals[v].groups[0];
			for (size_t t = 0U; t < group->troop_count; t++) {
				wounded = wounded || group->troops[t].wounded;
			}
		}
	}
	cairo_assert_true(wounded);
	cairo_assert_lt(world->vasals[0].groups[0].troop_count
		+ world->vasals[1].groups[0].troop_count, 12U);
}

// spec: vintaines.modes#4 vintaines.modes#8
cairo_test_new(engine_combat, defensive_vintaines_let_enemies_pass) {
	vasal_engine_rules_standard(&_vasal_tests_engine_combat_rules);
	vasal_engine_world_s* const world = _vasal_tests_engine_combat_start(
		&_vasal_tests_engine_combat_world, 60U);
	_vasal_tests_engine_combat_add(world, 0U, 0U, 5U);
	_vasal_tests_engine_combat_add(world, 1U, 0U, 5U);
	cairo_assert_eq(world->vasals[0].groups[0].mode,
		vasal_engine_world_mode_defensive);
	// The second marches past, three tiles away, both defensive.
	const vasal_engine_map_point_s past = { 5U, 44U };
	(void)vasal_engine_world_order_move(world, 1U, 1U, past);
	for (uint32_t i = 0U; i < 1500U; i++) {
		vasal_engine_world_tick(world);
		cairo_assert_false(world->vasals[0].groups[0].fighting);
	}
	cairo_assert_eq(world->vasals[0].groups[0].troop_count, 5U);
	cairo_assert_eq(world->vasals[1].groups[0].troop_count, 5U);
}

// spec: vintaines.modes#2 vintaines.modes#5 npcs.bandits#1
cairo_test_new(engine_combat, camps_guard_and_return_to_their_post) {
	vasal_engine_rules_standard(&_vasal_tests_engine_combat_rules);
	vasal_engine_world_s* const world = _vasal_tests_engine_combat_start(
		&_vasal_tests_engine_combat_world, 70U);
	const vasal_engine_map_point_s camp = { 50U, 40U };
	const size_t bandits = vasal_engine_world_add_camp(world, camp);
	cairo_assert_lt(bandits, vasal_engine_world_vasals_max);
	cairo_assert_true(world->vasals[bandits].npc);
	cairo_assert_eq(world->vasals[bandits].groups[0].mode,
		vasal_engine_world_mode_guard);
	cairo_assert_eq(world->vasals[bandits].groups[0].troop_count, 14U);
	// A player vintaine walks into the camp's radius; the bandits engage,
	// and the defensive vintaine fights back.
	_vasal_tests_engine_combat_add(world, 0U, 1U, 10U);
	const vasal_engine_map_point_s near = { 44U, 40U };
	(void)vasal_engine_world_order_move(world, 0U, 1U, near);
	bool_t engaged = false;
	for (uint32_t i = 0U; (i < 800U) && !engaged; i++) {
		vasal_engine_world_tick(world);
		engaged = world->vasals[bandits].groups[0].fighting;
	}
	cairo_assert_true(engaged);
	_vasal_tests_engine_combat_run(world, 4000U);
	// Once one side is gone, the bandits are back at their camp.
	const vasal_engine_world_group_s* const band =
		&world->vasals[bandits].groups[0];
	if ((0U < band->troop_count)
			&& (0U == world->vasals[0].groups[0].troop_count)) {
		const int32_t dx = band->anchor.x - ((50 * 256) + 128);
		const int32_t dy = band->anchor.y - ((40 * 256) + 128);
		cairo_assert_lt((dx * dx) + (dy * dy), (3 * 256) * (3 * 256));
	} else {
		cairo_assert_gt(world->vasals[0].kills, 0U);
	}
}

// spec: vintaines.modes#6
cairo_test_new(engine_combat, fleeing_vintaines_run_and_never_shoot) {
	vasal_engine_rules_standard(&_vasal_tests_engine_combat_rules);
	vasal_engine_world_s* const world = _vasal_tests_engine_combat_start(
		&_vasal_tests_engine_combat_world, 30U);
	_vasal_tests_engine_combat_add(world, 0U, 2U, 5U);
	_vasal_tests_engine_combat_add(world, 1U, 0U, 5U);
	vasal_engine_world_set_mode(world, 0U, 1U, vasal_engine_world_mode_flee);
	const int32_t start = world->vasals[0].groups[0].anchor.x;
	for (uint32_t i = 0U; i < 200U; i++) {
		vasal_engine_world_tick(world);
		cairo_assert_eq(world->shot_count, 0U);
	}
	// Away from the enemy, to the west.
	cairo_assert_lt(world->vasals[0].groups[0].anchor.x, start - 512);
}

// spec: combat.random#2 simulation.determinism#1
cairo_test_new(engine_combat, the_same_seed_the_same_battle) {
	vasal_engine_rules_standard(&_vasal_tests_engine_combat_rules);
	vasal_engine_world_s* const worlds[2] = {
		&_vasal_tests_engine_combat_world, &_vasal_tests_engine_combat_twin
	};
	for (size_t w = 0U; w < 2U; w++) {
		vasal_engine_world_s* const world =
			_vasal_tests_engine_combat_start(worlds[w], 32U);
		_vasal_tests_engine_combat_add(world, 0U, 0U, 12U);
		_vasal_tests_engine_combat_add(world, 0U, 2U, 6U);
		_vasal_tests_engine_combat_add(world, 1U, 1U, 15U);
		vasal_engine_world_set_mode(world, 0U, 1U,
			vasal_engine_world_mode_aggressive);
		_vasal_tests_engine_combat_run(world, 1200U);
	}
	for (size_t v = 0U; v < 2U; v++) {
		const vasal_engine_world_group_s* const a =
			&worlds[0]->vasals[v].groups[0];
		const vasal_engine_world_group_s* const b =
			&worlds[1]->vasals[v].groups[0];
		cairo_assert_eq(a->troop_count, b->troop_count);
		for (size_t t = 0U; t < a->troop_count; t++) {
			cairo_assert_eq(a->troops[t].id, b->troops[t].id);
			cairo_assert_eq(a->troops[t].position.x, b->troops[t].position.x);
			cairo_assert_eq(a->troops[t].wounded, b->troops[t].wounded);
		}
	}
}

// spec: rules.content#5 centaine.centenar#2
cairo_test_new(engine_combat, armies_come_from_the_rules) {
	vasal_engine_rules_standard(&_vasal_tests_engine_combat_rules);
	_vasal_tests_engine_combat_rules.start[4][0].unit = 4U;
	_vasal_tests_engine_combat_rules.start[4][0].count = 7U;
	_vasal_tests_engine_combat_rules.units[4].values
		[vasal_engine_rules_unit_speed] = 33U;
	vasal_engine_world_s* const world = _vasal_tests_engine_combat_start(
		&_vasal_tests_engine_combat_world, 70U);
	vasal_engine_world_add_army(world, 0U);
	const vasal_engine_world_vasal_s* const vasal = &world->vasals[0];
	cairo_assert_eq(vasal->groups[0].troop_count, 20U);
	cairo_assert_eq(vasal->groups[4].troop_count, 7U);
	cairo_assert_eq(vasal->groups[4].troops[0].unit, 4U);
	cairo_assert_eq(vasal->groups[4].troops[0].speed, 33U);
	cairo_assert_eq(vasal->groups[vasal_engine_world_centenar].troop_count, 1U);
	cairo_assert_eq(vasal->groups[vasal_engine_world_centenar].troops[0].unit,
		_vasal_tests_engine_combat_rules.centenar);
}
