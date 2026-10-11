
/// The edits the map view and the panels make to the map being edited
/// (editor/document.h), apart from drawing: finding, adding, copying and
/// deleting objects, painting with a brush, along a line or by filling an
/// area, and adding and removing terrain types. Every function that
/// changes the map does it inside the document operation the caller has
/// begun, unless it says otherwise.

#ifndef vasal__editor__edits_h
#define vasal__editor__edits_h

#include "engine/rules.h"

/// The kinds of objects on a map.
typedef enum vasal_editor_edits_kind_e {
	vasal_editor_edits_settlement,
	vasal_editor_edits_spawn,
	vasal_editor_edits_camp,
	vasal_editor_edits_decoration,
	vasal_editor_edits_kinds
} vasal_editor_edits_kind_e;

/// A brush: its radius in tiles (0 paints one tile) and whether it is a
/// square rather than a disc.
typedef struct vasal_editor_edits_brush_s {
	int32_t radius;
	bool_t  square;
} vasal_editor_edits_brush_s;

/// The largest brush radius.
#define vasal_editor_edits_brush_max 32

/// How many objects of a kind the map has, and how many it can hold.
uint16_t vasal_editor_edits_count(const vasal_editor_edits_kind_e kind);
uint16_t vasal_editor_edits_capacity(const vasal_editor_edits_kind_e kind);

/// Where an object stands.
vasal_engine_map_point_s* vasal_editor_edits_position(
	const vasal_editor_edits_kind_e kind, const uint16_t index);

/// The object of a kind nearest a point in tiles, within reach tiles of
/// its tile's middle. Returns false when there is none.
bool_t vasal_editor_edits_pick(const vasal_editor_edits_kind_e kind,
	const float x, const float y, const float reach, uint16_t* const index);

/// The object of any kind nearest a point, within reach; settlements win
/// over camps, camps over spawn areas and those over decorations at the
/// same distance. Returns false when there is none.
bool_t vasal_editor_edits_pick_any(const float x, const float y,
	const float reach, vasal_editor_edits_kind_e* const kind,
	uint16_t* const index);

/// Adds an object of a kind at a tile: a settlement of the type with the
/// type's defaults from the rules, a spawn area of radius 6, a camp, or a
/// decoration of the kind given. Returns false when the kind is full;
/// index receives the new object's.
bool_t vasal_editor_edits_add(const vasal_editor_edits_kind_e kind,
	const vasal_engine_map_point_s at, const vasal_engine_rules_s* const rules,
	const vasal_engine_map_settlement_type_e type, const uint16_t decoration,
	uint16_t* const index);

/// Adds a copy of an object at a tile. Returns false when the kind is
/// full; index receives the copy's.
bool_t vasal_editor_edits_duplicate(const vasal_editor_edits_kind_e kind,
	const uint16_t original, const vasal_engine_map_point_s at,
	uint16_t* const index);

/// Deletes an object; the kind's last object takes its index.
void vasal_editor_edits_remove(const vasal_editor_edits_kind_e kind,
	const uint16_t index);

/// Paints the brush with a terrain type around a tile, which may be off
/// the map.
void vasal_editor_edits_stamp(const vasal_editor_edits_brush_s brush,
	const int32_t x, const int32_t y, const uint8_t terrain);

/// Paints the brush along a straight line from one tile to another.
void vasal_editor_edits_line(const vasal_editor_edits_brush_s brush,
	const int32_t from_x, const int32_t from_y, const int32_t to_x,
	const int32_t to_y, const uint8_t terrain);

/// The outcome of filling.
typedef enum vasal_editor_edits_fill_e {
	/// The area was filled.
	vasal_editor_edits_fill_done,
	/// The area already is of the terrain type, or the tile is off the
	/// map: nothing changed.
	vasal_editor_edits_fill_same,
	/// The area holds more tiles than one operation can change: nothing
	/// changed.
	vasal_editor_edits_fill_large
} vasal_editor_edits_fill_e;

/// Fills the area of a tile, the tiles of its terrain type joined to it
/// through their sides, with a terrain type. filled receives how many
/// tiles changed.
vasal_editor_edits_fill_e vasal_editor_edits_fill(const uint16_t x,
	const uint16_t y, const uint8_t terrain, size_t* const filled);

/// Adds a terrain type, passable at full speed and sight, named "new
/// terrain". Returns false when there are vasal_engine_map_terrains_max;
/// index receives the new type's.
bool_t vasal_editor_edits_add_terrain(uint8_t* const index);

/// Removes a terrain type other than the first: its tiles become the
/// first type, and later types move down by one, their tiles with them.
/// Returns false when it can't be removed.
bool_t vasal_editor_edits_remove_terrain(const uint8_t terrain);

/// How many tiles of each terrain type the map has, into counts (one per
/// type).
void vasal_editor_edits_census(size_t* const counts);

#endif
