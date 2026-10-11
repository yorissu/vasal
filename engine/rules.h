
/// The game's rules: everything about troops and combat that a designer
/// decides, kept out of the code. A rules file holds the unit types with
/// their values and behaviour, the settlement types' default values, the
/// army a vasal starts with, the bandits of a camp, and the constants of
/// combat. The server loads one and hands it to every client that joins,
/// with the map; the editor makes them. The engine has mechanisms, never
/// numbers or names of its own: it reads them all from here.
///
/// Every value is a field of a table below, with its label, help text and
/// range, so the file reader checks it and the editor shows it without
/// knowing it.
///
/// Layout of a rules file: the magic word "vasalrul", the format version
/// (u16), the rules' name, the unit types (a u8 count, then each one's
/// name, color u32 and fields u32), the combat fields (u32 each), the
/// settlement types' fields (u32 each, 4 types), the starting army (5
/// vintaines of 4 slots, then the centenar's unit type, u8), the bandit
/// camp (4 slots), and last a CRC-32 (u32) of everything before it. A slot
/// is a unit type and a count (u8 each); a unit type of
/// vasal_engine_rules_none leaves it empty. Names take
/// vasal_engine_map_name_capacity bytes, zero-padded; little-endian.

#ifndef vasal__engine__rules_h
#define vasal__engine__rules_h

#include "engine/map.h"

/// The format version this code writes and reads.
#define vasal_engine_rules_version 1U

/// The most unit types: bit 0 of a settlement's flags is for centenars,
/// the others for unit types.
#define vasal_engine_rules_units_max 30U

/// No unit type, in a slot or for the centenar.
#define vasal_engine_rules_none 0xffU

/// Slots of a vintaine of the starting army, and of a bandit camp.
#define vasal_engine_rules_slots 4U

/// Vintaines of the starting army.
#define vasal_engine_rules_vintaines 5U

/// The most troops of a vintaine, as the world holds them.
#define vasal_engine_rules_vintaine_max 20U

/// The largest rules file, in bytes.
#define vasal_engine_rules_capacity 8192U

/// The fields of a unit type. Distances are in tiles unless named
/// otherwise, times in ticks, chances in percent.
typedef enum vasal_engine_rules_unit_e {
	/// 1 for a ranged type, 0 for melee.
	vasal_engine_rules_unit_ranged,
	/// Its speed, in 1/vasal_engine_map_precision of a tile per tick.
	vasal_engine_rules_unit_speed,
	/// How far it sees.
	vasal_engine_rules_unit_sight,
	/// Its skill, which weighs its first strike, parry and aim.
	vasal_engine_rules_unit_skill,
	/// Its chance to parry a strike, against equal skill.
	vasal_engine_rules_unit_parry,
	/// Its chance to block a shot that hits.
	vasal_engine_rules_unit_block,
	/// The ticks between its strikes in a duel.
	vasal_engine_rules_unit_interval,
	/// The chance that a strike or shot of it that lands kills; else it
	/// wounds, and a second wound kills.
	vasal_engine_rules_unit_kill,
	/// How far it shoots (ranged types).
	vasal_engine_rules_unit_range,
	/// The ticks between its shots (ranged types).
	vasal_engine_rules_unit_reload,
	/// How much a duel lowers its chances when it is ranged and caught in
	/// melee.
	vasal_engine_rules_unit_sidearm,
	/// How far it leaves its place in the formation to reach an enemy.
	vasal_engine_rules_unit_chase,
	/// 1 when a wounded troop of it leaves fights and keeps to its place.
	vasal_engine_rules_unit_careful,
	/// What it costs to recruit, and its salary, in gold.
	vasal_engine_rules_unit_cost,
	vasal_engine_rules_unit_salary,
	vasal_engine_rules_unit_fields
} vasal_engine_rules_unit_e;

/// The combat fields.
typedef enum vasal_engine_rules_combat_e {
	/// The distance at which two troops lock into a duel, in
	/// 1/vasal_engine_map_precision of a tile.
	vasal_engine_rules_combat_duel,
	/// How long a troop stays in combat after shooting or being shot at.
	vasal_engine_rules_combat_linger,
	/// How far aggressive and fleeing vintaines look for enemies.
	vasal_engine_rules_combat_sight,
	/// The radius around a guarding vintaine's post it defends.
	vasal_engine_rules_combat_guard,
	/// A shot's chance to hit at skill 0; skill adds half of itself.
	vasal_engine_rules_combat_aim,
	vasal_engine_rules_combat_fields
} vasal_engine_rules_combat_e;

