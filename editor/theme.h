
/// The editor's look: a dark theme for raygui, the colors the editor draws
/// with itself, and its typeface (DejaVu Sans, carried inside the editor
/// like the art) at the sizes it uses, with helpers to draw and measure
/// text in it.

#ifndef vasal__editor__theme_h
#define vasal__editor__theme_h

#include "engine/types.h"

#include <raylib.h>

/// The colors.
#define vasal_editor_theme_background (Color){ 24, 25, 28, 255 }
#define vasal_editor_theme_panel (Color){ 34, 36, 40, 255 }
#define vasal_editor_theme_raised (Color){ 44, 47, 53, 255 }
#define vasal_editor_theme_hover (Color){ 56, 60, 68, 255 }
#define vasal_editor_theme_border (Color){ 62, 66, 74, 255 }
#define vasal_editor_theme_foreground (Color){ 222, 225, 230, 255 }
#define vasal_editor_theme_dim (Color){ 140, 146, 156, 255 }
#define vasal_editor_theme_accent (Color){ 78, 141, 255, 255 }
#define vasal_editor_theme_accent_dark (Color){ 38, 74, 140, 255 }
#define vasal_editor_theme_good (Color){ 70, 185, 90, 255 }
#define vasal_editor_theme_warning (Color){ 220, 160, 50, 255 }
#define vasal_editor_theme_bad (Color){ 232, 86, 76, 255 }

/// The text sizes, in pixels.
#define vasal_editor_theme_small 13
#define vasal_editor_theme_normal 16
#define vasal_editor_theme_large 20

/// Loads the typeface and sets raygui's style. Call once, after the
/// window is open.
void vasal_editor_theme_load(void);

/// Draws text with its top left corner at a point, in the regular or bold
/// face, at one of the sizes above.
void vasal_editor_theme_text(const char* const text, const float x,
	const float y, const int size, const bool_t bold, const Color color);

/// How wide text is, in pixels.
float vasal_editor_theme_measure(const char* const text, const int size,
	const bool_t bold);

/// Draws text wrapped at word boundaries into a width, starting at a
/// point. Returns the height it took.
float vasal_editor_theme_wrapped(const char* const text, const float x,
	const float y, const float width, const int size, const Color color);

/// How tall text wrapped into a width is, without drawing it.
float vasal_editor_theme_wrapped_height(const char* const text,
	const float width, const int size);

/// A color from 0xRRGGBBAA, and back.
Color vasal_editor_theme_color(const uint32_t rgba);
uint32_t vasal_editor_theme_rgba(const Color color);

#endif
