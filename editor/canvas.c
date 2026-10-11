
/// The ground is drawn from a texture with one pixel per tile, rebuilt
/// when the document's tiles, its terrain types or the overlay change, so
/// even the largest map draws in one call at any zoom. Objects are drawn
/// on top: settlements and decorations with the game's sprites, spawn
/// areas and camps as marks. A tool's objects are picked within a short
/// distance of the pointer.

#include "editor/canvas.h"
#include "editor/art.h"
#include "editor/document.h"
#include "editor/ruleset.h"
#include "editor/theme.h"
#include "engine/sprites.h"

#include <math.h>

// How close, in tiles, the pointer must be to pick an object.
#define _vasal_editor_canvas_reach 1.5f

// The fewest pixels per tile sprites are drawn at, so settlements and
// structures stay visible on a map zoomed far out.
#define _vasal_editor_canvas_least 7.0f

// The minimap's longest side, and its margin from the view's corner.
#define _vasal_editor_canvas_minimap_side 200.0f
#define _vasal_editor_canvas_minimap_margin 12.0f

// The objects' colors.
#define _vasal_editor_canvas_settlement (Color){ 240, 196, 70, 255 }
#define _vasal_editor_canvas_spawn (Color){ 90, 200, 255, 255 }
#define _vasal_editor_canvas_camp (Color){ 225, 70, 60, 255 }

// The merge key of nudges, so consecutive nudges are one operation.
#define _vasal_editor_canvas_nudge_key 0x4e000000U

static Color _vasal_editor_canvas_pixels
	[vasal_engine_map_side_max * vasal_engine_map_side_max];
static Texture2D _vasal_editor_canvas_texture;
static bool_t _vasal_editor_canvas_loaded = false;
static uint32_t _vasal_editor_canvas_revision = 0xffffffffU;
static vasal_editor_state_overlay_e _vasal_editor_canvas_overlay =
	vasal_editor_state_overlay_terrain;
// Where the selection was grabbed, from its tile, while it is dragged;
// whether the minimap is being dragged; whether the view is being panned.
static int32_t _vasal_editor_canvas_grab_x = 0;
static int32_t _vasal_editor_canvas_grab_y = 0;
static bool_t _vasal_editor_canvas_steering = false;
static bool_t _vasal_editor_canvas_panning = false;

// A color between two, by a share from 0 to 1.
static Color _vasal_editor_canvas_blend(const Color from, const Color to,
		const float share) {
	const float rest = 1.0f - share;
	const float r = ((float)from.r * rest) + ((float)to.r * share);
	const float g = ((float)from.g * rest) + ((float)to.g * share);
	const float b = ((float)from.b * rest) + ((float)to.b * share);
	const Color blended = {
		(unsigned char)r, (unsigned char)g, (unsigned char)b, 255U
	};
	return blended;
}

// The color of a terrain type under an overlay: its own, its speed from
// red (slow) through yellow to green (fast, dark red when impassable), or
// its sight from dark (hidden) to light (open).
// spec: editor.tools#9
static Color _vasal_editor_canvas_ground(
		const vasal_engine_map_terrain_s* const terrain,
		const vasal_editor_state_overlay_e overlay) {
	Color color = vasal_editor_theme_color(terrain->color | 0xffU);
	if (vasal_editor_state_overlay_speed == overlay) {
		const float speed = fminf((float)terrain->speed / 120.0f, 1.0f);
		const Color slow = { 200, 60, 50, 255 };
		const Color middle = { 225, 200, 60, 255 };
		const Color fast = { 70, 190, 90, 255 };
		color = (0.5f > speed)
			? _vasal_editor_canvas_blend(slow, middle, speed * 2.0f)
			: _vasal_editor_canvas_blend(middle, fast, (speed - 0.5f) * 2.0f);
		if (!terrain->passable) {
			color = (Color){ 70, 16, 20, 255 };
		}
	} else if (vasal_editor_state_overlay_sight == overlay) {
		const float sight = fminf((float)terrain->sight / 120.0f, 1.0f);
		color = _vasal_editor_canvas_blend((Color){ 22, 26, 44, 255 },
			(Color){ 240, 238, 205, 255 }, sight);
	} else {
		// The terrain's own color.
	}
	return color;
}

