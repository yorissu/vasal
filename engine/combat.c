
/// Every troop is filed by its slot (vasal, group and place, as one number)
/// into a hash of cells two tiles (512) wide, rebuilt each step, so looking for
/// enemies near a point visits only the cells around it. Distances are
/// compared squared, in 1/vasal_engine_map_precision of a tile, and ties go
/// to the lower troop id, so the outcome never depends on memory.

#include "engine/combat.h"

#define _vasal_engine_combat_slots                                            \
	(vasal_engine_world_vasals_max * vasal_engine_world_groups                \
		* vasal_engine_world_troops)
#define _vasal_engine_combat_cell ((int32_t)512)
#define _vasal_engine_combat_buckets 4096U
#define _vasal_engine_combat_empty 0xffffU
// A group's troops keep this close to its anchor, in tiles, beyond the
// chase distance.
#define _vasal_engine_combat_formation 3
// Ticks between plans of a fighting or fleeing anchor's way.
#define _vasal_engine_combat_replan 20U
// A fighting anchor stops this close to its enemy, in tiles.
#define _vasal_engine_combat_close 3
#define _vasal_engine_combat_pairs_max 2048U
#define _vasal_engine_combat_hits_max 4096U

static uint16_t _vasal_engine_combat_heads[_vasal_engine_combat_buckets];
static uint16_t _vasal_engine_combat_next[_vasal_engine_combat_slots];

// A possible duel, and a landed strike or shot.
typedef struct _vasal_engine_combat_pair_s {
	int64_t  distance;
	uint32_t id;
	uint16_t a;
	uint16_t b;
} _vasal_engine_combat_pair_s;

typedef struct _vasal_engine_combat_hit_s {
	uint16_t slot;
	uint8_t  killer;
	bool_t   kills;
} _vasal_engine_combat_hit_s;

static _vasal_engine_combat_pair_s
	_vasal_engine_combat_pairs[_vasal_engine_combat_pairs_max];
static _vasal_engine_combat_hit_s
	_vasal_engine_combat_hits[_vasal_engine_combat_hits_max];
static size_t _vasal_engine_combat_hit_count = 0U;

// The slot of a troop, and back.
static uint16_t _vasal_engine_combat_slot(const size_t vasal,
		const size_t group, const size_t troop) {
	const size_t slot = (((vasal * vasal_engine_world_groups) + group)
		* vasal_engine_world_troops) + troop;
	return (uint16_t)slot;
}

static vasal_engine_world_where_s _vasal_engine_combat_where(
		const uint16_t slot) {
	const size_t troops = vasal_engine_world_troops;
	const size_t groups = vasal_engine_world_groups;
	const vasal_engine_world_where_s where = {
		(uint8_t)((size_t)slot / (troops * groups)),
		(uint8_t)(((size_t)slot / troops) % groups),
		(uint8_t)((size_t)slot % troops)
	};
	return where;
}

static vasal_engine_world_group_s* _vasal_engine_combat_group(
		vasal_engine_world_s* const world,
		const vasal_engine_world_where_s where) {
	return &world->vasals[where.vasal].groups[where.group];
}

static vasal_engine_world_troop_s* _vasal_engine_combat_troop(
		vasal_engine_world_s* const world, const uint16_t slot) {
	const vasal_engine_world_where_s where = _vasal_engine_combat_where(slot);
	return &_vasal_engine_combat_group(world, where)->troops[where.troop];
}

static int64_t _vasal_engine_combat_distance(
		const vasal_engine_world_position_s a,
		const vasal_engine_world_position_s b) {
	const int64_t dx = (int64_t)a.x - (int64_t)b.x;
	const int64_t dy = (int64_t)a.y - (int64_t)b.y;
	return (dx * dx) + (dy * dy);
}

static uint32_t _vasal_engine_combat_bucket(const int32_t cx,
		const int32_t cy) {
	const uint32_t hash = ((uint32_t)cx * 73856093U)
		^ ((uint32_t)cy * 19349663U);
	return hash % _vasal_engine_combat_buckets;
}

