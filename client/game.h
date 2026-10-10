
/// The player's side of the game: the world as the client simulates it,
/// the player's vasal, which groups are selected and which one is focused,
/// the current action, and short notices. Until the server runs the match
/// (milestone 3), the client simulates the world itself.

#ifndef vasal__client__game_h
#define vasal__client__game_h

#include "engine/world.h"

/// The ordinary sight radius of a troop, in tiles (placeholder until
/// tuning).
#define vasal_client_game_sight 18.0f

/// What a right-click orders. Only move for now; the attacks come with
/// combat (vintaines.actions#6).
typedef enum vasal_client_game_action_e {
	vasal_client_game_action_move,
	vasal_client_game_action_count
} vasal_client_game_action_e;

/// The game.
typedef struct vasal_client_game_s {
	vasal_engine_world_s*      world;
	/// The player's vasal in the world.
	size_t                     vasal;
	/// The selected groups, bit i for group i.
	uint32_t                   selection;
	/// The focused group.
	size_t                     focus;
	vasal_client_game_action_e action;
	/// The notice shown to the player, and how long it still shows.
	const char*                notice;
	float                      notice_seconds;
	/// Time not yet simulated, in seconds.
	float                      pending;
	/// Every vasal's troops and anchors at the tick before the last, so
	/// frames between ticks are drawn between the two.
	vasal_engine_world_position_s previous_troops
		[vasal_engine_world_vasals_max][vasal_engine_world_groups]
		[vasal_engine_world_troops];
	vasal_engine_world_position_s previous_anchors
		[vasal_engine_world_vasals_max][vasal_engine_world_groups];
} vasal_client_game_s;

/// Starts a game on the map with the player's centaine at the map's first
/// spawn area.
void vasal_client_game_start(vasal_client_game_s* const game,
	const vasal_engine_map_s* const map);

/// Simulates the time passed since the last update, in whole ticks.
void vasal_client_game_update(vasal_client_game_s* const game,
	const float seconds);

/// One of the player's groups.
const vasal_engine_world_group_s* vasal_client_game_group(
	const vasal_client_game_s* const game, const size_t group);

/// Where a troop of the player's is drawn now, in tiles: between its
/// positions of the last two ticks, as far as time has passed since the
/// last one. The world moves 20 times a second, the screen 60 or more.
void vasal_client_game_troop_at(const vasal_client_game_s* const game,
	const size_t group, const size_t troop, float* const x, float* const y);

/// Where a troop of any vasal is drawn now, in tiles, likewise.
void vasal_client_game_position_at(const vasal_client_game_s* const game,
	const size_t vasal, const size_t group, const size_t troop,
	float* const x, float* const y);

/// Where a group's anchor is drawn now, in tiles, likewise.
void vasal_client_game_anchor_at(const vasal_client_game_s* const game,
	const size_t group, float* const x, float* const y);

/// Selects a group: alone, or with adding, adds it to the selection or
/// removes it if it is in it. A group selected either way is focused.
/// Empty groups can't be selected. Returns whether the selection changed.
bool_t vasal_client_game_select(vasal_client_game_s* const game,
	const size_t group, const bool_t adding);

/// Where a troop of the player's is drawn now, in tiles, and how far it
/// sees: the ordinary sight times the sight of the terrain it stands on.
float vasal_client_game_eye(const vasal_client_game_s* const game,
	const size_t group, const size_t troop, float* const x, float* const y);

/// How far a point, in tiles, is inside the player's sight: 1 well inside
/// a troop's sight, 0 outside every troop's, fading over the last tile of
/// each troop's radius.
float vasal_client_game_sight_at(const vasal_client_game_s* const game,
	const float x, const float y);

/// Selects every group with troops and focuses one of them.
void vasal_client_game_select_all(vasal_client_game_s* const game,
	const size_t focus);

/// Moves the focus to the next selected group, in the order of the keys.
void vasal_client_game_cycle_focus(vasal_client_game_s* const game);

/// Switches the marching pace of the selected groups: at the run when
/// every one of them marches in step, else in step. Their orders and queues
/// stay as they are.
void vasal_client_game_toggle_pace(vasal_client_game_s* const game);

/// Gives the selected groups an order of the action toward a tile, which
/// may be off the map (the order is then refused with a notice). A queued
/// order comes after the orders they have; otherwise it replaces them.
void vasal_client_game_order(vasal_client_game_s* const game,
	const int32_t x, const int32_t y,
	const vasal_client_game_action_e action, const bool_t queued);

/// Shows a notice for a few seconds.
void vasal_client_game_notify(vasal_client_game_s* const game,
	const char* const text);

#endif
