
/// The camera: it follows the focused group and zooms by itself, framing
/// the sight of the focused group alone, with a margin, to fit the
/// monitor rather than the window; other selected groups don't change
/// it. The window shows the middle of that picture, so resizing the window
/// crops or reveals and never rescales. Moves and zoom changes glide.
///
/// The view is isometric: the map's grid is turned by 45 degrees, so its
/// x axis runs down to the right and its y axis down to the left, and the
/// ground is squashed vertically by vasal_client_camera_tilt, so a tile is
/// a diamond twice as wide as tall. Troops and buildings are drawn
/// upright. The map is a diamond too; beyond it the window shows plain
/// ground.

#ifndef vasal__client__camera_h
#define vasal__client__camera_h

#include "client/game.h"

/// How tall the ground looks compared with how wide: circles on the
/// ground become ellipses this flat.
#define vasal_client_camera_tilt 0.5f

/// Placeholder values until tuning: the margin around the framed sight, in
/// parts of the framed extent; the widest framing, in tiles; how quickly
/// the camera glides, per second.
#define vasal_client_camera_margin 0.15f
#define vasal_client_camera_max_frame 90.0f
#define vasal_client_camera_glide 5.0f

/// The camera, in tiles and pixels.
typedef struct vasal_client_camera_s {
	/// The tile at the middle of the window.
	float x;
	float y;
	/// Pixels per tile of distance across the screen; down, it is scale
	/// times the tilt. A tile's diamond is the square root of 2 times
	/// scale wide.
	float scale;
	/// Whether the camera has a position yet; the first update jumps.
	bool_t placed;
} vasal_client_camera_s;

/// Moves the camera toward what it should show, seconds after the last
/// update, for a window of the given size on a monitor of the given size,
/// in pixels. A monitor size of 0 means it is unknown; the window's is used
/// then.
void vasal_client_camera_update(vasal_client_camera_s* const camera,
	const vasal_client_game_s* const game, const float seconds,
	const float width, const float height, const float monitor_width,
	const float monitor_height);

/// The window pixel of a point given in tiles.
void vasal_client_camera_to_screen(const vasal_client_camera_s* const camera,
	const float width, const float height, const float x, const float y,
	float* const screen_x, float* const screen_y);

/// The point, in tiles, under a window pixel.
void vasal_client_camera_to_map(const vasal_client_camera_s* const camera,
	const float width, const float height, const float screen_x,
	const float screen_y, float* const x, float* const y);

#endif