// Files every troop into the hash.
static void _vasal_engine_combat_file(const vasal_engine_world_s* const world) {
	for (size_t b = 0U; b < _vasal_engine_combat_buckets; b++) {
		_vasal_engine_combat_heads[b] = _vasal_engine_combat_empty;
	}
	for (size_t v = 0U; v < world->vasal_count; v++) {
		for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
			const vasal_engine_world_group_s* const group =
				&world->vasals[v].groups[g];
			for (size_t t = 0U; t < group->troop_count; t++) {
				const vasal_engine_world_position_s at =
					group->troops[t].position;
				const uint32_t bucket = _vasal_engine_combat_bucket(
					at.x / _vasal_engine_combat_cell,
					at.y / _vasal_engine_combat_cell);
				const uint16_t slot = _vasal_engine_combat_slot(v, g, t);
				_vasal_engine_combat_next[slot] =
					_vasal_engine_combat_heads[bucket];
				_vasal_engine_combat_heads[bucket] = slot;
			}
		}
	}
}

// What a search for the nearest troop looks for.
typedef struct _vasal_engine_combat_search_s {
	vasal_engine_world_position_s from;
	int64_t                       reach;
	/// Enemies of this vasal only.
	size_t                        vasal;
	/// Only troops of this group, unless it is nobody.
	vasal_engine_world_where_s    of;
	/// Only troops in no duel.
	bool_t                        free;
} _vasal_engine_combat_search_s;

// Whether a troop is one the search looks for.
static bool_t _vasal_engine_combat_fits(vasal_engine_world_s* const world,
		const _vasal_engine_combat_search_s* const search,
		const uint16_t slot) {
	const vasal_engine_world_where_s where = _vasal_engine_combat_where(slot);
	const vasal_engine_world_troop_s* const troop =
		_vasal_engine_combat_troop(world, slot);
	const bool_t group = (vasal_engine_world_nobody == search->of.vasal)
		|| ((where.vasal == search->of.vasal)
			&& (where.group == search->of.group));
	return group && (!search->free || (0U == troop->duel))
		&& vasal_engine_world_enemies(world, search->vasal, where.vasal);
}

// The nearest troop a search looks for, within its reach, or the empty
// slot.
static uint16_t _vasal_engine_combat_nearest(vasal_engine_world_s* const world,
		const _vasal_engine_combat_search_s* const search) {
	const int64_t reach = search->reach;
	const int32_t cells = (int32_t)(reach / _vasal_engine_combat_cell) + 1;
	const int32_t cx = search->from.x / _vasal_engine_combat_cell;
	const int32_t cy = search->from.y / _vasal_engine_combat_cell;
	uint16_t best = _vasal_engine_combat_empty;
	int64_t best_distance = (reach * reach) + 1;
	uint32_t best_id = 0U;
	for (int32_t y = cy - cells; y <= (cy + cells); y++) {
		for (int32_t x = cx - cells; x <= (cx + cells); x++) {
			uint16_t slot = _vasal_engine_combat_heads[
				_vasal_engine_combat_bucket(x, y)];
			while (_vasal_engine_combat_empty != slot) {
				const vasal_engine_world_troop_s* const troop =
					_vasal_engine_combat_troop(world, slot);
				const int64_t distance = _vasal_engine_combat_distance(
					search->from, troop->position);
				const bool_t nearer = (distance < best_distance)
					|| ((distance == best_distance) && (troop->id < best_id));
				if (nearer && _vasal_engine_combat_fits(world, search, slot)) {
					best = slot;
					best_distance = distance;
					best_id = troop->id;
				}
				slot = _vasal_engine_combat_next[slot];
			}
		}
	}
	return best;
}

static uint32_t _vasal_engine_combat_unit(const vasal_engine_world_s* const
		world, const vasal_engine_world_troop_s* const troop,
		const vasal_engine_rules_unit_e field) {
	return vasal_engine_rules_get(world->rules, troop->unit, field);
}

static int64_t _vasal_engine_combat_tiles(const uint32_t tiles) {
	return (int64_t)tiles * (int64_t)vasal_engine_map_precision;
}

// A value kept within low and high.
static int32_t _vasal_engine_combat_clamp(const int32_t value,
		const int32_t low, const int32_t high) {
	int32_t kept = value;
	if (kept < low) {
		kept = low;
	}
	if (kept > high) {
		kept = high;
	}
	return kept;
}

