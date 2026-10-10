
/// What the editor's parts share: the tool in use, what is selected, the
/// view of the map, and the message shown to the user.

#ifndef vasal__editor__state_h
#define vasal__editor__state_h

#include "engine/map.h"

/// The tools.
typedef enum vasal_editor_state_tool_e {
	vasal_editor_state_tool_paint,
	vasal_editor_state_tool_settlement,
	vasal_editor_state_tool_spawn,
	vasal_editor_state_tool_camp,
	vasal_editor_state_tool_decoration,
	vasal_editor_state_tools
} vasal_editor_state_tool_e;

/// The editor's state.
typedef struct vasal_editor_state_s {
	vasal_editor_state_tool_e          tool;
	/// The terrain type painted, and the brush's radius in tiles.
	uint8_t                            terrain;
	int                                brush;
	/// The type of new settlements, and the kind of new decorations.
	vasal_engine_map_settlement_type_e settlement_type;
	uint16_t                           decoration_kind;
	/// The selected object of the current tool's kind, if any.
	bool_t                             selected;
	uint16_t                           selection;
	/// Whether a drag (painting, or moving the selection) is going on.
	bool_t                             dragging;
	/// The tile at the middle of the map view, and pixels per tile.
	float                              view_x;
	float                              view_y;
	float                              zoom;
	/// The message shown to the user, and how long it still shows.
	const char*                        message;
	float                              message_seconds;
} vasal_editor_state_s;

#endif
