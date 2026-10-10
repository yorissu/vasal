
/// Draws only the tiles in the window. A tile's sight is how far inside
/// the nearest troop's sight it lies, from 0 (outside) to 1 (well inside),
/// over the last tile of the radius.

#include "client/view.h"
#include "client/depth.h"

#include <raylib.h>

#include <math.h>

// The player's alliance color, and the accent around their troops.
#define _vasal_client_view_alliance (Color){ 214, 64, 52, 255 }
#define _vasal_client_view_accent (Color){ 255, 236, 200, 255 }
#define _vasal_client_view_centenar (Color){ 236, 190, 60, 255 }
#define _vasal_client_view_ring (Color){ 255, 255, 255, 140 }
#define _vasal_client_view_focus_ring (Color){ 255, 255, 255, 230 }

// The most troops whose sight is drawn.
#define _vasal_client_view_eyes_max                                           \
	(vasal_engine_world_groups * vasal_engine_world_troops)

static Color _vasal_client_view_color(const uint32_t rgba) {
	const Color color = {
		(unsigned char)((rgba >> 24U) & 0xffU),
		(unsigned char)((rgba >> 16U) & 0xffU),
		(unsigned char)((rgba >> 8U) & 0xffU),
		(unsigned char)(rgba & 0xffU)
	};
	return color;
}

// The color, grayed by how far outside sight it is.
static Color _vasal_client_view_sighted(const Color color, const float sight) {
	const float gray = ((0.30f * (float)color.r) + (0.59f * (float)color.g)
		+ (0.11f * (float)color.b)) * 0.6f;
	const Color result = {
		(unsigned char)(gray + ((float)color.r - gray) * sight),
		(unsigned char)(gray + ((float)color.g - gray) * sight),
		(unsigned char)(gray + ((float)color.b - gray) * sight),
		255U
	};
	return result;
}

// The tiles the window shows: the map's area under its four corners, which
// in the isometric view is a turned rectangle, so its bounds.
static void _vasal_client_view_bounds(const vasal_engine_map_s* const map,
		const vasal_client_camera_s* const camera, const float width,
		const float height, int32_t bounds[4]) {
	const float corners[4][2] = {
		{ 0.0f, 0.0f }, { width, 0.0f }, { 0.0f, height }, { width, height }
	};
	float low_x = 1.0e9f;
	float low_y = 1.0e9f;
	float high_x = -1.0e9f;
	float high_y = -1.0e9f;
	for (size_t c = 0U; c < 4U; c++) {
		float x = 0.0f;
		float y = 0.0f;
		vasal_client_camera_to_map(camera, width, height, corners[c][0],
			corners[c][1], &x, &y);
		low_x = fminf(low_x, x);
		low_y = fminf(low_y, y);
		high_x = fmaxf(high_x, x);
		high_y = fmaxf(high_y, y);
	}
	bounds[0] = (int32_t)fmaxf(floorf(low_x), 0.0f);
	bounds[1] = (int32_t)fmaxf(floorf(low_y), 0.0f);
	bounds[2] = (int32_t)fminf(ceilf(high_x), (float)map->width);
	bounds[3] = (int32_t)fminf(ceilf(high_y), (float)map->height);
}

// Every tile is a diamond: its corner (x, y) at the top, (x + 1, y) to the
// right, (x + 1, y + 1) at the bottom and (x, y + 1) to the left.
// spec: map.sight#2 map.sight#4 map.sight#5
static void _vasal_client_view_terrain(const vasal_client_game_s* const game,
		const vasal_client_camera_s* const camera, const float width,
		const float height) {
	const vasal_engine_map_s* const map = game->world->map;
	int32_t bounds[4] = { 0, 0, 0, 0 };
	_vasal_client_view_bounds(map, camera, width, height, bounds);
	// Half a diamond's width and height, half a pixel more, so neighbours
	// overlap and no seams show.
	const float half_width = (camera->scale * 0.70710678f) + 0.5f;
	const float half_height =
		(camera->scale * vasal_client_camera_tilt * 0.70710678f) + 0.5f;

	for (int32_t y = bounds[1]; y < bounds[3]; y++) {
		for (int32_t x = bounds[0]; x < bounds[2]; x++) {
			float sx = 0.0f;
			float sy = 0.0f;
			vasal_client_camera_to_screen(camera, width, height, (float)x,
				(float)y, &sx, &sy);
			if ((sx < -half_width) || (sx > (width + half_width))
					|| (sy > height) || ((sy + (2.0f * half_height)) < 0.0f)) {
				continue;
			}
			const float sight = vasal_client_game_sight_at(game,
				(float)x + 0.5f, (float)y + 0.5f);
			const Color color = _vasal_client_view_sighted(
				_vasal_client_view_color(vasal_engine_map_terrain(map,
				(uint16_t)x, (uint16_t)y)->color), sight);
			const Vector2 top = { sx, sy - 0.5f };
			const Vector2 right = { sx + half_width, sy + half_height - 0.5f };
			const Vector2 bottom = { sx, sy + (2.0f * half_height) - 1.0f };
			const Vector2 left = { sx - half_width, sy + half_height - 0.5f };
			DrawTriangle(top, left, right, color);
			DrawTriangle(left, bottom, right, color);
		}
	}
}