// The tile a position stands on.
static vasal_engine_map_point_s _vasal_engine_combat_tile(
		const vasal_engine_world_s* const world,
		const vasal_engine_world_position_s at) {
	const int32_t x = _vasal_engine_combat_clamp(
		at.x / vasal_engine_map_precision, 0, (int32_t)world->map->width - 1);
	const int32_t y = _vasal_engine_combat_clamp(
		at.y / vasal_engine_map_precision, 0, (int32_t)world->map->height - 1);
	const vasal_engine_map_point_s tile = { (uint16_t)x, (uint16_t)y };
	return tile;
}

// The enemy a group fights by its mode, or the empty slot.
// spec: vintaines.modes#1 vintaines.modes#2 vintaines.modes#3
// spec: vintaines.modes#4 vintaines.modes#5 vintaines.modes#6
// spec: vintaines.modes#7
static uint16_t _vasal_engine_combat_choose(vasal_engine_world_s* const world,
		const size_t vasal, const vasal_engine_world_group_s* const group) {
	const uint32_t* const combat = world->rules->combat;
	_vasal_engine_combat_search_s search;
	search.vasal = vasal;
	search.of.vasal = vasal_engine_world_nobody;
	search.of.group = vasal_engine_world_nobody;
	search.free = false;
	search.from = group->anchor;
	search.reach = _vasal_engine_combat_tiles(
		combat[vasal_engine_rules_combat_sight]);
	uint16_t enemy = _vasal_engine_combat_empty;
	if ((vasal_engine_world_mode_aggressive == group->mode)
			|| (vasal_engine_world_mode_flee == group->mode)) {
		enemy = _vasal_engine_combat_nearest(world, &search);
	} else if (vasal_engine_world_mode_guard == group->mode) {
		// Not while marching under orders; back at its post, or chasing
		// an enemy within the radius around it.
		search.from = group->post;
		search.reach = _vasal_engine_combat_tiles(
			combat[vasal_engine_rules_combat_guard]);
		if (!group->ordered) {
			enemy = _vasal_engine_combat_nearest(world, &search);
		}
	} else if (0U < group->attacked) {
		// Defensive: the vintaine that attacked it.
		search.of = group->attacker;
		search.reach *= 2;
		enemy = _vasal_engine_combat_nearest(world, &search);
	} else {
		// Defensive and left alone.
	}
	return enemy;
}

// Moves a fighting or fleeing group's anchor: toward its enemy until
// close, away from it while fleeing, back to its post or order after.
static void _vasal_engine_combat_steer(vasal_engine_world_s* const world,
		const size_t vasal, const size_t g, const bool_t was) {
	vasal_engine_world_group_s* const group = &world->vasals[vasal].groups[g];
	const int64_t close = _vasal_engine_combat_tiles(
		(uint32_t)_vasal_engine_combat_close);
	if (group->fighting) {
		if (_vasal_engine_combat_distance(group->anchor, group->enemy)
				<= (close * close)) {
			group->moving = false;
		} else if (0U == group->replan) {
			vasal_engine_world_steer(world, vasal, g,
				_vasal_engine_combat_tile(world, group->enemy));
			group->replan = _vasal_engine_combat_replan;
		} else {
			// On its way.
		}
	} else if (was) {
		// The fight is over: resume the order, or return to the post.
		if (group->ordered) {
			vasal_engine_world_steer(world, vasal, g, group->goal);
		} else if (vasal_engine_world_mode_aggressive != group->mode) {
			vasal_engine_world_steer(world, vasal, g,
				_vasal_engine_combat_tile(world, group->post));
		} else {
			group->moving = false;
		}
	} else {
		// Not fighting.
	}
}

// Runs from an enemy: toward the point as far beyond the anchor as the
// mode's sight, opposite the enemy.
// spec: vintaines.modes#6
static void _vasal_engine_combat_flee(vasal_engine_world_s* const world,
		const size_t vasal, const size_t g,
		const vasal_engine_world_position_s from) {
	vasal_engine_world_group_s* const group = &world->vasals[vasal].groups[g];
	if (0U != group->replan) {
		return;
	}
	const int64_t dx = (int64_t)group->anchor.x - (int64_t)from.x;
	const int64_t dy = (int64_t)group->anchor.y - (int64_t)from.y;
	const int64_t length = ((dx < 0) ? -dx : dx) + ((dy < 0) ? -dy : dy) + 1;
	const int64_t reach = _vasal_engine_combat_tiles(
		world->rules->combat[vasal_engine_rules_combat_sight]);
	const vasal_engine_world_position_s away = {
		group->anchor.x + (int32_t)((dx * reach) / length),
		group->anchor.y + (int32_t)((dy * reach) / length)
	};
	vasal_engine_world_steer(world, vasal, g,
		_vasal_engine_combat_tile(world, away));
	group->replan = _vasal_engine_combat_replan;
}