// Rebuilds the texture when the map or the overlay changed since it was
// last built.
static void _vasal_editor_canvas_refresh(
		const vasal_editor_state_overlay_e overlay) {
	const uint32_t revision = vasal_editor_document_revision();
	if (_vasal_editor_canvas_loaded
			&& (revision == _vasal_editor_canvas_revision)
			&& (overlay == _vasal_editor_canvas_overlay)) {
		return;
	}
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	Color colors[vasal_engine_map_terrains_max];
	for (size_t i = 0U; i < map->terrain_count; i++) {
		colors[i] = _vasal_editor_canvas_ground(&map->terrains[i], overlay);
	}
	const size_t tiles = (size_t)map->width * (size_t)map->height;
	for (size_t i = 0U; i < tiles; i++) {
		_vasal_editor_canvas_pixels[i] = colors[map->tiles[i]];
	}
	const bool_t same_size = _vasal_editor_canvas_loaded
		&& (_vasal_editor_canvas_texture.width == (int)map->width)
		&& (_vasal_editor_canvas_texture.height == (int)map->height);
	if (!same_size) {
		if (_vasal_editor_canvas_loaded) {
			UnloadTexture(_vasal_editor_canvas_texture);
		}
		const Image image = {
			.data    = _vasal_editor_canvas_pixels,
			.width   = (int)map->width,
			.height  = (int)map->height,
			.mipmaps = 1,
			.format  = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
		};
		_vasal_editor_canvas_texture = LoadTextureFromImage(image);
		SetTextureFilter(_vasal_editor_canvas_texture, TEXTURE_FILTER_POINT);
		_vasal_editor_canvas_loaded = true;
	} else {
		UpdateTexture(_vasal_editor_canvas_texture,
			_vasal_editor_canvas_pixels);
	}
	_vasal_editor_canvas_revision = revision;
	_vasal_editor_canvas_overlay = overlay;
}

static Vector2 _vasal_editor_canvas_to_screen(
		const vasal_editor_state_s* const state, const Rectangle area,
		const float x, const float y) {
	const Vector2 screen = {
		area.x + (area.width / 2.0f) + ((x - state->view_x) * state->zoom),
		area.y + (area.height / 2.0f) + ((y - state->view_y) * state->zoom)
	};
	return screen;
}

static Vector2 _vasal_editor_canvas_to_map(
		const vasal_editor_state_s* const state, const Rectangle area,
		const Vector2 screen) {
	const Vector2 point = {
		((screen.x - area.x - (area.width / 2.0f)) / state->zoom)
			+ state->view_x,
		((screen.y - area.y - (area.height / 2.0f)) / state->zoom)
			+ state->view_y
	};
	return point;
}

// The minimap's rectangle in the view's bottom right corner, and the
// pixels per tile in it.
static Rectangle _vasal_editor_canvas_minimap(const Rectangle area,
		float* const scale) {
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	const float longest = fmaxf((float)map->width, (float)map->height);
	*scale = _vasal_editor_canvas_minimap_side / longest;
	const float width = (float)map->width * *scale;
	const float height = (float)map->height * *scale;
	const Rectangle bounds = {
		(area.x + area.width) - width - _vasal_editor_canvas_minimap_margin,
		(area.y + area.height) - height - _vasal_editor_canvas_minimap_margin,
		width, height
	};
	return bounds;
}

// A value kept between 0 and an end, which it stays below.
static int32_t _vasal_editor_canvas_within(const int32_t value,
		const uint16_t end) {
	int32_t kept = value;
	if (0 > kept) {
		kept = 0;
	}
	if (kept >= (int32_t)end) {
		kept = (int32_t)end - 1;
	}
	return kept;
}

static float _vasal_editor_canvas_clamp_zoom(const float zoom) {
	return fminf(fmaxf(zoom, 0.1f), 64.0f);
}

void vasal_editor_canvas_fit(vasal_editor_state_s* const state,
	const Rectangle area) {
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	state->view_x = (float)map->width / 2.0f;
	state->view_y = (float)map->height / 2.0f;
	state->zoom = _vasal_editor_canvas_clamp_zoom(fminf(
		area.width / (float)map->width, area.height / (float)map->height)
		* 0.92f);
}

void vasal_editor_canvas_focus(vasal_editor_state_s* const state,
	const uint16_t x, const uint16_t y) {
	state->view_x = (float)x + 0.5f;
	state->view_y = (float)y + 0.5f;
	state->zoom = fmaxf(state->zoom, 8.0f);
}

void vasal_editor_canvas_zoom(vasal_editor_state_s* const state,
	const float factor) {
	state->zoom = _vasal_editor_canvas_clamp_zoom(state->zoom * factor);
}

