
/// Framing and gliding. The framed extent is the reach of the focused
/// group's sight; with the margin, it fills the monitor's shorter side.

#include "client/camera.h"

#include <math.h>

// spec: map.camera#1 map.camera#2 map.camera#4 map.camera#5 map.camera#8
// spec: map.camera#3 map.camera#6 map.camera#9
void vasal_client_camera_update(vasal_client_camera_s* const camera,
	const vasal_client_game_s* const game, const float seconds,
	const float width, const float height, const float monitor_width,
	const float monitor_height) {
	float focus_x = 0.0f;
	float focus_y = 0.0f;
	vasal_client_game_anchor_at(game, game->focus, &focus_x, &focus_y);

	// The focused group's sight; until troop types (troops.scout), every
	// troop sees as far.
	const float extent = vasal_client_game_sight;
	float frame = 2.0f * extent * (1.0f + vasal_client_camera_margin);
	if (frame > vasal_client_camera_max_frame) {
		frame = vasal_client_camera_max_frame;
	}

	// The scale fits the frame to the monitor, not the window: the framed
	// sight is an ellipse as wide as the frame and tilt times as tall.
	const float across = (monitor_width > 0.0f) ? monitor_width : width;
	const float down = (monitor_height > 0.0f) ? monitor_height : height;
	const float scale = fminf(across / frame,
		down / (frame * vasal_client_camera_tilt));

	if (!camera->placed) {
		camera->x = focus_x;
		camera->y = focus_y;
		camera->scale = scale;
		camera->placed = true;
	}
	float blend = seconds * vasal_client_camera_glide;
	if (blend > 1.0f) {
		blend = 1.0f;
	}
	camera->x += (focus_x - camera->x) * blend;
	camera->y += (focus_y - camera->y) * blend;
	camera->scale += (scale - camera->scale) * blend;

}

// Turning the map's grid by 45 degrees: across = (x - y) / root 2, down =
// (x + y) / root 2, so distances stay the same before the tilt.
#define _vasal_client_camera_root_half 0.70710678f

void vasal_client_camera_to_screen(const vasal_client_camera_s* const camera,
	const float width, const float height, const float x, const float y,
	float* const screen_x, float* const screen_y) {
	const float dx = x - camera->x;
	const float dy = y - camera->y;
	const float across = (dx - dy) * _vasal_client_camera_root_half;
	const float down = (dx + dy) * _vasal_client_camera_root_half;
	*screen_x = (across * camera->scale) + (width / 2.0f);
	*screen_y = (down * camera->scale * vasal_client_camera_tilt)
		+ (height / 2.0f);
}

void vasal_client_camera_to_map(const vasal_client_camera_s* const camera,
	const float width, const float height, const float screen_x,
	const float screen_y, float* const x, float* const y) {
	const float across = (screen_x - (width / 2.0f)) / camera->scale;
	const float down = (screen_y - (height / 2.0f))
		/ (camera->scale * vasal_client_camera_tilt);
	*x = ((across + down) * _vasal_client_camera_root_half) + camera->x;
	*y = ((down - across) * _vasal_client_camera_root_half) + camera->y;
}
