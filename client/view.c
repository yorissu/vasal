
/// The ground is one continuous plane, drawn by client/ground.c. Only
/// when the graphics can't run its shader is it drawn here instead, tile by
/// tile and only the tiles in the window, each grayed by how far inside the
/// nearest troop's sight it lies, from 0 (outside) to 1 (well inside), over
/// the last tile of the radius.

#include "client/view.h"
#include "client/depth.h"
#include "client/ground.h"
#include "engine/sprites.h"

#include <raylib.h>

#include <math.h>

// The player's alliance color, and the accent around their troops.
#define _vasal_client_view_alliance (Color){ 214, 64, 52, 255 }
#define _vasal_client_view_accent (Color){ 255, 236, 200, 255 }
#define _vasal_client_view_centenar (Color){ 236, 190, 60, 255 }
#define _vasal_client_view_ring (Color){ 255, 255, 255, 140 }
#define _vasal_client_view_focus_ring (Color){ 255, 255, 255, 230 }
// Other vasals' troops, until alliances have colors: a dark rim; bandits'
// brown; arrows.
#define _vasal_client_view_other_rim (Color){ 50, 56, 90, 255 }
#define _vasal_client_view_bandit_rim (Color){ 90, 52, 30, 255 }
#define _vasal_client_view_arrow (Color){ 40, 30, 20, 255 }

// The most troops and sprites drawn at once.
#define _vasal_client_view_troops_max                                         \
	((vasal_engine_world_vasals_max * vasal_engine_world_groups               \
		* vasal_engine_world_troops) + vasal_engine_map_settlements_max       \
		+ vasal_engine_map_decorations_max)

// The sheet of settlement and structure sprites (engine/sprites.h).
static Texture2D _vasal_client_view_sheet;
static bool_t _vasal_client_view_sheet_loaded = false;

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
// spec: map.sight#1 map.sight#2 map.sight#3
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
}

// Whether a point on the screen is close enough to the window to draw a
// troop there.
static bool_t _vasal_client_view_on_screen(const float sx, const float sy,
		const float width, const float height, const float reach) {
	return (sx > -reach) && (sy > -reach) && (sx < (width + reach))
		&& (sy < (height + reach));
}

// Gathers the troops to draw: the player's in the window, then other
// vasals' in the window and in the player's sight. Returns how many.
// spec: map.sight#1 client.render#1
static size_t _vasal_client_view_gather(const vasal_client_game_s* const game,
		const vasal_client_camera_s* const camera, const float width,
		const float height, vasal_client_depth_troop_s* const troops) {
	const vasal_engine_world_s* const world = game->world;
	const float reach = camera->scale;
	size_t count = 0U;
	const float precision = (float)vasal_engine_map_precision;
	for (size_t v = 0U; v < world->vasal_count; v++) {
		const bool_t own = v == game->vasal;
		for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
			const vasal_engine_world_group_s* const group =
				&world->vasals[v].groups[g];
			// A group whose anchor is well off the window has no troop in
			// it: troops keep within a few tiles of their anchor.
			float ax = 0.0f;
			float ay = 0.0f;
			vasal_client_camera_to_screen(camera, width, height,
				(float)group->anchor.x / precision,
				(float)group->anchor.y / precision, &ax, &ay);
			const size_t troops_count = _vasal_client_view_on_screen(ax, ay,
				width, height, 8.0f * reach) ? group->troop_count : 0U;
			for (size_t t = 0U; t < troops_count; t++) {
				float x = 0.0f;
				float y = 0.0f;
				vasal_client_game_position_at(game, v, g, t, &x, &y);
				float sx = 0.0f;
				float sy = 0.0f;
				vasal_client_camera_to_screen(camera, width, height, x, y,
					&sx, &sy);
				const bool_t shown = _vasal_client_view_on_screen(sx, sy,
					width, height, reach) && (own
						|| (vasal_client_game_sight_at(game, x, y) > 0.0f));
				if (shown) {
					troops[count].x = x;
					troops[count].y = y;
					troops[count].centenar = vasal_engine_world_centenar == g;
					troops[count].own = own;
					troops[count].sprite = -1;
					troops[count].sight = 1.0f;
					troops[count].unit = group->troops[t].unit;
					troops[count].wounded = group->troops[t].wounded;
					troops[count].npc = world->vasals[v].npc;
					count++;
				}
			}
		}
	}
	return count;
}

