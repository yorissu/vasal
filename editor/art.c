
/// The sheet is loaded on first use, with mipmaps, so sprites stay smooth
/// when the map is zoomed far out.

#include "editor/art.h"
#include "engine/sprites.h"

static Texture2D _vasal_editor_art_sheet;
static bool_t _vasal_editor_art_loaded = false;

static Texture2D _vasal_editor_art_texture(void) {
	if (!_vasal_editor_art_loaded) {
		_vasal_editor_art_sheet = LoadTexture(vasal_engine_sprites_file);
		GenTextureMipmaps(&_vasal_editor_art_sheet);
		SetTextureFilter(_vasal_editor_art_sheet, TEXTURE_FILTER_TRILINEAR);
		_vasal_editor_art_loaded = true;
	}
	return _vasal_editor_art_sheet;
}

static Rectangle _vasal_editor_art_source(const size_t sprite) {
	const vasal_engine_sprites_rect_s rect = vasal_engine_sprites_rect(sprite);
	const Rectangle source = {
		(float)rect.x, (float)rect.y, (float)rect.width, (float)rect.height
	};
	return source;
}

Rectangle vasal_editor_art_bounds(const size_t sprite, const Vector2 foot,
	const float pixels_per_tile) {
	const Rectangle source = _vasal_editor_art_source(sprite);
	int32_t foot_x = 0;
	int32_t foot_y = 0;
	vasal_engine_sprites_foot(sprite, &foot_x, &foot_y);
	const float size = pixels_per_tile
		/ (float)vasal_engine_sprites_pixels_per_tile;
	const Rectangle bounds = {
		foot.x - ((float)foot_x * size), foot.y - ((float)foot_y * size),
		source.width * size, source.height * size
	};
	return bounds;
}

// spec: editor.tools#6
void vasal_editor_art_standing(const size_t sprite, const Vector2 foot,
	const float pixels_per_tile, const Color tint) {
	DrawTexturePro(_vasal_editor_art_texture(),
		_vasal_editor_art_source(sprite),
		vasal_editor_art_bounds(sprite, foot, pixels_per_tile),
		(Vector2){ 0.0f, 0.0f }, 0.0f, tint);
}

// spec: editor.tools#6
void vasal_editor_art_picture(const size_t sprite, const Rectangle bounds) {
	const Rectangle source = _vasal_editor_art_source(sprite);
	float scale = bounds.width / source.width;
	if ((source.height * scale) > bounds.height) {
		scale = bounds.height / source.height;
	}
	const Rectangle target = {
		bounds.x + ((bounds.width - (source.width * scale)) / 2.0f),
		bounds.y + ((bounds.height - (source.height * scale)) / 2.0f),
		source.width * scale, source.height * scale
	};
	DrawTexturePro(_vasal_editor_art_texture(), source, target,
		(Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);
}
