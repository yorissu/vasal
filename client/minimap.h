
/// The minimap: the whole map in the bottom right corner of the window,
/// as a diamond like the main view, its grid turned by 45 degrees and
/// squashed by vasal_client_camera_tilt. Territory is drawn in its
/// alliance's color and everything else in gray; until there is territory
/// (map.regions), everything is gray, each terrain type its own shade. The
/// player's troops in sight show on it. It only shows: clicks on it do
/// nothing. These are its geometry and colors, free of any graphics
/// library so they can be tested; the HUD draws it.

#ifndef vasal__client__minimap_h
#define vasal__client__minimap_h

#include "client/camera.h"

/// The minimap's diamond, in pixels: this wide, and as much less tall as
/// the main view's ground is squashed, vasal_client_camera_tilt
/// (placeholder size until the HUD is designed).
#define vasal_client_minimap_width 240
#define vasal_client_minimap_height 120

/// The space around the diamond, inside its panel, and between the panel
/// and the window's edges, in pixels.
#define vasal_client_minimap_padding 8
#define vasal_client_minimap_margin 12

/// The pixel of the minimap's diamond, from its top left corner, where a
/// point on the map, in tiles, is shown.
void vasal_client_minimap_to_pixel(const vasal_engine_map_s* const map,
	const float x, const float y, float* const pixel_x,
	float* const pixel_y);

/// The point on the map, in tiles, that a pixel of the minimap's diamond
/// shows. Returns false when the pixel is off the map.
bool_t vasal_client_minimap_to_map(const vasal_engine_map_s* const map,
	const float pixel_x, const float pixel_y, float* const x,
	float* const y);

/// The color, 0xRRGGBBAA, the minimap shows a tile in: its terrain's color
/// as a shade of gray, since no alliance holds it.
uint32_t vasal_client_minimap_color(const vasal_engine_map_s* const map,
	const uint16_t x, const uint16_t y);

/// Where the minimap's panel stands in a window of the given size: its
/// top left corner, in pixels. The diamond starts the padding further in.
void vasal_client_minimap_panel(const float width, const float height,
	float* const left, float* const top);

/// Whether a window pixel is on the minimap's panel, so that a click there
/// gives no order.
bool_t vasal_client_minimap_contains(const float width, const float height,
	const float screen_x, const float screen_y);

#endif