// A line from a group (at ax, ay on the screen) to where its current order
// leads and, when it is selected, on through its queued orders.
// spec: vintaines.queue#7
static void _vasal_client_view_route(const vasal_client_camera_s* const camera,
		const float width, const float height,
		const vasal_engine_world_group_s* const group, const float ax,
		const float ay, const bool_t selected) {
	if (!group->moving || (0U == group->waypoint_count)) {
		return;
	}
	vasal_engine_map_point_s end = group->waypoints[group->waypoint_count - 1U];
	const size_t legs = selected ? (size_t)group->queue_count : 0U;
	float fx = ax;
	float fy = ay;
	for (size_t leg = 0U; leg <= legs; leg++) {
		if (0U < leg) {
			end = group->queue[leg - 1U];
		}
		float ex = 0.0f;
		float ey = 0.0f;
		vasal_client_camera_to_screen(camera, width, height,
			(float)end.x + 0.5f, (float)end.y + 0.5f, &ex, &ey);
		DrawLine((int)fx, (int)fy, (int)ex, (int)ey, _vasal_client_view_ring);
		fx = ex;
		fy = ey;
	}
}

// spec: client.render#2 client.render#4 client.render#5
static void _vasal_client_view_groups(const vasal_client_game_s* const game,
		const vasal_client_camera_s* const camera, const float width,
		const float height) {
	const float scale = camera->scale;
	for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
		const vasal_engine_world_group_s* const group =
			vasal_client_game_group(game, g);
		if (0U == group->troop_count) {
			continue;
		}
		float ax = 0.0f;
		float ay = 0.0f;
		vasal_client_game_anchor_at(game, g, &ax, &ay);
		vasal_client_camera_to_screen(camera, width, height, ax, ay, &ax,
			&ay);
		if (0U != (game->selection & ((uint32_t)1U << g))) {
			const bool_t focused = g == game->focus;
			DrawEllipseLines((int)ax, (int)ay, 3.4f * scale,
				3.4f * scale * vasal_client_camera_tilt, focused
				? _vasal_client_view_focus_ring : _vasal_client_view_ring);
		}
		_vasal_client_view_route(camera, width, height, group, ax, ay,
			0U != (game->selection & ((uint32_t)1U << g)));
	}
	// Troops lower on the screen are drawn later, in front.
	static vasal_client_depth_troop_s troops[_vasal_client_view_eyes_max];
	size_t count = 0U;
	for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
		const vasal_engine_world_group_s* const group =
			vasal_client_game_group(game, g);
		for (size_t t = 0U; t < group->troop_count; t++) {
			troops[count].centenar = vasal_engine_world_centenar == g;
			vasal_client_game_troop_at(game, g, t, &troops[count].x,
				&troops[count].y);
			count++;
		}
	}
	vasal_client_depth_sort(troops, count);
	for (size_t i = 0U; i < count; i++) {
		float sx = 0.0f;
		float sy = 0.0f;
		vasal_client_camera_to_screen(camera, width, height, troops[i].x,
			troops[i].y, &sx, &sy);
		const bool_t centenar = troops[i].centenar;
		const float radius = (centenar ? 0.45f : 0.32f) * scale;
		const Vector2 at = { sx, sy };
		DrawCircleV(at, radius + fmaxf(1.0f, 0.06f * scale),
			centenar ? _vasal_client_view_centenar : _vasal_client_view_accent);
		DrawCircleV(at, radius, _vasal_client_view_alliance);
	}
}

void vasal_client_view_draw(const vasal_client_game_s* const game,
	const vasal_client_camera_s* const camera, const float width,
	const float height) {
	_vasal_client_view_terrain(game, camera, width, height);
	_vasal_client_view_groups(game, camera, width, height);
}
