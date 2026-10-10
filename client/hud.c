
/// Plain shapes and raylib's default font, until the real art.

#include "client/hud.h"
#include "client/layout.h"
#include "client/minimap.h"

#include <raylib.h>

#include <math.h>
#include <string.h>

#define _vasal_client_hud_panel (Color){ 20, 20, 20, 190 }
#define _vasal_client_hud_text (Color){ 240, 236, 226, 255 }
#define _vasal_client_hud_muted (Color){ 140, 136, 128, 255 }
#define _vasal_client_hud_selected (Color){ 214, 64, 52, 255 }
#define _vasal_client_hud_focus (Color){ 255, 236, 200, 255 }
#define _vasal_client_hud_own (Color){ 236, 72, 58, 255 }
#define _vasal_client_hud_other (Color){ 150, 160, 200, 255 }

// The minimap's picture of the map, drawn once per map, as the map never
// changes during a match; territory will update only where it changes.
static Texture2D _vasal_client_hud_minimap;
static const vasal_engine_map_s* _vasal_client_hud_minimap_of = NULL;

static const char* const
	_vasal_client_hud_actions[vasal_client_game_action_count] = { "move" };

// Writes a prefix and a number into text, which holds 16 characters.
static void _vasal_client_hud_number(char* const text,
		const char* const prefix, const uint32_t number) {
	size_t at = strlen(prefix);
	if (at > 8U) {
		at = 8U;
	}
	for (size_t i = 0U; i < at; i++) {
		text[i] = prefix[i];
	}
	char digits[10];
	size_t count = 0U;
	uint32_t rest = number;
	do {
		digits[count] = "0123456789"[rest % 10U];
		count++;
		rest /= 10U;
	} while (0U != rest);
	while (0U < count) {
		count--;
		text[at] = digits[count];
		at++;
	}
	text[at] = '\0';
}

// The groups along the bottom left: number, pace, troops, selection and
// focus.
// spec: vintaines.pace#4
static void _vasal_client_hud_groups(const vasal_client_game_s* const game,
		const float height) {
	const int box = vasal_client_layout_box;
	for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
		const vasal_engine_world_group_s* const group =
			vasal_client_game_group(game, g);
		int32_t left = 0;
		int32_t top = 0;
		vasal_client_layout_group(height, g, &left, &top);
		const bool_t selected = 0U != (game->selection & ((uint32_t)1U << g));
		const bool_t empty = 0U == group->troop_count;
		DrawRectangle(left, top, box, box, _vasal_client_hud_panel);
		if (selected) {
			DrawRectangleLines(left, top, box, box, _vasal_client_hud_selected);
		}
		if (selected && (g == game->focus)) {
			DrawRectangleLines(left + 2, top + 2, box - 4, box - 4,
				_vasal_client_hud_focus);
		}
		char label[16];
		_vasal_client_hud_number(label, "", (uint32_t)g + 1U);
		DrawText(label, left + 8, top + 6, 20, empty
			? _vasal_client_hud_muted : _vasal_client_hud_text);
		if (!empty) {
			const bool_t running =
				vasal_engine_world_pace_at_the_run == group->pace;
			const char* const pace = running ? "run" : "step";
			DrawText(pace, left + box - 6 - MeasureText(pace, 10), top + 8,
				10, running ? _vasal_client_hud_focus
					: _vasal_client_hud_muted);
		}
		_vasal_client_hud_number(label,
			(vasal_engine_world_centenar == g) ? "C " : "",
			(uint32_t)group->troop_count);
		DrawText(label, left + 8, top + 36, 18, empty
			? _vasal_client_hud_muted : _vasal_client_hud_text);
	}
}

// Draws the map into the minimap's picture, a pixel at a time.
// spec: map.minimap#1 simulation.performance#2
static void _vasal_client_hud_paint(const vasal_engine_map_s* const map) {
	if (NULL != _vasal_client_hud_minimap_of) {
		UnloadTexture(_vasal_client_hud_minimap);
	}
	const int width = vasal_client_minimap_width;
	const int height = vasal_client_minimap_height;
	Image image = GenImageColor(width, height, BLANK);
	for (int py = 0; py < height; py++) {
		for (int px = 0; px < width; px++) {
			float x = 0.0f;
			float y = 0.0f;
			if (vasal_client_minimap_to_map(map, (float)px + 0.5f,
					(float)py + 0.5f, &x, &y)) {
				const uint32_t rgba = vasal_client_minimap_color(map,
					(uint16_t)x, (uint16_t)y);
				ImageDrawPixel(&image, px, py, GetColor(rgba));
			}
		}
	}
	_vasal_client_hud_minimap = LoadTextureFromImage(image);
	UnloadImage(image);
	_vasal_client_hud_minimap_of = map;
}

