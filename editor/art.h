
/// The sprites of settlements and lone structures in the editor, from the
/// same sheet as the game (engine/sprites.h), which the editor carries
/// inside itself: drawn on the map standing on their tile, and as pictures
/// in the panels to choose from.

#ifndef vasal__editor__art_h
#define vasal__editor__art_h

#include "engine/types.h"

#include <raylib.h>

/// Draws a sprite with its foot on a window pixel, at a size in window
/// pixels per tile, tinted.
void vasal_editor_art_standing(const size_t sprite, const Vector2 foot,
	const float pixels_per_tile, const Color tint);

/// Draws a sprite as large as fits into a rectangle, centered.
void vasal_editor_art_picture(const size_t sprite, const Rectangle bounds);

/// The rectangle a sprite standing with its foot on a window pixel covers,
/// at a size in window pixels per tile.
Rectangle vasal_editor_art_bounds(const size_t sprite, const Vector2 foot,
	const float pixels_per_tile);

#endif