// Whether a troop of a group is willing to fight this tick.
static bool_t _vasal_engine_combat_willing(const vasal_engine_world_s* const
		world, const vasal_engine_world_group_s* const group,
		const vasal_engine_world_troop_s* const troop) {
	const bool_t careful = 0U != _vasal_engine_combat_unit(world, troop,
		vasal_engine_rules_unit_careful);
	return group->fighting && (vasal_engine_world_mode_flee != group->mode)
		&& !(careful && troop->wounded);
}

// Where a willing troop not in a duel heads for: the nearest enemy within
// its chase of the anchor; a ranged troop with an enemy in range stands
// and aims at it.
// spec: combat.ranged#1
static void _vasal_engine_combat_aim(vasal_engine_world_s* const world,
		const size_t vasal, const vasal_engine_world_group_s* const group,
		vasal_engine_world_troop_s* const troop) {
	troop->aiming = false;
	troop->target = 0U;
	if ((0U != troop->duel) || !_vasal_engine_combat_willing(world, group,
			troop)) {
		return;
	}
	_vasal_engine_combat_search_s search;
	search.vasal = vasal;
	search.of.vasal = vasal_engine_world_nobody;
	search.of.group = vasal_engine_world_nobody;
	search.free = false;
	const bool_t ranged = 0U != _vasal_engine_combat_unit(world, troop,
		vasal_engine_rules_unit_ranged);
	if (ranged) {
		search.from = troop->position;
		search.reach = _vasal_engine_combat_tiles(_vasal_engine_combat_unit(
			world, troop, vasal_engine_rules_unit_range));
		const uint16_t sighted = _vasal_engine_combat_nearest(world, &search);
		if (_vasal_engine_combat_empty != sighted) {
			troop->target = _vasal_engine_combat_troop(world, sighted)->id;
			troop->sighted = _vasal_engine_combat_where(sighted);
			troop->aim = troop->position;
			troop->aiming = true;
			return;
		}
	}
	search.from = group->anchor;
	search.reach = _vasal_engine_combat_tiles(_vasal_engine_combat_unit(world,
		troop, vasal_engine_rules_unit_chase)
		+ (uint32_t)_vasal_engine_combat_formation);
	const uint16_t enemy = _vasal_engine_combat_nearest(world, &search);
	if (_vasal_engine_combat_empty != enemy) {
		troop->aim = _vasal_engine_combat_troop(world, enemy)->position;
		troop->aiming = true;
	}
}

// spec: simulation.steps#1 vintaines.modes
void vasal_engine_combat_targets(vasal_engine_world_s* const world) {
	_vasal_engine_combat_file(world);
	for (size_t v = 0U; v < world->vasal_count; v++) {
		for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
			vasal_engine_world_group_s* const group =
				&world->vasals[v].groups[g];
			if (0U == group->troop_count) {
				group->fighting = false;
				continue;
			}
			if (0U < group->attacked) {
				group->attacked--;
			}
			if (0U < group->replan) {
				group->replan--;
			}
			const bool_t was = group->fighting;
			const uint16_t enemy = _vasal_engine_combat_choose(world, v, group);
			const bool_t found = _vasal_engine_combat_empty != enemy;
			group->fighting = found
				&& (vasal_engine_world_mode_flee != group->mode);
			if (found) {
				group->enemy =
					_vasal_engine_combat_troop(world, enemy)->position;
			}
			if (found && (vasal_engine_world_mode_flee == group->mode)) {
				_vasal_engine_combat_flee(world, v, g, group->enemy);
			} else {
				_vasal_engine_combat_steer(world, v, g, was);
			}
			for (size_t t = 0U; t < group->troop_count; t++) {
				_vasal_engine_combat_aim(world, v, group, &group->troops[t]);
			}
		}
	}
}

