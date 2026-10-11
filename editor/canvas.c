
/// The map is drawn from a texture with one pixel per tile, rebuilt when
/// the document's tiles or terrain colors change, so even the largest map
/// draws in one call at any zoom. Objects are drawn on top as simple
/// shapes. A tool's objects are picked within a short distance of the
/// pointer.

#include "editor/canvas.h"
#include "editor/art.h"
#include "editor/document.h"
#include "editor/ruleset.h"
#include "engine/sprites.h"

#include <math.h>

// Placeholder look: colors of the objects and of the selection.
#define _vasal_editor_canvas_settlement (Color){ 236, 190, 60, 255 }
#define _vasal_editor_canvas_spawn (Color){ 90, 200, 255, 255 }
#define _vasal_editor_canvas_camp (Color){ 220, 60, 50, 255 }
#define _vasal_editor_canvas_decoration (Color){ 40, 40, 40, 255 }
#define _vasal_editor_canvas_selected (Color){ 255, 255, 255, 255 }

// How close, in tiles, the pointer must be to pick an object.
#define _vasal_editor_canvas_reach 1.5f

static Color _vasal_editor_canvas_pixels
	[vasal_engine_map_side_max * vasal_engine_map_side_max];
static Texture2D _vasal_editor_canvas_texture;
static bool_t _vasal_editor_canvas_loaded = false;
static uint32_t _vasal_editor_canvas_revision = 0xffffffffU;

static Color _vasal_editor_canvas_color(const uint32_t rgba) {
	const Color color = {
		(unsigned char)((rgba >> 24U) & 0xffU),
		(unsigned char)((rgba >> 16U) & 0xffU),
		(unsigned char)((rgba >> 8U) & 0xffU),
		255U
	};
	return color;
}

