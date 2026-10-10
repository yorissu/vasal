
/// The boxes stand in a row, one per group, in the order of their keys.

#include "client/layout.h"
#include "client/minimap.h"

void vasal_client_layout_group(const float height, const size_t group,
	int32_t* const left, int32_t* const top) {
	*left = vasal_client_layout_margin + ((int32_t)group
		* (vasal_client_layout_box + vasal_client_layout_gap));
	*top = (int32_t)height - vasal_client_layout_box
		- vasal_client_layout_margin;
}

// spec: vintaines.actions#7
bool_t vasal_client_layout_on_interface(const float width,
	const float height, const float x, const float y) {
	int32_t left = 0;
	int32_t top = 0;
	vasal_client_layout_group(height, 0U, &left, &top);
	const int32_t right = left + ((int32_t)vasal_engine_world_groups
		* (vasal_client_layout_box + vasal_client_layout_gap))
		- vasal_client_layout_gap;
	const int32_t bottom = top + vasal_client_layout_box;
	const bool_t on_groups = (x >= (float)left) && (x < (float)right)
		&& (y >= (float)top) && (y < (float)bottom);
	return on_groups || vasal_client_minimap_contains(width, height, x, y);
}