// Marks a troop and its group as attacked by a troop of another group.
static void _vasal_engine_combat_attacked(vasal_engine_world_s* const world,
		const uint16_t victim, const uint16_t attacker) {
	const vasal_engine_world_where_s by = _vasal_engine_combat_where(attacker);
	vasal_engine_world_group_s* const group = _vasal_engine_combat_group(world,
		_vasal_engine_combat_where(victim));
	group->attacked =
		(uint16_t)world->rules->combat[vasal_engine_rules_combat_linger] + 1U;
	group->attacker.vasal = by.vasal;
	group->attacker.group = by.group;
}

// Keeps the possible duels sorted by distance, then lower id.
static void _vasal_engine_combat_sort(const size_t count) {
	_vasal_engine_combat_pair_s* const pairs = _vasal_engine_combat_pairs;
	for (size_t i = 1U; i < count; i++) {
		const _vasal_engine_combat_pair_s pair = pairs[i];
		size_t at = i;
		while ((0U < at) && ((pairs[at - 1U].distance > pair.distance)
				|| ((pairs[at - 1U].distance == pair.distance)
					&& (pairs[at - 1U].id > pair.id)))) {
			pairs[at] = pairs[at - 1U];
			at--;
		}
		pairs[at] = pair;
	}
}

// The weight of a troop's first strike: its skill, lowered by its sidearm
// penalty when it is ranged.
static uint32_t _vasal_engine_combat_lowered(const vasal_engine_world_s* const
		world, const vasal_engine_world_troop_s* const troop,
		const uint32_t value) {
	uint32_t lowered = value;
	if (0U != _vasal_engine_combat_unit(world, troop,
			vasal_engine_rules_unit_ranged)) {
		lowered = (value * (100U - _vasal_engine_combat_unit(world, troop,
			vasal_engine_rules_unit_sidearm))) / 100U;
	}
	return lowered;
}

// Starts a duel: a roll weighted by skill decides who strikes first.
// spec: combat.duel#1 combat.strike#1 combat.strike#2 combat.sidearm#2
static void _vasal_engine_combat_start(vasal_engine_world_s* const world,
		const uint16_t slot_a, const uint16_t slot_b) {
	vasal_engine_world_troop_s* const a =
		_vasal_engine_combat_troop(world, slot_a);
	vasal_engine_world_troop_s* const b =
		_vasal_engine_combat_troop(world, slot_b);
	const uint32_t weight_a = _vasal_engine_combat_lowered(world, a,
		_vasal_engine_combat_unit(world, a, vasal_engine_rules_unit_skill))
		+ 1U;
	const uint32_t weight_b = _vasal_engine_combat_lowered(world, b,
		_vasal_engine_combat_unit(world, b, vasal_engine_rules_unit_skill))
		+ 1U;
	a->striking = vasal_engine_random_below(&world->combat_random,
		weight_a + weight_b) < weight_a;
	b->striking = !a->striking;
	a->duel = b->id;
	b->duel = a->id;
	a->partner = _vasal_engine_combat_where(slot_b);
	b->partner = _vasal_engine_combat_where(slot_a);
	vasal_engine_world_troop_s* const first = a->striking ? a : b;
	first->timer = (uint16_t)_vasal_engine_combat_unit(world, first,
		vasal_engine_rules_unit_interval);
	_vasal_engine_combat_attacked(world, slot_b, slot_a);
	_vasal_engine_combat_attacked(world, slot_a, slot_b);
}

