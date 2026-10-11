
/// What the editor's parts share: the page shown, the tool in use, what is
/// selected, the view of the map, the open dialog, the server to upload
/// to, and the message shown to the user.

#ifndef vasal__editor__state_h
#define vasal__editor__state_h

#include "editor/edits.h"

/// The pages: the map, the rules, and the server.
typedef enum vasal_editor_state_tab_e {
	vasal_editor_state_tab_map,
	vasal_editor_state_tab_rules,
	vasal_editor_state_tab_server,
	vasal_editor_state_tabs
} vasal_editor_state_tab_e;

/// The map tools. The object tools come in the order of
/// vasal_editor_edits_kind_e.
typedef enum vasal_editor_state_tool_e {
	vasal_editor_state_tool_select,
	vasal_editor_state_tool_paint,
	vasal_editor_state_tool_fill,
	vasal_editor_state_tool_pick,
	vasal_editor_state_tool_settlement,
	vasal_editor_state_tool_spawn,
	vasal_editor_state_tool_camp,
	vasal_editor_state_tool_decoration,
	vasal_editor_state_tools
} vasal_editor_state_tool_e;

/// What the map view colors the ground by.
typedef enum vasal_editor_state_overlay_e {
	vasal_editor_state_overlay_terrain,
	vasal_editor_state_overlay_speed,
	vasal_editor_state_overlay_sight,
	vasal_editor_state_overlays
} vasal_editor_state_overlay_e;

/// The dialogs.
typedef enum vasal_editor_state_dialog_e {
	vasal_editor_state_dialog_none,
	/// A new map's name and size.
	vasal_editor_state_dialog_new_map,
	/// The keys and mouse buttons.
	vasal_editor_state_dialog_help,
	/// Whether to replace the map, or the rules, with unsaved changes.
	vasal_editor_state_dialog_discard
} vasal_editor_state_dialog_e;

/// What waits on the discard dialog.
typedef enum vasal_editor_state_pending_e {
	vasal_editor_state_pending_none,
	vasal_editor_state_pending_new_map,
	vasal_editor_state_pending_new_rules,
	vasal_editor_state_pending_open
} vasal_editor_state_pending_e;

/// What the user asked for, which the editor does at the end of the frame.
typedef enum vasal_editor_state_command_e {
	vasal_editor_state_command_none,
	vasal_editor_state_command_new_map,
	vasal_editor_state_command_new_rules,
	vasal_editor_state_command_open,
	vasal_editor_state_command_save,
	vasal_editor_state_command_save_map,
	vasal_editor_state_command_save_rules,
	vasal_editor_state_command_upload_map,
	vasal_editor_state_command_upload_rules,
	vasal_editor_state_command_upload_both,
	vasal_editor_state_command_fetch_map,
	vasal_editor_state_command_fetch_rules,
	vasal_editor_state_command_undo,
	vasal_editor_state_command_redo,
	vasal_editor_state_command_fit,
	vasal_editor_state_commands
} vasal_editor_state_command_e;

/// The pages of the rules.
typedef enum vasal_editor_state_page_e {
	vasal_editor_state_page_general,
	vasal_editor_state_page_unit,
	vasal_editor_state_page_army,
	vasal_editor_state_page_camp,
	vasal_editor_state_page_settlements,
	vasal_editor_state_page_combat,
	vasal_editor_state_pages
} vasal_editor_state_page_e;

/// How many bytes a server address and an admin key take.
#define vasal_editor_state_address_capacity 128U
#define vasal_editor_state_key_capacity 64U

/// How many bytes a message takes.
#define vasal_editor_state_message_capacity 200U

/// The editor's state.
typedef struct vasal_editor_state_s {
	vasal_editor_state_tab_e           tab;
	vasal_editor_state_tool_e          tool;
	/// The terrain type painted, and the brush.
	uint8_t                            terrain;
	vasal_editor_edits_brush_s         brush;
	/// The type of new settlements, and the kind of new decorations.
	vasal_engine_map_settlement_type_e settlement_type;
	uint16_t                           decoration_kind;
	/// The selected object, if any.
	bool_t                             selected;
	vasal_editor_edits_kind_e          selection_kind;
	uint16_t                           selection;
	/// Whether the left button is held on the map (painting, or moving
	/// the selection), and the last tile painted.
	bool_t                             dragging;
	bool_t                             painted;
	int32_t                            painted_x;
	int32_t                            painted_y;
	/// The tile under the pointer, when it is over the map.
	bool_t                             hovering;
	int32_t                            hover_x;
	int32_t                            hover_y;
	/// The tile at the middle of the map view, and pixels per tile.
	float                              view_x;
	float                              view_y;
	float                              zoom;
	/// How the map view looks.
	vasal_editor_state_overlay_e       overlay;
	bool_t                             grid;
	bool_t                             minimap;
	/// What the user asked for this frame.
	vasal_editor_state_command_e       command;
	/// The open dialog, and what waits on it.
	vasal_editor_state_dialog_e        dialog;
	vasal_editor_state_pending_e       pending;
	/// The rules page shown, and the unit type on it.
	vasal_editor_state_page_e          page;
	uint8_t                            unit;
	/// The server to upload to, and its admin key.
	char        address[vasal_editor_state_address_capacity];
	char        key[vasal_editor_state_key_capacity];
	/// Whether the key is remembered in the browser.
	bool_t                             remember_key;
	/// The revisions of the map and the rules when they were last saved
	/// or opened, to tell whether they have unsaved changes.
	uint32_t                           map_saved;
	uint32_t                           rules_saved;
	/// The message shown to the user, how long it still shows, and
	/// whether it tells of a failure.
	char        message[vasal_editor_state_message_capacity];
	float                              message_seconds;
	bool_t                             failure;
} vasal_editor_state_s;

/// Shows a message for a few seconds; the text is copied, cut to fit.
void vasal_editor_state_say(vasal_editor_state_s* const state,
	const char* const text, const bool_t failure);

/// Selects an object, or nothing when selected is false.
void vasal_editor_state_select(vasal_editor_state_s* const state,
	const bool_t selected, const vasal_editor_edits_kind_e kind,
	const uint16_t index);

/// Whether the map, or the rules, have changes not saved.
bool_t vasal_editor_state_map_unsaved(const vasal_editor_state_s* const state);
bool_t vasal_editor_state_rules_unsaved(
	const vasal_editor_state_s* const state);

/// The object tool of a kind, and the kind of an object tool (the
/// settlement kind for other tools).
vasal_editor_state_tool_e vasal_editor_state_kind_tool(
	const vasal_editor_edits_kind_e kind);
vasal_editor_edits_kind_e vasal_editor_state_tool_kind(
	const vasal_editor_state_tool_e tool);

/// Whether a tool places objects.
bool_t vasal_editor_state_places(const vasal_editor_state_tool_e tool);

#endif
