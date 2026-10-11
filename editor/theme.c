
/// Each face is loaded once per size it is drawn at, so text is drawn at
/// its own size and stays sharp. raygui draws with the regular face at the
/// normal size.

#include "editor/theme.h"

#include <raygui.h>

#include <string.h>

#define _vasal_editor_theme_regular_file "/art/fonts/dejavu_sans.ttf"
#define _vasal_editor_theme_bold_file "/art/fonts/dejavu_sans_bold.ttf"

// The faces: regular and bold, at the small, normal and large sizes.
static Font _vasal_editor_theme_fonts[2][3];
static bool_t _vasal_editor_theme_loaded = false;

static int _vasal_editor_theme_int(const Color color) {
	return ColorToInt(color);
}

static size_t _vasal_editor_theme_size_index(const int size) {
	size_t index = 1U;
	if (vasal_editor_theme_small >= size) {
		index = 0U;
	} else if (vasal_editor_theme_large <= size) {
		index = 2U;
	} else {
		// The normal size.
	}
	return index;
}

static Font _vasal_editor_theme_font(const int size, const bool_t bold) {
	if (!_vasal_editor_theme_loaded) {
		return GetFontDefault();
	}
	return _vasal_editor_theme_fonts[bold ? 1U : 0U]
		[_vasal_editor_theme_size_index(size)];
}

// The raygui colors of one state of every control.
static void _vasal_editor_theme_state(const int border, const int base,
		const int text, const Color border_color, const Color base_color,
		const Color text_color) {
	GuiSetStyle(DEFAULT, border, _vasal_editor_theme_int(border_color));
	GuiSetStyle(DEFAULT, base, _vasal_editor_theme_int(base_color));
	GuiSetStyle(DEFAULT, text, _vasal_editor_theme_int(text_color));
}

void vasal_editor_theme_load(void) {
	static const int sizes[3] = {
		vasal_editor_theme_small, vasal_editor_theme_normal,
		vasal_editor_theme_large
	};
	static const char* const files[2] = {
		_vasal_editor_theme_regular_file, _vasal_editor_theme_bold_file
	};
	for (size_t face = 0U; face < 2U; face++) {
		for (size_t size = 0U; size < 3U; size++) {
			Font* const font = &_vasal_editor_theme_fonts[face][size];
			*font = LoadFontEx(files[face], sizes[size], NULL, 0);
			SetTextureFilter(font->texture, TEXTURE_FILTER_BILINEAR);
		}
	}
	_vasal_editor_theme_loaded = true;
	GuiSetFont(_vasal_editor_theme_fonts[0][1]);

	_vasal_editor_theme_state(BORDER_COLOR_NORMAL, BASE_COLOR_NORMAL,
		TEXT_COLOR_NORMAL, vasal_editor_theme_border,
		vasal_editor_theme_raised, vasal_editor_theme_foreground);
	_vasal_editor_theme_state(BORDER_COLOR_FOCUSED, BASE_COLOR_FOCUSED,
		TEXT_COLOR_FOCUSED, vasal_editor_theme_accent,
		vasal_editor_theme_hover, vasal_editor_theme_foreground);
	_vasal_editor_theme_state(BORDER_COLOR_PRESSED, BASE_COLOR_PRESSED,
		TEXT_COLOR_PRESSED, vasal_editor_theme_accent,
		vasal_editor_theme_accent_dark, vasal_editor_theme_foreground);
	_vasal_editor_theme_state(BORDER_COLOR_DISABLED, BASE_COLOR_DISABLED,
		TEXT_COLOR_DISABLED, vasal_editor_theme_panel,
		vasal_editor_theme_panel, vasal_editor_theme_dim);
	GuiSetStyle(DEFAULT, BORDER_WIDTH, 1);
	GuiSetStyle(DEFAULT, TEXT_SIZE, vasal_editor_theme_normal);
	GuiSetStyle(DEFAULT, TEXT_SPACING, 0);
	GuiSetStyle(DEFAULT, TEXT_PADDING, 6);
	GuiSetStyle(DEFAULT, LINE_COLOR,
		_vasal_editor_theme_int(vasal_editor_theme_border));
	GuiSetStyle(DEFAULT, BACKGROUND_COLOR,
		_vasal_editor_theme_int(vasal_editor_theme_panel));
	// Labels sit on panels: no box, dim when disabled.
	GuiSetStyle(LABEL, TEXT_COLOR_NORMAL,
		_vasal_editor_theme_int(vasal_editor_theme_foreground));
	GuiSetStyle(LABEL, TEXT_COLOR_FOCUSED,
		_vasal_editor_theme_int(vasal_editor_theme_foreground));
	GuiSetStyle(LABEL, TEXT_COLOR_PRESSED,
		_vasal_editor_theme_int(vasal_editor_theme_foreground));
	GuiSetStyle(TEXTBOX, BASE_COLOR_PRESSED,
		_vasal_editor_theme_int(vasal_editor_theme_background));
	GuiSetStyle(VALUEBOX, BASE_COLOR_PRESSED,
		_vasal_editor_theme_int(vasal_editor_theme_background));
	GuiSetStyle(SLIDER, BASE_COLOR_PRESSED,
		_vasal_editor_theme_int(vasal_editor_theme_accent));
	GuiSetStyle(CHECKBOX, BASE_COLOR_PRESSED,
		_vasal_editor_theme_int(vasal_editor_theme_accent));
	GuiSetStyle(LISTVIEW, BASE_COLOR_NORMAL,
		_vasal_editor_theme_int(vasal_editor_theme_panel));
	GuiEnableTooltip();
}