// Locks willing melee troops into duels with the nearest enemies in duel
// distance, the shortest pairs first, each troop in one duel at most.
// spec: combat.duel#1 combat.duel#2 combat.duel#3 combat.duel#4
static void _vasal_engine_combat_pair(vasal_engine_world_s* const world) {
	size_t count = 0U;
	_vasal_engine_combat_search_s search;
	search.of.vasal = vasal_engine_world_nobody;
	search.of.group = vasal_engine_world_nobody;
	search.free = true;
	search.reach =
		(int64_t)world->rules->combat[vasal_engine_rules_combat_duel];
	for (size_t v = 0U; v < world->vasal_count; v++) {
		for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
			vasal_engine_world_group_s* const group =
				&world->vasals[v].groups[g];
			for (size_t t = 0U; t < group->troop_count; t++) {
				vasal_engine_world_troop_s* const troop = &group->troops[t];
				const bool_t melee = 0U == _vasal_engine_combat_unit(world,
					troop, vasal_engine_rules_unit_ranged);
				if (!melee || (0U != troop->duel)
						|| !_vasal_engine_combat_willing(world, group, troop)
						|| (count >= _vasal_engine_combat_pairs_max)) {
					continue;
				}
				search.vasal = v;
				search.from = troop->position;
				const uint16_t enemy =
					_vasal_engine_combat_nearest(world, &search);
				if (_vasal_engine_combat_empty != enemy) {
					const vasal_engine_world_troop_s* const other =
						_vasal_engine_combat_troop(world, enemy);
					_vasal_engine_combat_pair_s* const pair =
						&_vasal_engine_combat_pairs[count];
					pair->distance = _vasal_engine_combat_distance(
						troop->position, other->position);
					pair->id = (troop->id < other->id) ? troop->id : other->id;
					pair->a = _vasal_engine_combat_slot(v, g, t);
					pair->b = enemy;
					count++;
				}
			}
		}
	}
	_vasal_engine_combat_sort(count);
	for (size_t i = 0U; i < count; i++) {
		const _vasal_engine_combat_pair_s* const pair =
			&_vasal_engine_combat_pairs[i];
		if ((0U == _vasal_engine_combat_troop(world, pair->a)->duel)
				&& (0U == _vasal_engine_combat_troop(world, pair->b)->duel)) {
			_vasal_engine_combat_start(world, pair->a, pair->b);
		}
	}
}

// Notes a landed strike or shot: it kills by the attacker's chance, else
// it wounds; the results apply at the end of the step.
// spec: combat.strike#5 combat.wound#1 combat.tick#1 combat.credit#1
static void _vasal_engine_combat_land(vasal_engine_world_s* const world,
		const uint16_t attacker, const uint16_t victim) {
	if (_vasal_engine_combat_hit_count >= _vasal_engine_combat_hits_max) {
		return;
	}
	const vasal_engine_world_troop_s* const by =
		_vasal_engine_combat_troop(world, attacker);
	_vasal_engine_combat_hit_s* const hit =
		&_vasal_engine_combat_hits[_vasal_engine_combat_hit_count];
	hit->slot = victim;
	hit->killer = _vasal_engine_combat_where(attacker).vasal;
	hit->kills = vasal_engine_random_chance(&world->combat_random,
		_vasal_engine_combat_unit(world, by, vasal_engine_rules_unit_kill));
	_vasal_engine_combat_hit_count++;
}

// A troop's slot, from where it was and its id, found again in its group
// when the group changed; or the empty slot when it is gone.
static uint16_t _vasal_engine_combat_find(vasal_engine_world_s* const world,
		const vasal_engine_world_where_s where, const uint32_t id) {
	if ((where.vasal >= world->vasal_count)
			|| (where.group >= vasal_engine_world_groups)) {
		return _vasal_engine_combat_empty;
	}
	const vasal_engine_world_group_s* const group =
		_vasal_engine_combat_group(world, where);
	for (size_t t = 0U; t < group->troop_count; t++) {
		const size_t at = ((size_t)where.troop + t) % group->troop_count;
		if (id == group->troops[at].id) {
			return _vasal_engine_combat_slot(where.vasal, where.group, at);
		}
	}
	return _vasal_engine_combat_empty;
}

