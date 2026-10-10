
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

/// A troop.
typedef struct vasal_engine_world_troop_s {
	vasal_engine_world_position_s position;
	uint32_t                      id;
	/// Its speed, in 1/vasal_engine_map_precision of a tile per tick, on
	/// terrain of 100 percent speed.
	uint16_t                      speed;
} vasal_engine_world_troop_s;

/// A vintaine, or the centenar's group.
typedef struct vasal_engine_world_group_s {
	vasal_engine_world_troop_s    troops[vasal_engine_world_troops];
	/// The point the group forms up around, moving along its path.
	vasal_engine_world_position_s anchor;
	vasal_engine_map_point_s      waypoints[vasal_engine_world_waypoints];
	uint16_t                      waypoint_count;
	uint16_t                      waypoint_next;
	/// Where the group goes after its current order, in order.
	vasal_engine_map_point_s      queue[vasal_engine_world_queue_max];
	uint8_t                       queue_count;
	uint8_t                       troop_count;
	vasal_engine_world_pace_e     pace;
	bool_t                        moving;
} vasal_engine_world_group_s;

/// A vasal and their centaine.
typedef struct vasal_engine_world_vasal_s {
	vasal_engine_world_group_s groups[vasal_engine_world_groups];
	uint32_t                   id;
} vasal_engine_world_vasal_s;

/// The world.
typedef struct vasal_engine_world_s {
	vasal_engine_world_vasal_s vasals[vasal_engine_world_vasals_max];
	const vasal_engine_map_s*  map;
	uint32_t                   tick;
	/// The next id to give; ids are never reused.
	uint32_t                   next_id;
	uint8_t                    vasal_count;
} vasal_engine_world_s;

/// Starts an empty world on a map, and prepares the map for paths.
void vasal_engine_world_start(vasal_engine_world_s* const world,
	const vasal_engine_map_s* const map);

/// Adds a vasal whose groups all stand at a tile, with no troops. Returns
/// the vasal's index, or vasal_engine_world_vasals_max when the world is
/// full or the tile is impassable or off the map.
size_t vasal_engine_world_add_vasal(vasal_engine_world_s* const world,
	const vasal_engine_map_point_s at);

/// Adds a troop of the given speed to a group, at the group's anchor.
/// Returns false when the group is full, or the vasal or the group does
/// not exist.
bool_t vasal_engine_world_add_troop(vasal_engine_world_s* const world,
	const size_t vasal, const size_t group, const uint16_t speed);

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