// Pans with the middle or right button, or the left one while space is
// held; zooms with the wheel around the pointer, or with Shift held sizes
// the brush of the painting tools.
// spec: editor.tools#3 editor.tools#10
static void _vasal_editor_canvas_navigate(vasal_editor_state_s* const state,
		const Rectangle area, const bool_t over) {
	const bool_t grab = IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)
		|| IsMouseButtonDown(MOUSE_BUTTON_RIGHT)
		|| (IsKeyDown(KEY_SPACE) && IsMouseButtonDown(MOUSE_BUTTON_LEFT));
	if (over && grab && !state->dragging) {
		_vasal_editor_canvas_panning = true;
	}
	if (!grab) {
		_vasal_editor_canvas_panning = false;
	}
	if (_vasal_editor_canvas_panning) {
		const Vector2 delta = GetMouseDelta();
		state->view_x -= delta.x / state->zoom;
		state->view_y -= delta.y / state->zoom;
	}
	const float wheel = GetMouseWheelMove();
	if (!over || (0.0f == wheel)) {
		return;
	}
	const bool_t sizing = (IsKeyDown(KEY_LEFT_SHIFT)
		|| IsKeyDown(KEY_RIGHT_SHIFT))
		&& ((vasal_editor_state_tool_paint == state->tool)
			|| (vasal_editor_state_tool_fill == state->tool));
	if (sizing) {
		int32_t step = -1;
		if (0.0f < wheel) {
			step = 1;
		}
		const int32_t radius = state->brush.radius + step;
		if ((0 <= radius) && (vasal_editor_edits_brush_max >= radius)) {
			state->brush.radius = radius;
		}
		return;
	}
	const Vector2 before = _vasal_editor_canvas_to_map(state, area,
		GetMousePosition());
	state->zoom = _vasal_editor_canvas_clamp_zoom(state->zoom
		* powf(1.2f, wheel));
	const Vector2 after = _vasal_editor_canvas_to_map(state, area,
		GetMousePosition());
	state->view_x += before.x - after.x;
	state->view_y += before.y - after.y;
}

// Clicking or dragging on the minimap centers the view there.
static bool_t _vasal_editor_canvas_steer(vasal_editor_state_s* const state,
		const Rectangle area, const bool_t input) {
	if (!state->minimap) {
		_vasal_editor_canvas_steering = false;
		return false;
	}
	float scale = 1.0f;
	const Rectangle bounds = _vasal_editor_canvas_minimap(area, &scale);
	const Vector2 mouse = GetMousePosition();
	if (input && !state->dragging && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
			&& CheckCollisionPointRec(mouse, bounds)) {
		_vasal_editor_canvas_steering = true;
	}
	if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
		_vasal_editor_canvas_steering = false;
	}
	if (_vasal_editor_canvas_steering) {
		state->view_x = (mouse.x - bounds.x) / scale;
		state->view_y = (mouse.y - bounds.y) / scale;
	}
	return _vasal_editor_canvas_steering
		|| CheckCollisionPointRec(mouse, bounds);
}

static bool_t _vasal_editor_canvas_shift(void) {
	return IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
}

// Picks the terrain type of a tile to paint with, and goes back to the
// brush.
// spec: editor.tools#8
static void _vasal_editor_canvas_take_terrain(
		vasal_editor_state_s* const state, const int32_t x, const int32_t y) {
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	state->terrain = map->tiles[((size_t)y * map->width) + (size_t)x];
	if (vasal_editor_state_tool_fill != state->tool) {
		state->tool = vasal_editor_state_tool_paint;
	}
	vasal_editor_state_say(state, map->terrains[state->terrain].name, false);
}

// Fills the area under the pointer, as one operation.
static void _vasal_editor_canvas_fill(vasal_editor_state_s* const state,
		const int32_t x, const int32_t y) {
	size_t filled = 0U;
	vasal_editor_document_begin(0U);
	const vasal_editor_edits_fill_e outcome = vasal_editor_edits_fill(
		(uint16_t)x, (uint16_t)y, state->terrain, &filled);
	vasal_editor_document_end();
	if (vasal_editor_edits_fill_large == outcome) {
		vasal_editor_state_say(state, "That area is too large to fill at "
			"once; split it with the brush first.", true);
	} else if (vasal_editor_edits_fill_done == outcome) {
		vasal_editor_state_say(state, TextFormat("Filled %u tiles.",
			(unsigned int)filled), false);
	} else {
		// Already of that type.
	}
}