// The troops in duels strike in turn, one strike every interval of the
// striker; the defender may parry, by its parry chance shifted by the
// difference of skills; a ranged troop's chances drop by its sidearm
// penalty.
// spec: combat.strike#3 combat.strike#4 combat.sidearm#2
static void _vasal_engine_combat_strike(vasal_engine_world_s* const world,
		const uint16_t slot) {
	vasal_engine_world_troop_s* const striker =
		_vasal_engine_combat_troop(world, slot);
	const uint16_t partner = _vasal_engine_combat_find(world, striker->partner,
		striker->duel);
	if (_vasal_engine_combat_empty == partner) {
		return;
	}
	vasal_engine_world_troop_s* const defender =
		_vasal_engine_combat_troop(world, partner);
	const int32_t skill_striker = (int32_t)_vasal_engine_combat_unit(world,
		striker, vasal_engine_rules_unit_skill);
	const int32_t skill_defender = (int32_t)_vasal_engine_combat_unit(world,
		defender, vasal_engine_rules_unit_skill);
	const int32_t parry = _vasal_engine_combat_clamp(
		(int32_t)_vasal_engine_combat_lowered(world, defender,
			_vasal_engine_combat_unit(world, defender,
				vasal_engine_rules_unit_parry))
		+ ((skill_defender - skill_striker) / 2), 0, 95);
	const bool_t lands = vasal_engine_random_chance(&world->combat_random,
		_vasal_engine_combat_lowered(world, striker, 100U));
	const bool_t parried = vasal_engine_random_chance(&world->combat_random,
		(uint32_t)parry);
	if (lands && !parried) {
		_vasal_engine_combat_land(world, slot, partner);
	}
	striker->striking = false;
	defender->striking = true;
	defender->timer = (uint16_t)_vasal_engine_combat_unit(world, defender,
		vasal_engine_rules_unit_interval);
}

// A ranged troop not in a duel shoots at its target once its reload is
// over: the shot hits by the aim and the shooter's skill, the target may
// block it, and a shot that gets through lands.
// spec: combat.ranged#2 combat.ranged#3 combat.ranged#4 combat.ranged#5
// spec: combat.ranged#6 combat.sidearm#1 combat.engaged#2
static void _vasal_engine_combat_shoot(vasal_engine_world_s* const world,
		const uint16_t slot) {
	vasal_engine_world_troop_s* const shooter =
		_vasal_engine_combat_troop(world, slot);
	const uint16_t target = _vasal_engine_combat_find(world, shooter->sighted,
		shooter->target);
	if (_vasal_engine_combat_empty == target) {
		return;
	}
	vasal_engine_world_troop_s* const victim =
		_vasal_engine_combat_troop(world, target);
	const uint32_t linger =
		world->rules->combat[vasal_engine_rules_combat_linger];
	uint32_t aim = world->rules->combat[vasal_engine_rules_combat_aim]
		+ (_vasal_engine_combat_unit(world, shooter,
			vasal_engine_rules_unit_skill) / 2U);
	aim = (aim > 95U) ? 95U : aim;
	const bool_t hits = vasal_engine_random_chance(&world->combat_random, aim);
	const bool_t blocked = vasal_engine_random_chance(&world->combat_random,
		_vasal_engine_combat_unit(world, victim,
			vasal_engine_rules_unit_block));
	if (hits && !blocked) {
		_vasal_engine_combat_land(world, slot, target);
	}
	shooter->timer = (uint16_t)_vasal_engine_combat_unit(world, shooter,
		vasal_engine_rules_unit_reload);
	shooter->engaged = (uint16_t)linger;
	victim->engaged = (uint16_t)linger;
	_vasal_engine_combat_attacked(world, target, slot);
	if (world->shot_count < vasal_engine_world_shots_max) {
		world->shots[world->shot_count].from = shooter->position;
		world->shots[world->shot_count].to = victim->position;
		world->shot_count++;
	}
}

// Counts down a troop's timers. Returns whether it strikes or shoots now.
static bool_t _vasal_engine_combat_ready(
		vasal_engine_world_troop_s* const troop) {
	if (0U < troop->engaged) {
		troop->engaged--;
	}
	if (0U < troop->timer) {
		troop->timer--;
	}
	const bool_t strikes = (0U != troop->duel) && troop->striking;
	const bool_t shoots = (0U == troop->duel) && (0U != troop->target);
	return (strikes || shoots) && (0U == troop->timer);
}

// Counts down every timer, and strikes and shoots those that reach 0:
// all decided against the state before, as they happen together.
static void _vasal_engine_combat_fight(vasal_engine_world_s* const world) {
	static uint16_t ready[_vasal_engine_combat_slots];
	size_t count = 0U;
	for (size_t v = 0U; v < world->vasal_count; v++) {
		for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
			vasal_engine_world_group_s* const group =
				&world->vasals[v].groups[g];
			for (size_t t = 0U; t < group->troop_count; t++) {
				if (_vasal_engine_combat_ready(&group->troops[t])) {
					ready[count] = _vasal_engine_combat_slot(v, g, t);
					count++;
				}
			}
		}
	}
	for (size_t i = 0U; i < count; i++) {
		if (0U != _vasal_engine_combat_troop(world, ready[i])->duel) {
			_vasal_engine_combat_strike(world, ready[i]);
		} else {
			_vasal_engine_combat_shoot(world, ready[i]);
		}
	}
}

