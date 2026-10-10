
/// Drawing order: in the isometric view, things lower on the screen stand
/// in front, so they are drawn later. How low a point is on the screen
/// grows with x + y on the map, so sorting is by that sum.

#ifndef vasal__client__depth_h
#define vasal__client__depth_h

#include "engine/types.h"

/// A troop to draw: where, in tiles, and whether it is a centenar.
typedef struct vasal_client_depth_troop_s {
	float  x;
	float  y;
	bool_t centenar;
} vasal_client_depth_troop_s;

/// Sorts troops by x + y, lowest first, keeping the order of equal ones.
void vasal_client_depth_sort(vasal_client_depth_troop_s* const troops,
	const size_t count);

#endif
