
/// The map: the descriptor of everything on it, all set in the editor. Its
/// own terrain types with their passability and speed, the terrain of every
/// tile, settlements with their values and flags, spawn areas, bandit camps
/// and decorations. Every count has a hard limit, so a map lives in static
/// memory.

#ifndef vasal__engine__map_h
#define vasal__engine__map_h

#include "engine/types.h"

/// The longest side of a map, in tiles.
#define vasal_engine_map_side_max 2048U

/// Positions are fixed-point numbers: this many steps make one tile.
#define vasal_engine_map_precision 256

/// Hard limits of the things a map holds.
#define vasal_engine_map_terrains_max 32U
#define vasal_engine_map_settlements_max 1024U
#define vasal_engine_map_spawns_max 256U
#define vasal_engine_map_camps_max 256U
#define vasal_engine_map_decorations_max 16384U

/// The longest name, including its terminating zero.
#define vasal_engine_map_name_capacity 32U

/// The fastest a terrain type may be, in percent of a troop's speed.
#define vasal_engine_map_speed_max 200U

/// The farthest troops on a terrain type may see, in percent of their
/// sight.
#define vasal_engine_map_sight_max 200U

/// Settlement types.
typedef enum vasal_engine_map_settlement_type_e {
	vasal_engine_map_settlement_village,
	vasal_engine_map_settlement_town,
	vasal_engine_map_settlement_castle,
	vasal_engine_map_settlement_city,
	vasal_engine_map_settlement_types
} vasal_engine_map_settlement_type_e;

/// Settlement flags: whether it has centenars, and which troop types it
/// recruits.
#define vasal_engine_map_flag_centenars 0x01U
#define vasal_engine_map_flag_spearman 0x02U
#define vasal_engine_map_flag_swordsman 0x04U
#define vasal_engine_map_flag_archer 0x08U
#define vasal_engine_map_flag_thrower 0x10U
#define vasal_engine_map_flag_scout 0x20U
/// Every flag a settlement may have.
#define vasal_engine_map_flags 0x3fU

/// A tile's coordinates.
typedef struct vasal_engine_map_point_s {
	uint16_t x;
	uint16_t y;
} vasal_engine_map_point_s;

/// A terrain type of this map.
typedef struct vasal_engine_map_terrain_s {
	char     name[vasal_engine_map_name_capacity];
	/// The color it is drawn in: 0xRRGGBBAA.
	uint32_t color;
	/// How fast troops cross it, in percent of their speed; 0 when it is
	/// impassable.
	uint16_t speed;
	/// How far troops standing on it see, in percent of their sight:
	/// forest and tall grass hide what is around.
	uint16_t sight;
	bool_t   passable;
} vasal_engine_map_terrain_s;

/// A settlement, with the values the editor set for it.
typedef struct vasal_engine_map_settlement_s {
	char                     name[vasal_engine_map_name_capacity];
	vasal_engine_map_point_s position;
	/// The capture area's radius, in tiles.
	uint16_t                 radius;
	/// Troops a side needs in the capture area to capture it.
	uint16_t                 requirement;
	/// Ticks a capture takes.
	uint32_t                 capture_time;
	/// Gold shared by the participants of a capture.
	uint32_t                 bonus;
	/// Gold it adds to its alliance's payout.
	uint32_t                 income;
	/// Its flags: which troops it recruits, whether it has centenars.
	uint32_t                 flags;
	/// Its type (village, town, castle, city, ...).
	uint8_t                  type;
} vasal_engine_map_settlement_s;

/// An area new vasals spawn in.
typedef struct vasal_engine_map_spawn_s {
	vasal_engine_map_point_s position;
	/// Its radius, in tiles.
	uint16_t                 radius;
} vasal_engine_map_spawn_s;

/// A bandit camp.
typedef struct vasal_engine_map_camp_s {
	vasal_engine_map_point_s position;
} vasal_engine_map_camp_s;

/// A decoration: drawn only, it changes nothing.
typedef struct vasal_engine_map_decoration_s {
	vasal_engine_map_point_s position;
	uint16_t                 kind;
} vasal_engine_map_decoration_s;

