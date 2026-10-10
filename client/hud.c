
/// Plain shapes and raylib's default font, until the real art.

#include "client/hud.h"

#include <raylib.h>

#include <math.h>
#include <string.h>

#define _vasal_client_hud_panel (Color){ 20, 20, 20, 190 }
#define _vasal_client_hud_text (Color){ 240, 236, 226, 255 }
#define _vasal_client_hud_muted (Color){ 140, 136, 128, 255 }
#define _vasal_client_hud_selected (Color){ 214, 64, 52, 255 }
#define _vasal_client_hud_focus (Color){ 255, 236, 200, 255 }

static const char* const _vasal_client_hud_actions[] = {
	"move", "attack to point", "attack beyond"
};

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

// The groups along the bottom left: number, troops, selection and focus.
static void _vasal_client_hud_groups(const vasal_client_game_s* const game,
		const float height) {
	const int box = 64;
	const int gap = 6;
	const int top = (int)height - box - 12;
	for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
		const vasal_engine_world_group_s* const group =
			vasal_client_game_group(game, g);
		const int left = 12 + ((int)g * (box + gap));
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
		_vasal_client_hud_number(label,
			(vasal_engine_world_centenar == g) ? "C " : "",
			(uint32_t)group->troop_count);
		DrawText(label, left + 8, top + 36, 18, empty
			? _vasal_client_hud_muted : _vasal_client_hud_text);
	}
}

// spec: vintaines.actions#2
static void _vasal_client_hud_wheel(const vasal_client_input_s* const input) {
	const Vector2 mouse = GetMousePosition();
	const vasal_client_game_action_e hovered =
		vasal_client_wheel_action(&input->wheel, mouse.x, mouse.y);
	const float r = vasal_client_wheel_radius;
	DrawCircle((int)input->wheel.press_x, (int)input->wheel.press_y, r,
		_vasal_client_hud_panel);
	const float places[3][2] = {
		{ 0.0f, -0.6f }, { -0.55f, 0.45f }, { 0.55f, 0.45f }
	};
	for (size_t a = 0U; a < (size_t)vasal_client_game_action_count; a++) {
		const char* const name = _vasal_client_hud_actions[a];
		const int width = MeasureText(name, 14);
		const float place_x = input->wheel.press_x + (places[a][0] * r);
		const float place_y = input->wheel.press_y + (places[a][1] * r);
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