// Grabs an object to drag it.
static void _vasal_editor_canvas_grab(vasal_editor_state_s* const state,
		const vasal_editor_edits_kind_e kind, const uint16_t index,
		const int32_t x, const int32_t y) {
	vasal_editor_state_select(state, true, kind, index);
	const vasal_engine_map_point_s* const at =
		vasal_editor_edits_position(kind, index);
	_vasal_editor_canvas_grab_x = (int32_t)at->x - x;
	_vasal_editor_canvas_grab_y = (int32_t)at->y - y;
	state->dragging = true;
	vasal_editor_document_begin(0U);
}

// The left button went down on a tile of the map with an object tool:
// grab one of its kind, or place a new one.
static void _vasal_editor_canvas_place(vasal_editor_state_s* const state,
		const Vector2 point, const int32_t x, const int32_t y) {
	const vasal_editor_edits_kind_e kind =
		vasal_editor_state_tool_kind(state->tool);
	uint16_t index = 0U;
	if (vasal_editor_edits_pick(kind, point.x, point.y,
			_vasal_editor_canvas_reach, &index)) {
		_vasal_editor_canvas_grab(state, kind, index, x, y);
		return;
	}
	const vasal_engine_map_point_s at = { (uint16_t)x, (uint16_t)y };
	vasal_editor_document_begin(0U);
	const bool_t added = vasal_editor_edits_add(kind, at,
		vasal_editor_ruleset_rules(), state->settlement_type,
		state->decoration_kind, &index);
	vasal_editor_document_end();
	if (!added) {
		vasal_editor_state_say(state, "There is no room for more of these.",
			true);
		return;
	}
	_vasal_editor_canvas_grab(state, kind, index, x, y);
}

// The left button went down on a tile of the map.
// spec: editor.tools#1 editor.tools#4 editor.tools#8
static void _vasal_editor_canvas_press(vasal_editor_state_s* const state,
		const Vector2 point, const int32_t x, const int32_t y) {
	const bool_t alt = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);
	const bool_t brush = (vasal_editor_state_tool_paint == state->tool)
		|| (vasal_editor_state_tool_fill == state->tool);
	if ((vasal_editor_state_tool_pick == state->tool) || (brush && alt)) {
		_vasal_editor_canvas_take_terrain(state, x, y);
	} else if (vasal_editor_state_tool_paint == state->tool) {
		state->dragging = true;
		vasal_editor_document_begin(0U);
		if (_vasal_editor_canvas_shift() && state->painted) {
			vasal_editor_edits_line(state->brush, state->painted_x,
				state->painted_y, x, y, state->terrain);
		} else {
			vasal_editor_edits_stamp(state->brush, x, y, state->terrain);
		}
		state->painted = true;
		state->painted_x = x;
		state->painted_y = y;
	} else if (vasal_editor_state_tool_fill == state->tool) {
		_vasal_editor_canvas_fill(state, x, y);
	} else if (vasal_editor_state_tool_select == state->tool) {
		vasal_editor_edits_kind_e kind = vasal_editor_edits_settlement;
		uint16_t index = 0U;
		if (vasal_editor_edits_pick_any(point.x, point.y,
				_vasal_editor_canvas_reach, &kind, &index)) {
			_vasal_editor_canvas_grab(state, kind, index, x, y);
		} else {
			vasal_editor_state_select(state, false, kind, 0U);
		}
	} else {
		_vasal_editor_canvas_place(state, point, x, y);
	}
}

// The pointer moved to a tile while the left button is held.
static void _vasal_editor_canvas_drag(vasal_editor_state_s* const state,
		const int32_t x, const int32_t y) {
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	if (vasal_editor_state_tool_paint == state->tool) {
		if ((x != state->painted_x) || (y != state->painted_y)) {
			vasal_editor_edits_line(state->brush, state->painted_x,
				state->painted_y, x, y, state->terrain);
			state->painted_x = x;
			state->painted_y = y;
		}
		return;
	}
	if (!state->selected) {
		return;
	}
	const int32_t to_x = _vasal_editor_canvas_within(
		x + _vasal_editor_canvas_grab_x, map->width);
	const int32_t to_y = _vasal_editor_canvas_within(
		y + _vasal_editor_canvas_grab_y, map->height);
	vasal_engine_map_point_s* const position = vasal_editor_edits_position(
		state->selection_kind, state->selection);
	if (((int32_t)position->x != to_x) || ((int32_t)position->y != to_y)) {
		position->x = (uint16_t)to_x;
		position->y = (uint16_t)to_y;
		vasal_editor_document_changed();
	}
}