// Rebuilds the texture when the map changed since it was last built.
static void _vasal_editor_canvas_refresh(void) {
	const uint32_t revision = vasal_editor_document_revision();
	if (_vasal_editor_canvas_loaded
			&& (revision == _vasal_editor_canvas_revision)) {
		return;
	}
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	const size_t tiles = (size_t)map->width * (size_t)map->height;
	for (size_t i = 0U; i < tiles; i++) {
		_vasal_editor_canvas_pixels[i] = _vasal_editor_canvas_color(
			map->terrains[map->tiles[i]].color);
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

// How many objects the tool's kind has, and where one stands.
static uint16_t _vasal_editor_canvas_count(const vasal_engine_map_s* const map,
		const vasal_editor_state_tool_e tool) {
	uint16_t count = 0U;
	if (vasal_editor_state_tool_settlement == tool) {
		count = map->settlement_count;
	} else if (vasal_editor_state_tool_spawn == tool) {
		count = map->spawn_count;
	} else if (vasal_editor_state_tool_camp == tool) {
		count = map->camp_count;
	} else if (vasal_editor_state_tool_decoration == tool) {
		count = map->decoration_count;
	} else {
		// Painting has no objects.
	}
	return count;
}

static vasal_engine_map_point_s* _vasal_editor_canvas_position(
		vasal_engine_map_s* const map, const vasal_editor_state_tool_e tool,
		const uint16_t index) {
	vasal_engine_map_point_s* position = &map->decorations[index].position;
	if (vasal_editor_state_tool_settlement == tool) {
		position = &map->settlements[index].position;
	} else if (vasal_editor_state_tool_spawn == tool) {
		position = &map->spawns[index].position;
	} else if (vasal_editor_state_tool_camp == tool) {
		position = &map->camps[index].position;
	} else {
		// A decoration.
	}
	return position;
}

// The object of the tool's kind nearest a point, within reach; returns
// false when there is none.
static bool_t _vasal_editor_canvas_pick(vasal_engine_map_s* const map,
		const vasal_editor_state_tool_e tool, const Vector2 point,
		uint16_t* const index) {
	float best = _vasal_editor_canvas_reach * _vasal_editor_canvas_reach;
	bool_t found = false;
	const uint16_t count = _vasal_editor_canvas_count(map, tool);
	for (uint16_t i = 0U; i < count; i++) {
		const vasal_engine_map_point_s* const at =
			_vasal_editor_canvas_position(map, tool, i);
		const float dx = ((float)at->x + 0.5f) - point.x;
		const float dy = ((float)at->y + 0.5f) - point.y;
		if (((dx * dx) + (dy * dy)) < best) {
			best = (dx * dx) + (dy * dy);
			*index = i;
			found = true;
		}
	}
	return found;
}

// Adds an object of the tool's kind at a tile. Returns false when the
// kind is full.
static bool_t _vasal_editor_canvas_add(vasal_editor_state_s* const state,
		vasal_engine_map_s* const map, const vasal_engine_map_point_s at) {
	const uint16_t count = _vasal_editor_canvas_count(map, state->tool);
	const bool_t full = ((vasal_editor_state_tool_settlement == state->tool)
			&& (vasal_engine_map_settlements_max <= count))
		|| ((vasal_editor_state_tool_spawn == state->tool)
			&& (vasal_engine_map_spawns_max <= count))
		|| ((vasal_editor_state_tool_camp == state->tool)
			&& (vasal_engine_map_camps_max <= count))
		|| ((vasal_editor_state_tool_decoration == state->tool)
			&& (vasal_engine_map_decorations_max <= count));
	if (full) {
		return false;
	}
	if (vasal_editor_state_tool_settlement == state->tool) {
		vasal_engine_map_settlement_s* const settlement =
			&map->settlements[count];
		settlement->name[0] = '\0';
		vasal_engine_rules_settlement_defaults(vasal_editor_ruleset_rules(),
			settlement, state->settlement_type);
		map->settlement_count++;
	} else if (vasal_editor_state_tool_spawn == state->tool) {
		map->spawns[count].radius = 6U;
		map->spawn_count++;
	} else if (vasal_editor_state_tool_camp == state->tool) {
		map->camp_count++;
	} else {
		map->decorations[count].kind = state->decoration_kind;
		map->decoration_count++;
	}
	*_vasal_editor_canvas_position(map, state->tool, count) = at;
	vasal_editor_document_changed();
	state->selected = true;
	state->selection = count;
	return true;
}

// Deletes the selected object, moving the last one into its place.
static void _vasal_editor_canvas_remove(vasal_editor_state_s* const state,
		vasal_engine_map_s* const map) {
	const uint16_t last = _vasal_editor_canvas_count(map, state->tool) - 1U;
	const uint16_t index = state->selection;
	if (vasal_editor_state_tool_settlement == state->tool) {
		map->settlements[index] = map->settlements[last];
		map->settlement_count--;
	} else if (vasal_editor_state_tool_spawn == state->tool) {
		map->spawns[index] = map->spawns[last];
		map->spawn_count--;
	} else if (vasal_editor_state_tool_camp == state->tool) {
		map->camps[index] = map->camps[last];
		map->camp_count--;
	} else {
		map->decorations[index] = map->decorations[last];
		map->decoration_count--;
	}
	vasal_editor_document_changed();
	state->selected = false;
}

// Paints the brush, a disc of the brush's radius, around a tile.
static void _vasal_editor_canvas_paint(const vasal_editor_state_s* const state,
		const vasal_engine_map_s* const map, const int32_t x,
		const int32_t y) {
	const int32_t r = (int32_t)state->brush;
	for (int32_t dy = -r; dy <= r; dy++) {
		for (int32_t dx = -r; dx <= r; dx++) {
			const int32_t px = x + dx;
			const int32_t py = y + dy;
			const bool_t inside = (px >= 0) && (py >= 0)
				&& (px < (int32_t)map->width) && (py < (int32_t)map->height);
			if (inside && (((dx * dx) + (dy * dy)) <= (r * r))) {
				vasal_editor_document_paint((uint16_t)px, (uint16_t)py,
					state->terrain);
			}
		}
	}
}

// Pans with the middle or right button and the arrow keys, and zooms with
// the wheel around the pointer.
static void _vasal_editor_canvas_navigate(vasal_editor_state_s* const state,
		const Rectangle area, const float seconds, const bool_t over) {
	if (over && (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)
			|| IsMouseButtonDown(MOUSE_BUTTON_RIGHT))) {
		const Vector2 delta = GetMouseDelta();
		state->view_x -= delta.x / state->zoom;
		state->view_y -= delta.y / state->zoom;
	}
	const float step = (600.0f * seconds) / state->zoom;
	state->view_x += (IsKeyDown(KEY_RIGHT) ? step : 0.0f)
		- (IsKeyDown(KEY_LEFT) ? step : 0.0f);
	state->view_y += (IsKeyDown(KEY_DOWN) ? step : 0.0f)
		- (IsKeyDown(KEY_UP) ? step : 0.0f);

	const float wheel = GetMouseWheelMove();
	if (over && (0.0f != wheel)) {
		const Vector2 before = _vasal_editor_canvas_to_map(state, area,
			GetMousePosition());
		state->zoom = fminf(fmaxf(state->zoom * powf(1.2f, wheel), 0.25f),
			64.0f);
		const Vector2 after = _vasal_editor_canvas_to_map(state, area,
			GetMousePosition());
		state->view_x += before.x - after.x;
		state->view_y += before.y - after.y;
	}
}

// The left button went down on the map: start painting, pick an object
// to move, or place a new one. Each starts an operation, ended when the
// button is released.
static void _vasal_editor_canvas_press(vasal_editor_state_s* const state,
		vasal_engine_map_s* const map, const Vector2 point,
		const vasal_engine_map_point_s tile) {
	state->dragging = true;
	vasal_editor_document_begin(0U);
	if (vasal_editor_state_tool_paint == state->tool) {
		return;
	}
	if (_vasal_editor_canvas_pick(map, state->tool, point,
			&state->selection)) {
		state->selected = true;
	} else if (!_vasal_editor_canvas_add(state, map, tile)) {
		state->message = "There is no room for more of these.";
		state->message_seconds = 3.0f;
	} else {
		// Placed and selected.
	}
}

// spec: editor.tools#1 editor.tools#3
void vasal_editor_canvas_update(vasal_editor_state_s* const state,
	const Rectangle area, const float seconds) {
	vasal_engine_map_s* const map = vasal_editor_document_map();
	const bool_t over = CheckCollisionPointRec(GetMousePosition(), area);
	_vasal_editor_canvas_navigate(state, area, seconds, over);

	const Vector2 point = _vasal_editor_canvas_to_map(state, area,
		GetMousePosition());
	const int32_t x = (int32_t)floorf(point.x);
	const int32_t y = (int32_t)floorf(point.y);
	const bool_t on_map = (x >= 0) && (y >= 0) && (x < (int32_t)map->width)
		&& (y < (int32_t)map->height);
	const vasal_engine_map_point_s tile = {
		on_map ? (uint16_t)x : 0U, on_map ? (uint16_t)y : 0U
	};

	if (over && on_map && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
		_vasal_editor_canvas_press(state, map, point, tile);
	}
	if (state->dragging && on_map) {
		if (vasal_editor_state_tool_paint == state->tool) {
			_vasal_editor_canvas_paint(state, map, x, y);
		} else if (state->selected) {
			vasal_engine_map_point_s* const position =
				_vasal_editor_canvas_position(map, state->tool,
					state->selection);
			if ((position->x != tile.x) || (position->y != tile.y)) {
				*position = tile;
				vasal_editor_document_changed();
			}
		} else {
			// Nothing to move.
		}
	}
	if (state->dragging && !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
		state->dragging = false;
		vasal_editor_document_end();
	}
	if (!state->dragging && state->selected
			&& (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE))) {
		vasal_editor_canvas_delete(state);
	}
}