// A troop's dot on the minimap.
static void _vasal_client_hud_dot(const vasal_engine_map_s* const map,
		const float left, const float top, const float x, const float y,
		const Color color) {
	float px = 0.0f;
	float py = 0.0f;
	vasal_client_minimap_to_pixel(map, x, y, &px, &py);
	const float dot_x = left + px;
	const float dot_y = top + py;
	DrawRectangle((int)dot_x - 1, (int)dot_y - 1, 2, 2, color);
}

// The minimap: the map, the player's troops, and other vasals' troops
// whose group stands in the player's sight.
// spec: map.minimap#1 map.minimap#2
static void _vasal_client_hud_map(const vasal_client_game_s* const game,
		const float width, const float height) {
	const vasal_engine_world_s* const world = game->world;
	if (world->map != _vasal_client_hud_minimap_of) {
		_vasal_client_hud_paint(world->map);
	}
	float left = 0.0f;
	float top = 0.0f;
	vasal_client_minimap_panel(width, height, &left, &top);
	const int padding = vasal_client_minimap_padding;
	DrawRectangle((int)left, (int)top,
		vasal_client_minimap_width + (2 * padding),
		vasal_client_minimap_height + (2 * padding), _vasal_client_hud_panel);
	left += (float)padding;
	top += (float)padding;
	DrawTexture(_vasal_client_hud_minimap, (int)left, (int)top, WHITE);
	for (size_t v = 0U; v < world->vasal_count; v++) {
		const bool_t own = v == game->vasal;
		for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
			const vasal_engine_world_group_s* const group =
				&world->vasals[v].groups[g];
			const float precision = (float)vasal_engine_map_precision;
			const bool_t shown = own || (vasal_client_game_sight_at(game,
				(float)group->anchor.x / precision,
				(float)group->anchor.y / precision) > 0.0f);
			for (size_t t = 0U; shown && (t < group->troop_count); t++) {
				const vasal_engine_world_position_s at =
					group->troops[t].position;
				_vasal_client_hud_dot(world->map, left, top,
					(float)at.x / precision, (float)at.y / precision,
					own ? _vasal_client_hud_own : _vasal_client_hud_other);
			}
		}
	}
}

// spec: vintaines.actions#2
static void _vasal_client_hud_wheel(const vasal_client_input_s* const input) {
	const Vector2 mouse = GetMousePosition();
	const vasal_client_game_action_e hovered =
		vasal_client_wheel_action(&input->wheel, mouse.x, mouse.y);
	const float r = vasal_client_wheel_radius;
	DrawCircle((int)input->wheel.x, (int)input->wheel.y, r,
		_vasal_client_hud_panel);
	for (size_t a = 0U; a < (size_t)vasal_client_game_action_count; a++) {
		const char* const name = _vasal_client_hud_actions[a];
		const int width = MeasureText(name, 14);
		float dx = 0.0f;
		float dy = 0.0f;
		vasal_client_wheel_place((vasal_client_game_action_e)a, &dx, &dy);
		const float place_x = input->wheel.x + dx;
		const float place_y = input->wheel.y + dy;
		const int x = (int)place_x - (width / 2);
		const int y = (int)place_y - 7;
		DrawText(name, x, y, 14, ((size_t)hovered == a)
			? _vasal_client_hud_focus : _vasal_client_hud_muted);
	}
}

void vasal_client_hud_draw(const vasal_client_game_s* const game,
	const vasal_client_input_s* const input, const float width,
	const float height) {
	_vasal_client_hud_groups(game, height);
	_vasal_client_hud_map(game, width, height);
	const char* const action = _vasal_client_hud_actions[game->action];
	DrawText(action, 12, 12, 18, _vasal_client_hud_text);
	if ((NULL != game->notice) && (game->notice_seconds > 0.0f)) {
		const int size = 20;
		const int text_width = MeasureText(game->notice, size);
		DrawText(game->notice, ((int)width - text_width) / 2,
			(int)height - 120, size, _vasal_client_hud_text);
	}
	if (input->wheel.open) {
		_vasal_client_hud_wheel(input);
	}
}