// spec: editor.tools#1 editor.tools#3
void vasal_editor_canvas_update(vasal_editor_state_s* const state,
	const Rectangle area, const float seconds, const bool_t input) {
	(void)seconds;
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	const bool_t on_minimap = _vasal_editor_canvas_steer(state, area, input);
	const bool_t over = input && !on_minimap
		&& CheckCollisionPointRec(GetMousePosition(), area);
	_vasal_editor_canvas_navigate(state, area, over);

	const Vector2 point = _vasal_editor_canvas_to_map(state, area,
		GetMousePosition());
	const int32_t x = (int32_t)floorf(point.x);
	const int32_t y = (int32_t)floorf(point.y);
	const bool_t on_map = (x >= 0) && (y >= 0) && (x < (int32_t)map->width)
		&& (y < (int32_t)map->height);
	state->hovering = over && on_map;
	state->hover_x = x;
	state->hover_y = y;

	const bool_t pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT)
		&& !IsKeyDown(KEY_SPACE);
	if (state->hovering && pressed && !state->dragging) {
		_vasal_editor_canvas_press(state, point, x, y);
	}
	if (state->dragging) {
		// Off the map, the brush and the object keep to its edge.
		_vasal_editor_canvas_drag(state,
			_vasal_editor_canvas_within(x, map->width),
			_vasal_editor_canvas_within(y, map->height));
	}
	if (state->dragging && !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
		state->dragging = false;
		vasal_editor_document_end();
	}
}

void vasal_editor_canvas_delete(vasal_editor_state_s* const state) {
	if (!state->selected || state->dragging) {
		return;
	}
	vasal_editor_document_begin(0U);
	vasal_editor_edits_remove(state->selection_kind, state->selection);
	vasal_editor_document_end();
	state->selected = false;
}

// spec: editor.tools#8
void vasal_editor_canvas_duplicate(vasal_editor_state_s* const state) {
	if (!state->selected || state->dragging) {
		return;
	}
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	const vasal_engine_map_point_s* const from = vasal_editor_edits_position(
		state->selection_kind, state->selection);
	uint32_t step = 2U;
	if (vasal_editor_edits_settlement == state->selection_kind) {
		step = 8U;
	}
	vasal_engine_map_point_s at = *from;
	if (((uint32_t)at.x + step) < (uint32_t)map->width) {
		at.x = (uint16_t)(at.x + step);
	}
	if (((uint32_t)at.y + step) < (uint32_t)map->height) {
		at.y = (uint16_t)(at.y + step);
	}
	uint16_t index = 0U;
	vasal_editor_document_begin(0U);
	const bool_t copied = vasal_editor_edits_duplicate(state->selection_kind,
		state->selection, at, &index);
	vasal_editor_document_end();
	if (copied) {
		vasal_editor_state_select(state, true, state->selection_kind, index);
	} else {
		vasal_editor_state_say(state, "There is no room for more of these.",
			true);
	}
}

// spec: editor.tools#8
void vasal_editor_canvas_nudge(vasal_editor_state_s* const state,
	const int32_t dx, const int32_t dy) {
	if (!state->selected || state->dragging) {
		return;
	}
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	vasal_engine_map_point_s* const position = vasal_editor_edits_position(
		state->selection_kind, state->selection);
	const int32_t x = _vasal_editor_canvas_within((int32_t)position->x + dx,
		map->width);
	const int32_t y = _vasal_editor_canvas_within((int32_t)position->y + dy,
		map->height);
	const uint32_t key = _vasal_editor_canvas_nudge_key
		| ((uint32_t)state->selection_kind << 16U) | state->selection;
	vasal_editor_document_begin(key);
	position->x = (uint16_t)x;
	position->y = (uint16_t)y;
	vasal_editor_document_changed();
	vasal_editor_document_end();
}