/// The fields of a settlement type: the default values of a new
/// settlement of the type.
typedef enum vasal_engine_rules_settlement_e {
	vasal_engine_rules_settlement_radius,
	vasal_engine_rules_settlement_requirement,
	vasal_engine_rules_settlement_capture,
	vasal_engine_rules_settlement_bonus,
	vasal_engine_rules_settlement_income,
	/// What it offers: bit 0 centenars, bit 1 + i unit type i.
	vasal_engine_rules_settlement_flags,
	vasal_engine_rules_settlement_fields
} vasal_engine_rules_settlement_e;

/// A field's description, for the reader's checks and the editor.
typedef struct vasal_engine_rules_field_s {
	const char* label;
	const char* help;
	uint32_t    minimum;
	uint32_t    maximum;
} vasal_engine_rules_field_s;

/// A unit type.
typedef struct vasal_engine_rules_unit_s {
	char     name[vasal_engine_map_name_capacity];
	/// The color it is drawn in until it has art: 0xRRGGBBAA.
	uint32_t color;
	uint32_t values[vasal_engine_rules_unit_fields];
} vasal_engine_rules_unit_s;

/// A number of troops of a unit type.
typedef struct vasal_engine_rules_slot_s {
	uint8_t unit;
	uint8_t count;
} vasal_engine_rules_slot_s;

/// The rules.
typedef struct vasal_engine_rules_s {
	uint32_t                  combat[vasal_engine_rules_combat_fields];
	uint32_t                  settlements[vasal_engine_map_settlement_types]
		[vasal_engine_rules_settlement_fields];
	vasal_engine_rules_unit_s units[vasal_engine_rules_units_max];
	uint8_t                   unit_count;
	/// The centenar's unit type of the starting army
	/// (vasal_engine_rules_none for none).
	uint8_t                   centenar;
	/// The bandits of each camp, one vintaine.
	vasal_engine_rules_slot_s camp[vasal_engine_rules_slots];
	char                      name[vasal_engine_map_name_capacity];
	/// The army a vasal starts with, vintaine by vintaine.
	vasal_engine_rules_slot_s start[vasal_engine_rules_vintaines]
		[vasal_engine_rules_slots];
} vasal_engine_rules_s;

/// The outcome of reading a rules file; the same as for map files.
typedef enum vasal_engine_rules_read_e {
	vasal_engine_rules_read_ok,
	vasal_engine_rules_read_magic,
	vasal_engine_rules_read_version,
	vasal_engine_rules_read_length,
	vasal_engine_rules_read_checksum,
	vasal_engine_rules_read_value
} vasal_engine_rules_read_e;

/// The descriptions of the fields.
const vasal_engine_rules_field_s* vasal_engine_rules_unit_field(
	const vasal_engine_rules_unit_e field);
const vasal_engine_rules_field_s* vasal_engine_rules_combat_field(
	const vasal_engine_rules_combat_e field);
const vasal_engine_rules_field_s* vasal_engine_rules_settlement_field(
	const vasal_engine_rules_settlement_e field);

/// Gives the rules the standard values: the first build's unit types
/// (spearman, swordsman, archer, thrower, scout, captain and bandit) and
/// everything else (placeholders until tuning). Clients use them until the
/// server sends rules, and the editor starts from them.
void vasal_engine_rules_standard(vasal_engine_rules_s* const rules);

/// Whether the rules hold together: every value within its field's range,
/// every slot and the centenar of a unit type that exists, no vintaine of
/// the starting army or camp above vasal_engine_rules_vintaine_max troops,
/// and no settlement flag for a unit type that doesn't exist.
bool_t vasal_engine_rules_valid(const vasal_engine_rules_s* const rules);

/// Writes the rules into buffer. Returns the file's size, or 0 when it
/// does not fit or the rules are not valid.
size_t vasal_engine_rules_write(const vasal_engine_rules_s* const rules,
	uint8_t* const buffer, const size_t capacity);

/// Reads a rules file of size bytes into rules, checking everything.
/// Unless the outcome is ok, the rules are undefined. version receives the
/// file's version.
vasal_engine_rules_read_e vasal_engine_rules_read(const uint8_t* const data,
	const size_t size, vasal_engine_rules_s* const rules,
	uint16_t* const version);

/// Whether two rules are the same: names up to their end, the unit types
/// that exist, and every value.
bool_t vasal_engine_rules_equal(const vasal_engine_rules_s* const a,
	const vasal_engine_rules_s* const b);

/// A unit type's field.
uint32_t vasal_engine_rules_get(const vasal_engine_rules_s* const rules,
	const size_t unit, const vasal_engine_rules_unit_e field);

/// Gives a settlement its type and that type's default values from the
/// rules. The name and position are left as they are.
void vasal_engine_rules_settlement_defaults(
	const vasal_engine_rules_s* const rules,
	vasal_engine_map_settlement_s* const settlement,
	const vasal_engine_map_settlement_type_e type);

#endif
