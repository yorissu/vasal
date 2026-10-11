
/// The simulated world: the vasals, their centaines and troops, on a map.
/// It advances in fixed ticks, with whole numbers only, so the same orders
/// give the same result on every machine. All of it lives in fixed pools
/// sized by the hard limits.
///
/// A vasal's centaine has 5 vintaines and the centenar, who is controlled
/// like a vintaine of one; here the centenar is group 5, so groups 0 to 4
/// are the vintaines.

#ifndef vasal__engine__world_h
#define vasal__engine__world_h

#include "engine/map.h"
#include "engine/path.h"
#include "engine/random.h"
#include "engine/rules.h"

/// Ticks per second.
#define vasal_engine_world_tick_rate 20U

/// Groups of a centaine: 5 vintaines and the centenar.
#define vasal_engine_world_groups 6U
/// The centenar's group.
#define vasal_engine_world_centenar 5U
/// The most troops of a vintaine; the centenar's group holds one.
#define vasal_engine_world_troops 20U
/// The most vasals in a world.
#define vasal_engine_world_vasals_max 100U
/// The most turning points a group's path keeps.
#define vasal_engine_world_waypoints 256U

/// The most orders a group can have queued after its current one
/// (placeholder until tuning).
#define vasal_engine_world_queue_max 8U

/// The alliance of bandits, enemies of every player.
#define vasal_engine_world_bandits 0xffffffffU

/// The most shots a tick remembers, for the client to draw.
#define vasal_engine_world_shots_max 512U

/// No troop, group or vasal, in a reference.
#define vasal_engine_world_nobody 0xffU

/// A position, in 1/vasal_engine_map_precision of a tile.
typedef struct vasal_engine_world_position_s {
	int32_t x;
	int32_t y;
} vasal_engine_world_position_s;

/// How a group marches.
typedef enum vasal_engine_world_pace_e {
	/// Together, at the speed of its slowest troop.
	vasal_engine_world_pace_in_step,
	/// Every troop as fast as it can; the slow ones catch up.
	vasal_engine_world_pace_at_the_run
} vasal_engine_world_pace_e;

/// When a vintaine is willing to fight (vintaines.modes).
typedef enum vasal_engine_world_mode_e {
	/// Attacks any enemy it sees.
	vasal_engine_world_mode_aggressive,
	/// Engages any enemy entering a radius around its post, then returns.
	vasal_engine_world_mode_guard,
	/// Fights only when attacked, then returns to its post.
	vasal_engine_world_mode_defensive,
	/// Runs from enemies it sees.
	vasal_engine_world_mode_flee,
	vasal_engine_world_modes
} vasal_engine_world_mode_e;

/// Where a troop stands in the world: its vasal, group and place in it.
typedef struct vasal_engine_world_where_s {
	uint8_t vasal;
	uint8_t group;
	uint8_t troop;
} vasal_engine_world_where_s;

/// A troop.
typedef struct vasal_engine_world_troop_s {
	vasal_engine_world_position_s position;
	uint32_t                      id;
	/// Its speed, in 1/vasal_engine_map_precision of a tile per tick, on
	/// terrain of 100 percent speed.
	uint16_t                      speed;
	/// Its unit type in the rules.
	uint8_t                       unit;
	bool_t                        wounded;
	/// Its duel partner's id, 0 when it is in no duel, and where the
	/// partner stands; whether it strikes next.
	uint32_t                      duel;
	vasal_engine_world_where_s    partner;
	bool_t                        striking;
	/// Ticks until its next strike or shot.
	uint16_t                      timer;
	/// Ticks it stays in combat after a shot (combat.engaged).
	uint16_t                      engaged;
	/// Where combat sends it this tick, when aiming.
	vasal_engine_world_position_s aim;
	bool_t                        aiming;
	/// The enemy it shoots at, by id (0 for none) and where it stands.
	uint32_t                      target;
	vasal_engine_world_where_s    sighted;
} vasal_engine_world_troop_s;

/// A shot of the last tick, for the client to draw.
typedef struct vasal_engine_world_shot_s {
	vasal_engine_world_position_s from;
	vasal_engine_world_position_s to;
} vasal_engine_world_shot_s;

/// A vintaine, or the centenar's group.
typedef struct vasal_engine_world_group_s {
	vasal_engine_world_troop_s    troops[vasal_engine_world_troops];
	/// The point the group forms up around, moving along its path.
	vasal_engine_world_position_s anchor;
	/// Where the anchor stood when the group set out on its path.
	vasal_engine_world_position_s origin;
	vasal_engine_map_point_s      waypoints[vasal_engine_world_waypoints];
	uint16_t                      waypoint_count;
	uint16_t                      waypoint_next;
	/// Where the group goes after its current order, in order.
	vasal_engine_map_point_s      queue[vasal_engine_world_queue_max];
	uint8_t                       queue_count;
	uint8_t                       troop_count;
	vasal_engine_world_pace_e     pace;
	bool_t                        moving;
	vasal_engine_world_mode_e     mode;
	/// Its post: where its last move order led, or where it stood when it
	/// got its mode.
	vasal_engine_world_position_s post;
	/// The goal of its current move order, to resume after a fight.
	vasal_engine_map_point_s      goal;
	bool_t                        ordered;
	/// Whether it is fighting, and where its enemy stands.
	bool_t                        fighting;
	vasal_engine_world_position_s enemy;
	/// The group that last attacked it and how long ago that still
	/// counts, for the defensive mode.
	vasal_engine_world_where_s    attacker;
	uint16_t                      attacked;
	/// Ticks until it plans its way toward or away from its enemy again.
	uint16_t                      replan;
	/// Whether its path was planned by a fight rather than an order.
	bool_t                        steered;
} vasal_engine_world_group_s;