// Adds a sprite standing at a tile to the things to draw, if it reaches
// into the window. Returns the new count.
static size_t _vasal_client_view_sprite(const vasal_client_game_s* const game,
		const vasal_client_camera_s* const camera, const float width,
		const float height, const vasal_engine_map_point_s at,
		const size_t sprite, vasal_client_depth_troop_s* const things,
		const size_t count) {
	const float x = (float)at.x + 0.5f;
	const float y = (float)at.y + 0.5f;
	float sx = 0.0f;
	float sy = 0.0f;
	vasal_client_camera_to_screen(camera, width, height, x, y, &sx, &sy);
	const vasal_engine_sprites_rect_s rect = vasal_engine_sprites_rect(sprite);
	const float reach = ((float)rect.width + (float)rect.height)
		/ (float)vasal_engine_sprites_pixels_per_tile * camera->scale;
	if (!_vasal_client_view_on_screen(sx, sy, width, height, reach)) {
		return count;
	}
	things[count].x = x;
	things[count].y = y;
	things[count].centenar = false;
	things[count].own = false;
	things[count].sprite = (int32_t)sprite;
	things[count].sight = vasal_client_game_sight_at(game, x, y);
	things[count].unit = 0U;
	things[count].wounded = false;
	things[count].npc = false;
	return count + 1U;
}

// Gathers the map's settlements and lone structures in the window. They
// are shown inside sight or not: the map is known.
// spec: client.render#7 map.sight#4
static size_t _vasal_client_view_structures(
		const vasal_client_game_s* const game,
		const vasal_client_camera_s* const camera, const float width,
		const float height, vasal_client_depth_troop_s* const things,
		const size_t count) {
	const vasal_engine_map_s* const map = game->world->map;
	size_t total = count;
	for (size_t i = 0U; i < map->settlement_count; i++) {
		total = _vasal_client_view_sprite(game, camera, width, height,
			map->settlements[i].position,
			vasal_engine_sprites_settlement(&map->settlements[i]), things,
			total);
	}
	for (size_t i = 0U; i < map->decoration_count; i++) {
		total = _vasal_client_view_sprite(game, camera, width, height,
			map->decorations[i].position,
			vasal_engine_sprites_decoration(&map->decorations[i]), things,
			total);
	}
	return total;
}

// Draws a sprite with its foot on a window pixel, grayed outside sight as
// the ground is.
static void _vasal_client_view_draw_sprite(
		const vasal_client_camera_s* const camera,
		const vasal_client_depth_troop_s* const thing, const float sx,
		const float sy) {
	if (!_vasal_client_view_sheet_loaded) {
		_vasal_client_view_sheet = LoadTexture(vasal_engine_sprites_file);
		GenTextureMipmaps(&_vasal_client_view_sheet);
		SetTextureFilter(_vasal_client_view_sheet,
			TEXTURE_FILTER_TRILINEAR);
		_vasal_client_view_sheet_loaded = true;
	}
	const size_t sprite = (size_t)thing->sprite;
	const vasal_engine_sprites_rect_s rect = vasal_engine_sprites_rect(sprite);
	int32_t foot_x = 0;
	int32_t foot_y = 0;
	vasal_engine_sprites_foot(sprite, &foot_x, &foot_y);
	const float size = camera->scale
		/ (float)vasal_engine_sprites_pixels_per_tile;
	const Rectangle source = {
		(float)rect.x, (float)rect.y, (float)rect.width, (float)rect.height
	};
	const Rectangle target = {
		sx - ((float)foot_x * size), sy - ((float)foot_y * size),
		(float)rect.width * size,
		(float)rect.height * size
	};
	const float light = 150.0f + (105.0f * thing->sight);
	const unsigned char shade = (unsigned char)light;
	const Color tint = { shade, shade, shade, 255U };
	DrawTexturePro(_vasal_client_view_sheet, source, target,
		(Vector2){ 0.0f, 0.0f }, 0.0f, tint);
}

// A color darkened to a share in percent.
static Color _vasal_client_view_darker(const Color color, const int share) {
	const int r = ((int)color.r * share) / 100;
	const int g = ((int)color.g * share) / 100;
	const int b = ((int)color.b * share) / 100;
	const Color darker = {
		(unsigned char)r, (unsigned char)g, (unsigned char)b, color.a
	};
	return darker;
}