void vasal_editor_canvas_delete(vasal_editor_state_s* const state) {
	if (!state->selected) {
		return;
	}
	vasal_editor_document_begin(0U);
	_vasal_editor_canvas_remove(state, vasal_editor_document_map());
	vasal_editor_document_end();
}

void vasal_editor_canvas_fit(vasal_editor_state_s* const state,
	const Rectangle area) {
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	state->view_x = (float)map->width / 2.0f;
	state->view_y = (float)map->height / 2.0f;
	state->zoom = fminf(area.width / (float)map->width,
		area.height / (float)map->height) * 0.95f;
}

// Draws a settlement's or decoration's sprite standing on its tile, at
// least big enough to see, framed when it is selected.
// spec: editor.tools#6
static void _vasal_editor_canvas_sprite(const vasal_editor_state_s* const state,
		const Vector2 foot, const size_t sprite, const bool_t selected) {
	const float pixels_per_tile = fmaxf(state->zoom, 3.0f);
	vasal_editor_art_standing(sprite, foot, pixels_per_tile, WHITE);
	if (selected) {
		DrawRectangleLinesEx(vasal_editor_art_bounds(sprite, foot,
			pixels_per_tile), 2.0f, _vasal_editor_canvas_selected);
	}
}

// Draws the objects of one kind; the selected one, when it is of the
// current tool's kind, stands out.
static void _vasal_editor_canvas_objects(
		const vasal_editor_state_s* const state, const Rectangle area,
		const vasal_editor_state_tool_e kind, const Color color) {
	vasal_engine_map_s* const map = vasal_editor_document_map();
	const uint16_t count = _vasal_editor_canvas_count(map, kind);
	for (uint16_t i = 0U; i < count; i++) {
		const vasal_engine_map_point_s* const at =
			_vasal_editor_canvas_position(map, kind, i);
		const Vector2 center = _vasal_editor_canvas_to_screen(state, area,
			(float)at->x + 0.5f, (float)at->y + 0.5f);
		const bool_t selected = state->selected && (state->tool == kind)
			&& (state->selection == i);
		const Color shown = selected ? _vasal_editor_canvas_selected : color;
		const float size = fmaxf(4.0f, state->zoom * 0.8f);
		if (vasal_editor_state_tool_settlement == kind) {
			DrawCircleLinesV(center,
				(float)map->settlements[i].radius * state->zoom, shown);
			_vasal_editor_canvas_sprite(state, center,
				vasal_engine_sprites_settlement(&map->settlements[i]),
				selected);
			if (state->zoom >= 4.0f) {
				const float text_x = center.x + size;
				const float text_y = center.y + size;
				DrawText(map->settlements[i].name, (int)text_x, (int)text_y,
					16, shown);
			}
		} else if (vasal_editor_state_tool_spawn == kind) {
			DrawCircleLinesV(center,
				(float)map->spawns[i].radius * state->zoom, shown);
			DrawCircleV(center, size, shown);
		} else if (vasal_editor_state_tool_camp == kind) {
			DrawTriangle((Vector2){ center.x, center.y - size },
				(Vector2){ center.x - size, center.y + size },
				(Vector2){ center.x + size, center.y + size }, shown);
		} else {
			_vasal_editor_canvas_sprite(state, center,
				vasal_engine_sprites_decoration(&map->decorations[i]),
				selected);
		}
	}
}

