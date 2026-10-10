
/// Paths across the map, in two levels: the map is split into square
/// regions, a coarse search finds a way through regions, and a fine search
/// finds the tiles within the corridor of regions along it. Paths are the
/// fastest, not the shortest: crossing a tile costs more the slower its
/// terrain is. When the goal can't be reached, the path leads to the
/// nearest point that can.
///
/// The module keeps the prepared map and its search buffers in static
/// memory, so it serves one map at a time.

#ifndef vasal__engine__path_h
#define vasal__engine__path_h

#include "engine/map.h"

/// The side of a region, in tiles.
#define vasal_engine_path_region_side 16U

/// The most points a path has.
#define vasal_engine_path_points_max 2048U

/// Prepares the map for path searches: region costs and links. Must be
/// called again after the map's terrain changes. The map must outlive its
/// use here.
void vasal_engine_path_prepare(const vasal_engine_map_s* const map);

/// Finds the fastest path from one tile to another and writes its turning
/// points into points, up to capacity, ending with the goal (or the nearest
/// tile reachable from it). The path is pulled straight, so the line
/// between two points (and from the start to the first) may run at any
/// angle; it crosses only passable tiles and is never slower than the
/// tile-by-tile way it replaces. Returns how many points were written: 0 when
/// the start is impassable or off the map.
size_t vasal_engine_path_find(const vasal_engine_map_point_s from,
	const vasal_engine_map_point_s to, vasal_engine_map_point_s* const points,
	const size_t capacity);

/// Whether a straight line between two positions, in
/// 1/vasal_engine_map_precision of a tile, crosses only passable tiles of
/// the prepared map, all on the map. Where it passes exactly through a
/// tile corner, both tiles beside the corner must be passable too, as for
/// a diagonal step.
bool_t vasal_engine_path_clear(const int32_t from_x, const int32_t from_y,
	const int32_t to_x, const int32_t to_y);

/// The cost of crossing one tile straight on the prepared map: 10000
/// divided by the terrain's speed, 0 when it is impassable.
uint32_t vasal_engine_path_step_cost(const uint16_t x, const uint16_t y);

#endif
