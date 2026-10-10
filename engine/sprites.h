
/// The sprites of settlements and lone structures: where each stands on
/// the sheet art/structures.png, which the client and the editor carry
/// inside themselves, which sprite a settlement or decoration of the map
/// shows, and where its foot is, the point that stands on the settlement's
/// or decoration's position. Data only, shared through the engine because
/// the client and the editor draw the same sprites.
///
/// The sheet holds, in rows: 8 villages, 6 towns, 6 cities, 7 castles,
/// and 10 lone structures (4 watchtowers, 3 chapels, a windmill, a bridge
/// and a ruin). A settlement shows one of its type's sprites, picked by its
/// position, so a map always looks the same; a decoration shows the lone
/// structure of its kind.

#ifndef vasal__engine__sprites_h
#define vasal__engine__sprites_h

#include "engine/map.h"

/// Where the sheet is inside the client.
#define vasal_engine_sprites_file "/art/structures.png"

/// How many sprites the sheet holds.
#define vasal_engine_sprites_count 37U

/// How many lone structures, the decorations' sprites.
#define vasal_engine_sprites_structures 10U

/// How many of the sheet's pixels make one tile across the screen, for
/// every sprite alike (placeholder until tuning): a village is about 4
/// tiles wide, a city about 6.5, a house about two troops wide.
#define vasal_engine_sprites_pixels_per_tile 36

/// A sprite's rectangle on the sheet, in pixels.
typedef struct vasal_engine_sprites_rect_s {
	int32_t x;
	int32_t y;
	int32_t width;
	int32_t height;
} vasal_engine_sprites_rect_s;

/// A sprite's rectangle on the sheet.
vasal_engine_sprites_rect_s vasal_engine_sprites_rect(const size_t sprite);

/// The sprite a settlement shows.
size_t vasal_engine_sprites_settlement(
	const vasal_engine_map_settlement_s* const settlement);

/// The sprite a decoration shows.
size_t vasal_engine_sprites_decoration(
	const vasal_engine_map_decoration_s* const decoration);

/// A sprite's foot, in pixels from its rectangle's top left corner: the
/// middle of the ground it stands on, which the art draws as a diamond as
/// wide as the sprite.
void vasal_engine_sprites_foot(const size_t sprite, int32_t* const x,
	int32_t* const y);

/// The first sprite of a settlement type, which the editor shows as the
/// type's picture.
size_t vasal_engine_sprites_type(
	const vasal_engine_map_settlement_type_e type);

#endif