// Draws a troop: a rim in its side's color (the player's bright, others'
// dark, bandits' brown) around its unit type's color, darker when
// wounded; centenars larger, with a golden rim.
// spec: client.render#4 rules.content#1
static void _vasal_client_view_troop(const vasal_client_game_s* const game,
		const vasal_client_depth_troop_s* const troop, const float sx,
		const float sy, const float scale) {
	const vasal_engine_rules_s* const rules = game->world->rules;
	const bool_t centenar = troop->centenar;
	const float radius = (centenar ? 0.45f : 0.32f) * scale;
	const Vector2 at = { sx, sy };
	Color rim = _vasal_client_view_other_rim;
	if (troop->own) {
		rim = centenar ? _vasal_client_view_centenar
			: _vasal_client_view_alliance;
	} else if (troop->npc) {
		rim = _vasal_client_view_bandit_rim;
	} else {
		// Another vasal's.
	}
	Color body = _vasal_client_view_color(
		rules->units[troop->unit % rules->unit_count].color);
	if (!troop->own) {
		body = _vasal_client_view_darker(body, 75);
	}
	if (troop->wounded) {
		body = _vasal_client_view_darker(body, 55);
	}
	DrawCircleV(at, radius + fmaxf(1.5f, 0.12f * scale), rim);
	DrawCircleV(at, radius, body);
}

// Draws the shots of the last tick, as short arrows flying from the
// shooter to the target over the tick.
// spec: combat.ranged#5
static void _vasal_client_view_shots(const vasal_client_game_s* const game,
		const vasal_client_camera_s* const camera, const float width,
		const float height) {
	const vasal_engine_world_s* const world = game->world;
	const float part = fminf(game->pending
		* (float)vasal_engine_world_tick_rate, 1.0f);
	const float precision = (float)vasal_engine_map_precision;
	for (size_t i = 0U; i < world->shot_count; i++) {
		const vasal_engine_world_shot_s* const shot = &world->shots[i];
		const float fx = (float)shot->from.x / precision;
		const float fy = (float)shot->from.y / precision;
		const float tx = (float)shot->to.x / precision;
		const float ty = (float)shot->to.y / precision;
		const float head = fminf(part + 0.25f, 1.0f);
		float ax = 0.0f;
		float ay = 0.0f;
		float bx = 0.0f;
		float by = 0.0f;
		vasal_client_camera_to_screen(camera, width, height,
			fx + ((tx - fx) * part), fy + ((ty - fy) * part), &ax, &ay);
		vasal_client_camera_to_screen(camera, width, height,
			fx + ((tx - fx) * head), fy + ((ty - fy) * head), &bx, &by);
		// Arrows fly above the ground.
		const float lift = 0.6f * camera->scale;
		DrawLineEx((Vector2){ ax, ay - lift }, (Vector2){ bx, by - lift },
			fmaxf(1.0f, 0.05f * camera->scale), _vasal_client_view_arrow);
	}
}

// Draws troops lower on the screen later, in front.
// spec: client.render#2 client.render#4
static size_t _vasal_client_view_troops(const vasal_client_game_s* const game,
		const vasal_client_camera_s* const camera, const float width,
		const float height) {
	static vasal_client_depth_troop_s troops[_vasal_client_view_troops_max];
	const size_t count = _vasal_client_view_structures(game, camera, width,
		height, troops, _vasal_client_view_gather(game, camera, width,
			height, troops));
	vasal_client_depth_sort(troops, count);
	const float scale = camera->scale;
	for (size_t i = 0U; i < count; i++) {
		float sx = 0.0f;
		float sy = 0.0f;
		vasal_client_camera_to_screen(camera, width, height, troops[i].x,
			troops[i].y, &sx, &sy);
		if (0 <= troops[i].sprite) {
			_vasal_client_view_draw_sprite(camera, &troops[i], sx, sy);
			continue;
		}
		_vasal_client_view_troop(game, &troops[i], sx, sy, scale);
	}
	return count;
}

size_t vasal_client_view_draw(const vasal_client_game_s* const game,
	const vasal_client_camera_s* const camera, const float width,
	const float height) {
	if (!vasal_client_ground_draw(game, camera, width, height)) {
		_vasal_client_view_terrain(game, camera, width, height);
	}
	_vasal_client_view_groups(game, camera, width, height);
	const size_t drawn = _vasal_client_view_troops(game, camera, width,
		height);
	_vasal_client_view_shots(game, camera, width, height);
	return drawn;
}
