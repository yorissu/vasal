
/// Where the interface's panels stand in the window, free of any graphics
/// library so it can be tested: the groups' boxes along the bottom left
/// and the minimap in the bottom right corner (client/minimap.h). The
/// mouse works the map everywhere else.

#ifndef vasal__client__layout_h
#define vasal__client__layout_h

#include "engine/types.h"

/// A group's box: its side, the gap between boxes, and the space between
/// them and the window's edges, in pixels.
#define vasal_client_layout_box 64
#define vasal_client_layout_gap 6
#define vasal_client_layout_margin 12

/// The top left corner of a group's box in a window this tall, in pixels.
void vasal_client_layout_group(const float height, const size_t group,
	int32_t* const left, int32_t* const top);

/// Whether a window pixel is on a panel of the interface.
bool_t vasal_client_layout_on_interface(const float width,
	const float height, const float x, const float y);

#endif