// spec: editor.tools
void vasal_editor_canvas_draw(const vasal_editor_state_s* const state,
	const Rectangle area) {
	_vasal_editor_canvas_refresh();
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	BeginScissorMode((int)area.x, (int)area.y, (int)area.width,
		(int)area.height);
	const Vector2 origin = _vasal_editor_canvas_to_screen(state, area, 0.0f,
		0.0f);
	const Rectangle source = {
		0.0f, 0.0f, (float)map->width, (float)map->height
	};
	const Rectangle target = {
		origin.x, origin.y, (float)map->width * state->zoom,
		(float)map->height * state->zoom
	};
	DrawTexturePro(_vasal_editor_canvas_texture, source, target,
		(Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
	DrawRectangleLinesEx(target, 1.0f, _vasal_editor_canvas_decoration);
	_vasal_editor_canvas_objects(state, area,
		vasal_editor_state_tool_decoration, _vasal_editor_canvas_decoration);
	_vasal_editor_canvas_objects(state, area, vasal_editor_state_tool_camp,
		_vasal_editor_canvas_camp);
	_vasal_editor_canvas_objects(state, area, vasal_editor_state_tool_spawn,
		_vasal_editor_canvas_spawn);
	_vasal_editor_canvas_objects(state, area,
		vasal_editor_state_tool_settlement, _vasal_editor_canvas_settlement);
	EndScissorMode();
}