/// A map.
typedef struct vasal_engine_map_s {
	char                          name[vasal_engine_map_name_capacity];
	uint16_t                      width;
	uint16_t                      height;
	vasal_engine_map_terrain_s
		terrains[vasal_engine_map_terrains_max];
	uint8_t                       terrain_count;
	/// The terrain type of every tile, row by row.
	uint8_t
		tiles[vasal_engine_map_side_max * vasal_engine_map_side_max];
	vasal_engine_map_settlement_s
		settlements[vasal_engine_map_settlements_max];
	uint16_t                      settlement_count;
	vasal_engine_map_spawn_s      spawns[vasal_engine_map_spawns_max];
	uint16_t                      spawn_count;
	vasal_engine_map_camp_s       camps[vasal_engine_map_camps_max];
	uint16_t                      camp_count;
	vasal_engine_map_decoration_s
		decorations[vasal_engine_map_decorations_max];
	uint16_t                      decoration_count;
} vasal_engine_map_s;

/// What can be wrong with a map that reads fine, for the editor and the
/// server to report.
typedef enum vasal_engine_map_problem_e {
	/// The map has no settlement.
	vasal_engine_map_problem_no_settlement,
	/// The map has no spawn area.
	vasal_engine_map_problem_no_spawn,
	/// A settlement stands on an impassable tile.
	vasal_engine_map_problem_settlement_impassable,
	/// A spawn area's center is on an impassable tile.
	vasal_engine_map_problem_spawn_impassable,
	/// Two settlements' capture areas overlap.
	vasal_engine_map_problem_capture_overlap
} vasal_engine_map_problem_e;

/// A problem and the index of what it is about (a settlement or spawn
/// area), 0 when it is about the whole map.
typedef struct vasal_engine_map_finding_s {
	vasal_engine_map_problem_e problem;
	uint16_t                   index;
} vasal_engine_map_finding_s;

/// The standard terrain types, in the order
/// vasal_engine_map_standard_terrains gives them.
typedef enum vasal_engine_map_standard_e {
	vasal_engine_map_standard_grass,
	vasal_engine_map_standard_short_grass,
	vasal_engine_map_standard_tall_grass,
	vasal_engine_map_standard_forest,
	vasal_engine_map_standard_road,
	vasal_engine_map_standard_water,
	vasal_engine_map_standards
} vasal_engine_map_standard_e;

/// Gives the map the standard terrain types in place of its own: grass,
/// short grass, tall grass, forest, road and water, with their colors,
/// speeds and sights (placeholders until tuning). Tiles keep their numbers,
/// so a cleared map stays grass. New maps of the editor and the
/// demonstration map start with them; a map may change them all.
void vasal_engine_map_standard_terrains(vasal_engine_map_s* const map);

/// Makes the map empty, width by height tiles of one passable terrain type
/// named "grass". Returns false, leaving the map unchanged, when a side is
/// 0 or longer than vasal_engine_map_side_max.
bool_t vasal_engine_map_clear(vasal_engine_map_s* const map,
	const uint16_t width, const uint16_t height);

/// Gives a settlement its type and that type's default values, which the
/// editor may then change (placeholders until the settlement types are
/// tuned). The name and position are left as they are.
void vasal_engine_map_settlement_defaults(
	vasal_engine_map_settlement_s* const settlement,
	const vasal_engine_map_settlement_type_e type);

/// Whether the tile is on the map.
bool_t vasal_engine_map_contains(const vasal_engine_map_s* const map,
	const uint16_t x, const uint16_t y);

/// The terrain type of a tile on the map.
const vasal_engine_map_terrain_s* vasal_engine_map_terrain(
	const vasal_engine_map_s* const map, const uint16_t x,
	const uint16_t y);

/// Checks what the map needs to be played on, writing up to capacity
/// findings. Returns how many problems there are, which may be more than
/// were written.
size_t vasal_engine_map_check(const vasal_engine_map_s* const map,
	vasal_engine_map_finding_s* const findings, const size_t capacity);

#endif