// Draws the grid: every tile when zoomed in far, and the 16 tile chunks.
// spec: editor.tools#9
static void _vasal_editor_canvas_grid(const vasal_editor_state_s* const state,
		const Rectangle area) {
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	const Vector2 first = _vasal_editor_canvas_to_map(state, area,
		(Vector2){ area.x, area.y });
	const Vector2 last = _vasal_editor_canvas_to_map(state, area,
		(Vector2){ area.x + area.width, area.y + area.height });
	const int32_t left = (int32_t)fmaxf(0.0f, floorf(first.x));
	const int32_t top = (int32_t)fmaxf(0.0f, floorf(first.y));
	const int32_t right = (int32_t)fminf((float)map->width, ceilf(last.x));
	const int32_t bottom = (int32_t)fminf((float)map->height, ceilf(last.y));
	int32_t step = 16;
	if (8.0f <= state->zoom) {
		step = 1;
	}
	if (1.0f > (state->zoom * (float)step)) {
		return;
	}
	const Vector2 top_left = _vasal_editor_canvas_to_screen(state, area,
		(float)left, (float)top);
	const Vector2 bottom_right = _vasal_editor_canvas_to_screen(state, area,
		(float)right, (float)bottom);
	for (int32_t x = left - (left % step); x <= right; x += step) {
		const float sx = area.x + (area.width / 2.0f)
			+ (((float)x - state->view_x) * state->zoom);
		const unsigned char alpha = (0 == (x % 16)) ? 70U : 28U;
		DrawLineV((Vector2){ sx, top_left.y }, (Vector2){ sx,
			bottom_right.y }, (Color){ 0, 0, 0, alpha });
	}
	for (int32_t y = top - (top % step); y <= bottom; y += step) {
		const float sy = area.y + (area.height / 2.0f)
			+ (((float)y - state->view_y) * state->zoom);
		const unsigned char alpha = (0 == (y % 16)) ? 70U : 28U;
		DrawLineV((Vector2){ top_left.x, sy }, (Vector2){ bottom_right.x,
			sy }, (Color){ 0, 0, 0, alpha });
	}
}

// A name on a dark pill under a point.
static void _vasal_editor_canvas_tag(const char* const text,
		const Vector2 at, const Color color) {
	if ('\0' == text[0]) {
		return;
	}
	const float width = vasal_editor_theme_measure(text,
		vasal_editor_theme_small, true);
	const Rectangle pill = {
		at.x - (width / 2.0f) - 6.0f, at.y, width + 12.0f, 18.0f
	};
	DrawRectangleRounded(pill, 0.5f, 6, (Color){ 16, 17, 20, 200 });
	vasal_editor_theme_text(text, pill.x + 6.0f, pill.y + 2.0f,
		vasal_editor_theme_small, true, color);
}

// A camp's mark: a tent.
static void _vasal_editor_canvas_tent(const Vector2 center, const float size,
		const Color color) {
	const Vector2 top = { center.x, center.y - size };
	const Vector2 left = { center.x - size, center.y + (size * 0.7f) };
	const Vector2 right = { center.x + size, center.y + (size * 0.7f) };
	DrawTriangle(top, left, right, color);
	DrawTriangleLines(top, left, right, (Color){ 30, 10, 10, 255 });
	DrawTriangle((Vector2){ center.x, center.y }, (Vector2){
		center.x - (size * 0.3f), center.y + (size * 0.7f) }, (Vector2){
		center.x + (size * 0.3f), center.y + (size * 0.7f) },
		(Color){ 30, 10, 10, 255 });
}

// Draws one object, standing out when it is selected or under the
// pointer, or faint as a preview.
static void _vasal_editor_canvas_object(const vasal_editor_state_s* const state,
		const Rectangle area, const vasal_editor_edits_kind_e kind,
		const uint16_t index, const Color tint) {
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	const vasal_engine_map_point_s* const at =
		vasal_editor_edits_position(kind, index);
	const Vector2 center = _vasal_editor_canvas_to_screen(state, area,
		(float)at->x + 0.5f, (float)at->y + 0.5f);
	const bool_t selected = state->selected
		&& (state->selection_kind == kind) && (state->selection == index);
	const Color mark = selected ? vasal_editor_theme_accent : tint;
	const float pixels = fmaxf(state->zoom, _vasal_editor_canvas_least);
	const float size = fmaxf(5.0f, state->zoom * 0.8f);
	if (vasal_editor_edits_settlement == kind) {
		const vasal_engine_map_settlement_s* const settlement =
			&map->settlements[index];
		DrawCircleLinesV(center, (float)settlement->radius * state->zoom,
			ColorAlpha(mark, 0.8f));
		const size_t sprite = vasal_engine_sprites_settlement(settlement);
		vasal_editor_art_standing(sprite, center, pixels, ColorAlpha(WHITE,
			(float)tint.a / 255.0f));
		if (selected) {
			DrawRectangleLinesEx(vasal_editor_art_bounds(sprite, center,
				pixels), 2.0f, vasal_editor_theme_accent);
		}
		if (3.0f <= state->zoom) {
			_vasal_editor_canvas_tag(settlement->name, (Vector2){ center.x,
				center.y + (pixels * 0.6f) }, selected
				? vasal_editor_theme_accent : vasal_editor_theme_foreground);
		}
	} else if (vasal_editor_edits_spawn == kind) {
		DrawCircleV(center, (float)map->spawns[index].radius * state->zoom,
			ColorAlpha(mark, 0.15f));
		DrawCircleLinesV(center, (float)map->spawns[index].radius
			* state->zoom, mark);
		DrawCircleV(center, size * 0.6f, mark);
	} else if (vasal_editor_edits_camp == kind) {
		_vasal_editor_canvas_tent(center, size, mark);
	} else {
		const size_t sprite =
			vasal_engine_sprites_decoration(&map->decorations[index]);
		vasal_editor_art_standing(sprite, center, pixels, ColorAlpha(WHITE,
			(float)tint.a / 255.0f));
		if (selected) {
			DrawRectangleLinesEx(vasal_editor_art_bounds(sprite, center,
				pixels), 2.0f, vasal_editor_theme_accent);
		}
	}
}

