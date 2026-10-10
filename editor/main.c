
/// The map editor, a standalone page opened from disk. It starts with an
/// empty map, and runs the frame: files the page delivered, undo and redo
/// keys, the map view and the panels.

#include "editor/canvas.h"
#include "editor/document.h"
#include "editor/files.h"
#include "editor/panel.h"

#include <emscripten/emscripten.h>
#include <raygui.h>
#include <raylib.h>

static vasal_editor_state_s _vasal_editor_main_state;
static Rectangle _vasal_editor_main_area;
static bool_t _vasal_editor_main_fitted = false;

static const char* const _vasal_editor_main_open_problems[] = {
	"Map opened.",
	"That file is not a map file.",
	"That map file was made for another version.",
	"That map file is cut short.",
	"That map file is corrupted.",
	"That map file holds a value out of range."
};

static void _vasal_editor_main_message(const char* const text) {
	_vasal_editor_main_state.message = text;
	_vasal_editor_main_state.message_seconds = 3.0f;
}

// Opens the file the page delivered, if any.
// spec: editor.files#1
static void _vasal_editor_main_take_file(void) {
	size_t size = 0U;
	if (!vasal_editor_files_take(&size)) {
		return;
	}
	if (0U == size) {
		_vasal_editor_main_message("That file is too large to be a map.");
		return;
	}
	const uint32_t outcome =
		vasal_editor_document_open(vasal_editor_files_buffer(), size);
	_vasal_editor_main_message(_vasal_editor_main_open_problems[outcome]);
	if (0U == outcome) {
		_vasal_editor_main_state.selected = false;
		vasal_editor_canvas_fit(&_vasal_editor_main_state,
			_vasal_editor_main_area);
	}
}

// Ctrl+Z undoes, Ctrl+Y or Ctrl+Shift+Z redoes.
static void _vasal_editor_main_keys(void) {
	const bool_t control = IsKeyDown(KEY_LEFT_CONTROL)
		|| IsKeyDown(KEY_RIGHT_CONTROL);
	const bool_t shift = IsKeyDown(KEY_LEFT_SHIFT)
		|| IsKeyDown(KEY_RIGHT_SHIFT);
	if (control && IsKeyPressed(KEY_Z)) {
		if (shift) {
			(void)vasal_editor_document_redo();
		} else {
			(void)vasal_editor_document_undo();
		}
	}
	if (control && IsKeyPressed(KEY_Y)) {
		(void)vasal_editor_document_redo();
	}
}

// Runs and draws one frame. The browser calls it once per display refresh.
static void _vasal_editor_main_update(void) {
	vasal_editor_state_s* const state = &_vasal_editor_main_state;
	const float seconds = GetFrameTime();
	const float width = (float)GetScreenWidth();
	const float height = (float)GetScreenHeight();
	_vasal_editor_main_take_file();
	_vasal_editor_main_keys();
	if (!_vasal_editor_main_fitted && (0.0f < _vasal_editor_main_area.width)) {
		vasal_editor_canvas_fit(state, _vasal_editor_main_area);
		_vasal_editor_main_fitted = true;
	}

	BeginDrawing();
	ClearBackground((Color){ 32, 32, 32, 255 });
	vasal_editor_canvas_update(state, _vasal_editor_main_area, seconds);
	vasal_editor_canvas_draw(state, _vasal_editor_main_area);
	_vasal_editor_main_area = vasal_editor_panel_update(state, width, height);
	if ((NULL != state->message) && (0.0f < state->message_seconds)) {
		DrawText(state->message, (int)_vasal_editor_main_area.x + 16,
			(int)height - 36, 20, RAYWHITE);
		state->message_seconds -= seconds;
	}
	EndDrawing();
}

// spec: editor.platform
int main(void) {
	// A resizable window follows the browser window: raylib sizes the
	// canvas to it at start and whenever it changes.
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(1280, 720, "vasal editor");
	GuiSetStyle(DEFAULT, TEXT_SIZE, 16);
	(void)vasal_editor_document_create(512U, 512U);
	_vasal_editor_main_state.tool = vasal_editor_state_tool_paint;
	_vasal_editor_main_state.brush = 2;
	_vasal_editor_main_state.zoom = 1.0f;
	emscripten_set_main_loop(_vasal_editor_main_update, 0, true);
	return 0;
}
