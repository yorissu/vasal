
/// The sprites of settlements and lone structures: where each stands on
/// the sheet art/structures.png, which the client carries inside itself,
/// which sprite a settlement or decoration of the map shows, and where its
/// foot is, the point that stands on the settlement's or decoration's
/// position. Free of any graphics library, so it can be tested; the view
/// draws them.
///
/// The sheet holds, in rows: 8 villages, 6 towns, 6 cities, 7 castles,
/// and 10 lone structures (4 watchtowers, 3 chapels, a windmill, a bridge
/// and a ruin). A settlement shows one of its type's sprites, picked by its
/// position, so a map always looks the same; a decoration shows the lone
/// structure of its kind.

#ifndef vasal__client__sprites_h
#define vasal__client__sprites_h

#include "engine/map.h"

/// Where the sheet is inside the client.
#define vasal_client_sprites_file "/art/structures.png"

/// How many sprites the sheet holds.
#define vasal_client_sprites_count 37U

/// How many lone structures, the decorations' sprites.
#define vasal_client_sprites_structures 10U

/// How many of the sheet's pixels make one tile across the screen, for
/// every sprite alike (placeholder until tuning): a village is about 4
/// tiles wide, a city about 6.5, a house about two troops wide.
#define vasal_client_sprites_pixels_per_tile 36.0f

/// A sprite's rectangle on the sheet, in pixels.
typedef struct vasal_client_sprites_rect_s {
	int32_t x;
	int32_t y;
	int32_t width;
	int32_t height;
} vasal_client_sprites_rect_s;

/// A sprite's rectangle on the sheet.
vasal_client_sprites_rect_s vasal_client_sprites_rect(const size_t sprite);

/// The sprite a settlement shows.
size_t vasal_client_sprites_settlement(
	const vasal_engine_map_settlement_s* const settlement);

/// The sprite a decoration shows.
size_t vasal_client_sprites_decoration(
	const vasal_engine_map_decoration_s* const decoration);

/// A sprite's foot, in pixels from its rectangle's top left corner: the
/// middle of the ground it stands on, which the art draws as a diamond as
/// wide as the sprite.
void vasal_client_sprites_foot(const size_t sprite, float* const x,
	float* const y);

#endif