static void _vasal_editor_canvas_objects(
		const vasal_editor_state_s* const state, const Rectangle area,
		const vasal_editor_edits_kind_e kind, const Color color) {
	const uint16_t count = vasal_editor_edits_count(kind);
	for (uint16_t i = 0U; i < count; i++) {
		_vasal_editor_canvas_object(state, area, kind, i, color);
	}
}

// What a new object would look like at the pointer, before it is placed.
// spec: editor.tools#10
static void _vasal_editor_canvas_ghost(const vasal_editor_state_s* const state,
		const Rectangle area) {
	const Vector2 center = _vasal_editor_canvas_to_screen(state, area,
		(float)state->hover_x + 0.5f, (float)state->hover_y + 0.5f);
	const float pixels = fmaxf(state->zoom, _vasal_editor_canvas_least);
	const Color faint = { 255, 255, 255, 140 };
	if (vasal_editor_state_tool_settlement == state->tool) {
		const vasal_engine_rules_s* const rules = vasal_editor_ruleset_rules();
		const uint32_t radius = rules->settlements[state->settlement_type]
			[vasal_engine_rules_settlement_radius];
		DrawCircleLinesV(center, (float)radius * state->zoom,
			ColorAlpha(_vasal_editor_canvas_settlement, 0.6f));
		vasal_editor_art_standing(vasal_engine_sprites_type(
			state->settlement_type), center, pixels, faint);
	} else if (vasal_editor_state_tool_spawn == state->tool) {
		DrawCircleLinesV(center, 6.0f * state->zoom,
			ColorAlpha(_vasal_editor_canvas_spawn, 0.6f));
	} else if (vasal_editor_state_tool_camp == state->tool) {
		_vasal_editor_canvas_tent(center, fmaxf(5.0f, state->zoom * 0.8f),
			ColorAlpha(_vasal_editor_canvas_camp, 0.5f));
	} else {
		const vasal_engine_map_decoration_s decoration = {
			{ 0U, 0U }, state->decoration_kind
		};
		vasal_editor_art_standing(vasal_engine_sprites_decoration(&decoration),
			center, pixels, faint);
	}
}

// The tiles the brush would paint, outlined, and the line Shift would
// paint.
// spec: editor.tools#10
static void _vasal_editor_canvas_brush(const vasal_editor_state_s* const state,
		const Rectangle area) {
	const float r = (float)state->brush.radius;
	const Color outline = { 255, 255, 255, 200 };
	const bool_t painting = vasal_editor_state_tool_paint == state->tool;
	if (painting && _vasal_editor_canvas_shift() && state->painted
			&& !state->dragging) {
		const Vector2 from = _vasal_editor_canvas_to_screen(state, area,
			(float)state->painted_x + 0.5f, (float)state->painted_y + 0.5f);
		const Vector2 to = _vasal_editor_canvas_to_screen(state, area,
			(float)state->hover_x + 0.5f, (float)state->hover_y + 0.5f);
		DrawLineEx(from, to, fmaxf(2.0f, ((2.0f * r) + 1.0f) * state->zoom),
			(Color){ 255, 255, 255, 50 });
	}
	if (!painting || state->brush.square || (0.0f == r)) {
		const float reach = painting ? r : 0.0f;
		const Vector2 corner = _vasal_editor_canvas_to_screen(state, area,
			(float)state->hover_x - reach, (float)state->hover_y - reach);
		const float side = ((2.0f * reach) + 1.0f) * state->zoom;
		DrawRectangleLinesEx((Rectangle){ corner.x, corner.y, side, side },
			1.5f, outline);
		return;
	}
	const Vector2 center = _vasal_editor_canvas_to_screen(state, area,
		(float)state->hover_x + 0.5f, (float)state->hover_y + 0.5f);
	DrawCircleLinesV(center, (r + 0.5f) * state->zoom, outline);
}