// Applies the step's hits: a kill or a second wound kills; and removes
// the dead, each group keeping its order otherwise.
// spec: combat.tick#1 combat.wound#1 combat.wound#2 combat.duel#6
static void _vasal_engine_combat_bury(vasal_engine_world_s* const world) {
	static uint8_t wounds[_vasal_engine_combat_slots];
	static bool_t dead[_vasal_engine_combat_slots];
	for (size_t i = 0U; i < _vasal_engine_combat_hit_count; i++) {
		const _vasal_engine_combat_hit_s* const hit =
			&_vasal_engine_combat_hits[i];
		wounds[hit->slot] = 0U;
		dead[hit->slot] = false;
	}
	for (size_t i = 0U; i < _vasal_engine_combat_hit_count; i++) {
		const _vasal_engine_combat_hit_s* const hit =
			&_vasal_engine_combat_hits[i];
		const vasal_engine_world_troop_s* const troop =
			_vasal_engine_combat_troop(world, hit->slot);
		const bool_t was = dead[hit->slot];
		wounds[hit->slot]++;
		dead[hit->slot] = was || hit->kills || troop->wounded
			|| (2U <= wounds[hit->slot]);
		if (!was && dead[hit->slot]) {
			world->vasals[hit->killer].kills++;
		}
	}
	for (size_t i = 0U; i < _vasal_engine_combat_hit_count; i++) {
		_vasal_engine_combat_troop(world, _vasal_engine_combat_hits[i].slot)
			->wounded = true;
	}
	// The dead, each once, from the highest slot down, so a troop that
	// takes a dead one's place (the last of its group) was looked at.
	static uint16_t buried[_vasal_engine_combat_hits_max];
	size_t count = 0U;
	for (size_t i = 0U; i < _vasal_engine_combat_hit_count; i++) {
		const uint16_t slot = _vasal_engine_combat_hits[i].slot;
		if (dead[slot]) {
			dead[slot] = false;
			size_t at = count;
			while ((0U < at) && (buried[at - 1U] < slot)) {
				buried[at] = buried[at - 1U];
				at--;
			}
			buried[at] = slot;
			count++;
		}
	}
	for (size_t i = 0U; i < count; i++) {
		const vasal_engine_world_where_s where =
			_vasal_engine_combat_where(buried[i]);
		vasal_engine_world_group_s* const group =
			_vasal_engine_combat_group(world, where);
		group->troops[where.troop] = group->troops[group->troop_count - 1U];
		group->troop_count--;
	}
}

// Ends the duels whose partner is gone; they follow their orders from the
// next tick.
static void _vasal_engine_combat_part(vasal_engine_world_s* const world) {
	for (size_t v = 0U; v < world->vasal_count; v++) {
		for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
			vasal_engine_world_group_s* const group =
				&world->vasals[v].groups[g];
			for (size_t t = 0U; t < group->troop_count; t++) {
				vasal_engine_world_troop_s* const troop = &group->troops[t];
				if (0U == troop->duel) {
					continue;
				}
				const uint16_t partner = _vasal_engine_combat_find(world,
					troop->partner, troop->duel);
				if (_vasal_engine_combat_empty == partner) {
					troop->duel = 0U;
					troop->striking = false;
				} else {
					troop->partner = _vasal_engine_combat_where(partner);
				}
			}
		}
	}
}

// spec: simulation.steps#1 combat.tick#1
void vasal_engine_combat_resolve(vasal_engine_world_s* const world) {
	world->shot_count = 0U;
	_vasal_engine_combat_hit_count = 0U;
	_vasal_engine_combat_file(world);
	_vasal_engine_combat_pair(world);
	_vasal_engine_combat_fight(world);
	_vasal_engine_combat_bury(world);
	_vasal_engine_combat_part(world);
}