void vasal_editor_theme_text(const char* const text, const float x,
	const float y, const int size, const bool_t bold, const Color color) {
	const Font font = _vasal_editor_theme_font(size, bold);
	DrawTextEx(font, text, (Vector2){ x, y }, (float)size, 0.0f, color);
}

float vasal_editor_theme_measure(const char* const text, const int size,
	const bool_t bold) {
	const Font font = _vasal_editor_theme_font(size, bold);
	return MeasureTextEx(font, text, (float)size, 0.0f).x;
}

// Text being wrapped: where it goes, the line being gathered, and the
// height taken so far.
typedef struct _vasal_editor_theme_lines_s {
	float  x;
	float  y;
	float  width;
	int    size;
	Color  color;
	bool_t drawing;
	char   line[256];
	size_t length;
	float  height;
} _vasal_editor_theme_lines_s;

// Ends the line gathered: draws it when drawing, and moves down.
static void _vasal_editor_theme_flush(
		_vasal_editor_theme_lines_s* const lines) {
	lines->line[lines->length] = '\0';
	if (lines->drawing) {
		vasal_editor_theme_text(lines->line, lines->x,
			lines->y + lines->height, lines->size, false, lines->color);
	}
	lines->height += (float)lines->size * 1.3f;
	lines->length = 0U;
}

// Where the word from at ends: after the spaces before it and its letters,
// at most 200 of them.
static size_t _vasal_editor_theme_word_end(const char* const text,
		const size_t at) {
	size_t end = at;
	while (' ' == text[end]) {
		end++;
	}
	while (('\0' != text[end]) && (' ' != text[end]) && ('\n' != text[end])
			&& ((end - at) < 200U)) {
		end++;
	}
	return end;
}

// Whether a word added to the line gathered would make it wider than the
// width.
static bool_t _vasal_editor_theme_too_wide(
		const _vasal_editor_theme_lines_s* const lines,
		const char* const word, const size_t length) {
	char candidate[256];
	if ((lines->length + length) >= sizeof(candidate)) {
		return true;
	}
	(void)memcpy(candidate, lines->line, lines->length);
	(void)memcpy(&candidate[lines->length], word, length);
	candidate[lines->length + length] = '\0';
	return (0U < lines->length) && (vasal_editor_theme_measure(candidate,
		lines->size, false) > lines->width);
}

// Walks text wrapped into the width, drawing each line when asked. Returns
// the height taken.
static float _vasal_editor_theme_wrap(
		_vasal_editor_theme_lines_s* const lines, const char* const text) {
	size_t at = 0U;
	while ('\0' != text[at]) {
		const size_t end = _vasal_editor_theme_word_end(text, at);
		const size_t length = end - at;
		if (_vasal_editor_theme_too_wide(lines, &text[at], length)) {
			_vasal_editor_theme_flush(lines);
			while (' ' == text[at]) {
				at++;
			}
		} else {
			(void)memcpy(&lines->line[lines->length], &text[at], length);
			lines->length += length;
			at = end;
		}
		if ('\n' == text[at]) {
			_vasal_editor_theme_flush(lines);
			at++;
		}
	}
	if (0U < lines->length) {
		_vasal_editor_theme_flush(lines);
	}
	return lines->height;
}

float vasal_editor_theme_wrapped(const char* const text, const float x,
	const float y, const float width, const int size, const Color color) {
	_vasal_editor_theme_lines_s lines;
	(void)memset(&lines, 0, sizeof(lines));
	lines.x = x;
	lines.y = y;
	lines.width = width;
	lines.size = size;
	lines.color = color;
	lines.drawing = true;
	return _vasal_editor_theme_wrap(&lines, text);
}

float vasal_editor_theme_wrapped_height(const char* const text,
	const float width, const int size) {
	_vasal_editor_theme_lines_s lines;
	(void)memset(&lines, 0, sizeof(lines));
	lines.width = width;
	lines.size = size;
	return _vasal_editor_theme_wrap(&lines, text);
}

Color vasal_editor_theme_color(const uint32_t rgba) {
	const Color color = {
		(unsigned char)((rgba >> 24U) & 0xffU),
		(unsigned char)((rgba >> 16U) & 0xffU),
		(unsigned char)((rgba >> 8U) & 0xffU),
		(unsigned char)(rgba & 0xffU)
	};
	return color;
}

uint32_t vasal_editor_theme_rgba(const Color color) {
	return ((uint32_t)color.r << 24U) | ((uint32_t)color.g << 16U)
		| ((uint32_t)color.b << 8U) | (uint32_t)color.a;
}