// The minimap: the whole ground, and the view's rectangle on it.
static void _vasal_editor_canvas_draw_minimap(
		const vasal_editor_state_s* const state, const Rectangle area) {
	float scale = 1.0f;
	const Rectangle bounds = _vasal_editor_canvas_minimap(area, &scale);
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	DrawRectangleRec((Rectangle){ bounds.x - 3.0f, bounds.y - 3.0f,
		bounds.width + 6.0f, bounds.height + 6.0f },
		vasal_editor_theme_panel);
	DrawTexturePro(_vasal_editor_canvas_texture, (Rectangle){ 0.0f, 0.0f,
		(float)map->width, (float)map->height }, bounds,
		(Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
	for (uint16_t i = 0U; i < map->settlement_count; i++) {
		DrawRectangleV((Vector2){
			bounds.x + ((float)map->settlements[i].position.x * scale) - 1.0f,
			bounds.y + ((float)map->settlements[i].position.y * scale) - 1.0f
		}, (Vector2){ 3.0f, 3.0f }, _vasal_editor_canvas_settlement);
	}
	const float width = (area.width / state->zoom) * scale;
	const float height = (area.height / state->zoom) * scale;
	const Rectangle view = {
		bounds.x + (state->view_x * scale) - (width / 2.0f),
		bounds.y + (state->view_y * scale) - (height / 2.0f), width, height
	};
	BeginScissorMode((int)bounds.x, (int)bounds.y, (int)bounds.width,
		(int)bounds.height);
	DrawRectangleLinesEx(view, 1.5f, WHITE);
	EndScissorMode();
	DrawRectangleLinesEx((Rectangle){ bounds.x - 3.0f, bounds.y - 3.0f,
		bounds.width + 6.0f, bounds.height + 6.0f }, 1.0f,
		vasal_editor_theme_border);
}

// spec: editor.tools
void vasal_editor_canvas_draw(const vasal_editor_state_s* const state,
	const Rectangle area) {
	_vasal_editor_canvas_refresh(state->overlay);
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	BeginScissorMode((int)area.x, (int)area.y, (int)area.width,
		(int)area.height);
	DrawRectangleRec(area, vasal_editor_theme_background);
	const Vector2 origin = _vasal_editor_canvas_to_screen(state, area, 0.0f,
		0.0f);
	const Rectangle target = {
		origin.x, origin.y, (float)map->width * state->zoom,
		(float)map->height * state->zoom
	};
	DrawRectangleRec((Rectangle){ target.x + 6.0f, target.y + 6.0f,
		target.width, target.height }, (Color){ 0, 0, 0, 90 });
	DrawTexturePro(_vasal_editor_canvas_texture, (Rectangle){ 0.0f, 0.0f,
		(float)map->width, (float)map->height }, target,
		(Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
	if (state->grid) {
		_vasal_editor_canvas_grid(state, area);
	}
	DrawRectangleLinesEx(target, 1.0f, vasal_editor_theme_border);
	_vasal_editor_canvas_objects(state, area, vasal_editor_edits_decoration,
		WHITE);
	_vasal_editor_canvas_objects(state, area, vasal_editor_edits_spawn,
		_vasal_editor_canvas_spawn);
	_vasal_editor_canvas_objects(state, area, vasal_editor_edits_camp,
		_vasal_editor_canvas_camp);
	_vasal_editor_canvas_objects(state, area, vasal_editor_edits_settlement,
		_vasal_editor_canvas_settlement);
	if (state->hovering && !state->dragging
			&& vasal_editor_state_places(state->tool)) {
		uint16_t index = 0U;
		const Vector2 point = {
			(float)state->hover_x + 0.5f, (float)state->hover_y + 0.5f
		};
		if (!vasal_editor_edits_pick(vasal_editor_state_tool_kind(
				state->tool), point.x, point.y, _vasal_editor_canvas_reach,
				&index)) {
			_vasal_editor_canvas_ghost(state, area);
		}
	}
	if (state->hovering && (vasal_editor_state_tool_select != state->tool)
			&& !vasal_editor_state_places(state->tool)) {
		_vasal_editor_canvas_brush(state, area);
	}
	if (state->minimap) {
		_vasal_editor_canvas_draw_minimap(state, area);
	}
	EndScissorMode();
}
