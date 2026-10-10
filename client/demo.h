
/// A built-in demonstration map, used when the server offers no map file:
/// grass with a river and a ford, a road, forests and tall grass, so every
/// kind of terrain can be tried before the editor makes real maps.

#ifndef vasal__client__demo_h
#define vasal__client__demo_h

#include "engine/map.h"

/// The side of the demonstration map, in tiles.
#define vasal_client_demo_side 256U

/// Where the player's centaine starts on the demonstration map.
#define vasal_client_demo_spawn_x 60U
#define vasal_client_demo_spawn_y 120U

/// Fills the map with the demonstration map.
void vasal_client_demo_make(vasal_engine_map_s* const map);

#endif