/// A vasal and their centaine, or a band of NPCs.
typedef struct vasal_engine_world_vasal_s {
	vasal_engine_world_group_s groups[vasal_engine_world_groups];
	uint32_t                   id;
	/// Its alliance: vasals of different alliances are enemies;
	/// vasal_engine_world_bandits for bandits.
	uint32_t                   alliance;
	/// Enemy troops it killed.
	uint32_t                   kills;
	bool_t                     npc;
} vasal_engine_world_vasal_s;

/// The world.
typedef struct vasal_engine_world_s {
	vasal_engine_world_vasal_s vasals[vasal_engine_world_vasals_max];
	const vasal_engine_map_s*  map;
	const vasal_engine_rules_s* rules;
	/// The combat stream of the match's seed.
	vasal_engine_random_s      combat_random;
	/// The shots fired in the last tick.
	vasal_engine_world_shot_s  shots[vasal_engine_world_shots_max];
	uint16_t                   shot_count;
	uint32_t                   tick;
	/// The next id to give; ids are never reused.
	uint32_t                   next_id;
	uint8_t                    vasal_count;
} vasal_engine_world_s;

/// Starts an empty world on a map with rules (the standard rules when
/// NULL) and a seed for its random streams, and prepares the map for
/// paths. The map and rules must outlive the world.
void vasal_engine_world_start(vasal_engine_world_s* const world,
	const vasal_engine_map_s* const map,
	const vasal_engine_rules_s* const rules, const uint64_t seed);

/// Adds a vasal whose groups all stand at a tile, with no troops. Returns
/// the vasal's index, or vasal_engine_world_vasals_max when the world is
/// full or the tile is impassable or off the map.
size_t vasal_engine_world_add_vasal(vasal_engine_world_s* const world,
	const vasal_engine_map_point_s at);

/// Adds a troop of the first unit type, at the given speed, to a group,
/// at the group's anchor. Returns false when the group is full, or the
/// vasal or the group does not exist.
bool_t vasal_engine_world_add_troop(vasal_engine_world_s* const world,
	const size_t vasal, const size_t group, const uint16_t speed);

/// Adds a troop of a unit type of the rules, at its speed, to a group, as
/// vasal_engine_world_add_troop.
bool_t vasal_engine_world_add_unit(vasal_engine_world_s* const world,
	const size_t vasal, const size_t group, const size_t unit);

/// Gives a vasal the rules' starting army: its vintaines and its
/// centenar.
void vasal_engine_world_add_army(vasal_engine_world_s* const world,
	const size_t vasal);

/// Adds a band of bandits guarding a camp: an NPC vasal at the camp with
/// the rules' camp vintaine in guard mode. Returns its index, or
/// vasal_engine_world_vasals_max when there is no room.
size_t vasal_engine_world_add_camp(vasal_engine_world_s* const world,
	const vasal_engine_map_point_s at);

/// Sets the mode of the groups in the mask; their post becomes where they
/// stand.
void vasal_engine_world_set_mode(vasal_engine_world_s* const world,
	const size_t vasal, const uint32_t mask,
	const vasal_engine_world_mode_e mode);

/// Sends a group's anchor along a path to a tile without touching its
/// orders, queue or post: what fights do, toward or away from an enemy.
void vasal_engine_world_steer(vasal_engine_world_s* const world,
	const size_t vasal, const size_t group,
	const vasal_engine_map_point_s to);

/// Whether two vasals are enemies: of different alliances.
bool_t vasal_engine_world_enemies(const vasal_engine_world_s* const world,
	const size_t a, const size_t b);

/// Orders the groups set in the mask (bit i for group i) to move to a
/// tile; several groups each form up on their own spot next to it. The
/// order replaces their current order and their queue. Returns false,
/// changing nothing, when the tile is off the map or a group in the mask
/// has no troops.
bool_t vasal_engine_world_order_move(vasal_engine_world_s* const world,
	const size_t vasal, const uint32_t mask,
	const vasal_engine_map_point_s to);

/// Like vasal_engine_world_order_move, but queues the move after the
/// orders the groups already have; an idle group starts it at once. Also
/// returns false, changing nothing, when a group's queue is full.
bool_t vasal_engine_world_queue_move(vasal_engine_world_s* const world,
	const size_t vasal, const uint32_t mask,
	const vasal_engine_map_point_s to);

/// Whether a group in the mask can queue no more orders.
bool_t vasal_engine_world_queue_full(const vasal_engine_world_s* const world,
	const size_t vasal, const uint32_t mask);

/// Sets the pace of the groups in the mask.
void vasal_engine_world_set_pace(vasal_engine_world_s* const world,
	const size_t vasal, const uint32_t mask,
	const vasal_engine_world_pace_e pace);

/// Advances the world by one tick.
void vasal_engine_world_tick(vasal_engine_world_s* const world);

#endif
