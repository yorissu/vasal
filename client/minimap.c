
/// The diamond spans the map's x - y across, from -height to width, and
/// its x + y down, from 0 to width + height, so one tile of either is
/// vasal_client_minimap_width / (width + height) pixels across.

#include "client/minimap.h"

// Pixels across per tile of x - y or x + y.
static float _vasal_client_minimap_step(const vasal_engine_map_s* const map) {
	return (float)vasal_client_minimap_width
		/ ((float)map->width + (float)map->height);
}

// spec: map.minimap#1
void vasal_client_minimap_to_pixel(const vasal_engine_map_s* const map,
	const float x, const float y, float* const pixel_x,
	float* const pixel_y) {
	const float step = _vasal_client_minimap_step(map);
	*pixel_x = ((x - y) + (float)map->height) * step;
	*pixel_y = (x + y) * step * vasal_client_camera_tilt;
}

// spec: map.minimap#1
bool_t vasal_client_minimap_to_map(const vasal_engine_map_s* const map,
	const float pixel_x, const float pixel_y, float* const x,
	float* const y) {
	const float step = _vasal_client_minimap_step(map);
	const float across = (pixel_x / step) - (float)map->height;
	const float down = pixel_y / (step * vasal_client_camera_tilt);
	*x = (across + down) * 0.5f;
	*y = (down - across) * 0.5f;
	return (*x >= 0.0f) && (*y >= 0.0f) && (*x < (float)map->width)
		&& (*y < (float)map->height);
}

// spec: map.minimap#1
uint32_t vasal_client_minimap_color(const vasal_engine_map_s* const map,
	const uint16_t x, const uint16_t y) {
	const uint32_t rgba = vasal_engine_map_terrain(map, x, y)->color;
	const uint32_t red = (rgba >> 24U) & 0xffU;
	const uint32_t green = (rgba >> 16U) & 0xffU;
	const uint32_t blue = (rgba >> 8U) & 0xffU;
	// Its brightness, as the eye weighs the three.
	const uint32_t gray = ((red * 30U) + (green * 59U) + (blue * 11U))
		/ 100U;
	return (gray << 24U) | (gray << 16U) | (gray << 8U) | 0xffU;
}

void vasal_client_minimap_panel(const float width, const float height,
	float* const left, float* const top) {
	const float padding = (float)vasal_client_minimap_padding;
	const float margin = (float)vasal_client_minimap_margin;
	*left = width - margin - (float)vasal_client_minimap_width
		- (2.0f * padding);
	*top = height - margin - (float)vasal_client_minimap_height
		- (2.0f * padding);
}

// spec: map.minimap#1
bool_t vasal_client_minimap_contains(const float width, const float height,
	const float screen_x, const float screen_y) {
	float left = 0.0f;
	float top = 0.0f;
	vasal_client_minimap_panel(width, height, &left, &top);
	const float padding = (float)vasal_client_minimap_padding;
	return (screen_x >= left) && (screen_y >= top)
		&& (screen_x < (left + (float)vasal_client_minimap_width
			+ (2.0f * padding)))
		&& (screen_y < (top + (float)vasal_client_minimap_height
			+ (2.0f * padding)));
}
