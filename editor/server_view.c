
/// The key shows as stars unless it is being typed or shown. The address
/// and, when asked, the key are remembered in the browser as they change.

#include "editor/server_view.h"
#include "editor/document.h"
#include "editor/files.h"
#include "editor/form.h"
#include "editor/theme.h"

#include <raygui.h>

#include <math.h>
#include <string.h>

#define _vasal_editor_server_view_lines 10U
#define _vasal_editor_server_view_line_capacity 160U
#define _vasal_editor_server_view_address_id 0x300001U
#define _vasal_editor_server_view_key_id 0x300002U

// The log, newest first.
static char _vasal_editor_server_view_log_lines[_vasal_editor_server_view_lines]
	[_vasal_editor_server_view_line_capacity];
static bool_t _vasal_editor_server_view_log_good
	[_vasal_editor_server_view_lines];
static double _vasal_editor_server_view_log_times
	[_vasal_editor_server_view_lines];
static size_t _vasal_editor_server_view_log_count = 0U;
static float _vasal_editor_server_view_scroll = 0.0f;
static bool_t _vasal_editor_server_view_showing = false;

void vasal_editor_server_view_log(const char* const text, const bool_t good) {
	for (size_t i = _vasal_editor_server_view_lines - 1U; 0U < i; i--) {
		(void)memcpy(_vasal_editor_server_view_log_lines[i],
			_vasal_editor_server_view_log_lines[i - 1U],
			_vasal_editor_server_view_line_capacity);
		_vasal_editor_server_view_log_good[i] =
			_vasal_editor_server_view_log_good[i - 1U];
		_vasal_editor_server_view_log_times[i] =
			_vasal_editor_server_view_log_times[i - 1U];
	}
	const size_t length = strlen(text);
	const size_t kept = (length < _vasal_editor_server_view_line_capacity)
		? length : (_vasal_editor_server_view_line_capacity - 1U);
	(void)memcpy(_vasal_editor_server_view_log_lines[0], text, kept);
	_vasal_editor_server_view_log_lines[0][kept] = '\0';
	_vasal_editor_server_view_log_good[0] = good;
	_vasal_editor_server_view_log_times[0] = GetTime();
	if (_vasal_editor_server_view_lines > _vasal_editor_server_view_log_count) {
		_vasal_editor_server_view_log_count++;
	}
}

// The address and key.
// spec: editor.rules#5
static void _vasal_editor_server_view_where(vasal_editor_state_s* const state,
		vasal_editor_form_s* const form) {
	bool_t changed = vasal_editor_form_text(form, "Address", state->address,
		sizeof(state->address), _vasal_editor_server_view_address_id);
	const Rectangle row = vasal_editor_form_take(form, vasal_editor_form_row);
	vasal_editor_theme_text("Admin key", row.x, row.y + 6.0f,
		vasal_editor_theme_normal, false, vasal_editor_theme_foreground);
	const Rectangle box = {
		row.x + (row.width * 0.46f), row.y, (row.width * 0.54f) - 70.0f,
		row.height
	};
	if (_vasal_editor_server_view_showing) {
		changed = vasal_editor_form_text_box(box, state->key,
			sizeof(state->key), _vasal_editor_server_view_key_id) || changed;
	} else {
		char stars[vasal_editor_state_key_capacity];
		const size_t length = strlen(state->key);
		(void)memset(stars, '*', length);
		stars[length] = '\0';
		// Clicking the stars shows the key, to type it.
		if (0 != GuiTextBox(box, stars, (int)sizeof(stars), false)) {
			_vasal_editor_server_view_showing = true;
		}
	}
	bool showing = _vasal_editor_server_view_showing;
	(void)GuiToggle((Rectangle){ (box.x + box.width) + 6.0f, row.y, 64.0f,
		row.height }, showing ? "Hide" : "Show", &showing);
	_vasal_editor_server_view_showing = showing;
	bool_t remember = state->remember_key;
	if (vasal_editor_form_check(form, "Remember the key", "On this computer, "
			"in the browser's storage.", &remember)) {
		state->remember_key = remember;
		changed = true;
	}
	if (changed) {
		vasal_editor_files_remember(state->address, state->remember_key
			? state->key : NULL);
	}
}

// Uploading and fetching.
// spec: editor.rules#5 editor.rules#7
static void _vasal_editor_server_view_actions(
		vasal_editor_state_s* const state, vasal_editor_form_s* const form) {
	const bool_t ready = ('\0' != state->address[0]) && ('\0' != state->key[0]);
	if (!ready) {
		GuiDisable();
	}
	const Rectangle row = vasal_editor_form_take(form, 36.0f);
	if (0 != GuiButton(vasal_editor_form_part(row, 3U, 0U),
			GuiIconText(ICON_FILE_EXPORT, "Upload map"))) {
		state->command = vasal_editor_state_command_upload_map;
	}
	if (0 != GuiButton(vasal_editor_form_part(row, 3U, 1U),
			GuiIconText(ICON_FILE_EXPORT, "Upload rules"))) {
		state->command = vasal_editor_state_command_upload_rules;
	}
	if (0 != GuiButton(vasal_editor_form_part(row, 3U, 2U),
			GuiIconText(ICON_FILE_EXPORT, "Upload both"))) {
		state->command = vasal_editor_state_command_upload_both;
	}
	GuiEnable();
	if (!ready) {
		vasal_editor_form_paragraph(form, "Give the server's address and its "
			"admin key to upload.");
	}
	if ('\0' == state->address[0]) {
		GuiDisable();
	}
	const Rectangle fetch = vasal_editor_form_take(form,
		vasal_editor_form_row);
	if (0 != GuiButton(vasal_editor_form_part(fetch, 2U, 0U),
			GuiIconText(ICON_FILE_OPEN, "Get the server's map"))) {
		state->command = vasal_editor_state_command_fetch_map;
	}
	if (0 != GuiButton(vasal_editor_form_part(fetch, 2U, 1U),
			GuiIconText(ICON_FILE_OPEN, "Get the server's rules"))) {
		state->command = vasal_editor_state_command_fetch_rules;
	}
	GuiEnable();
}

// The log of answers.
static void _vasal_editor_server_view_answers(vasal_editor_form_s* const form) {
	vasal_editor_form_heading(form, "What the server said");
	if (0U == _vasal_editor_server_view_log_count) {
		vasal_editor_form_paragraph(form, "Nothing yet.");
	}
	const double now = GetTime();
	for (size_t i = 0U; i < _vasal_editor_server_view_log_count; i++) {
		const char* const text = _vasal_editor_server_view_log_lines[i];
		const float height = vasal_editor_theme_wrapped_height(text,
			form->width - 110.0f, vasal_editor_theme_normal) + 8.0f;
		const Rectangle row = vasal_editor_form_take(form, height);
		DrawRectangleRounded(row, 0.1f, 6, vasal_editor_theme_panel);
		const bool_t good = _vasal_editor_server_view_log_good[i];
		GuiDrawIcon(good ? ICON_OK_TICK : ICON_CROSS, (int)row.x + 6,
			(int)row.y + 4, 1, good ? vasal_editor_theme_good
			: vasal_editor_theme_bad);
		(void)vasal_editor_theme_wrapped(text, row.x + 30.0f, row.y + 5.0f,
			row.width - 110.0f, vasal_editor_theme_normal,
			vasal_editor_theme_foreground);
		const double passed = now - _vasal_editor_server_view_log_times[i];
		const int seconds = (int)passed;
		const char* const age = (60 > seconds) ? TextFormat("%d s ago",
			seconds) : TextFormat("%d min ago", seconds / 60);
		vasal_editor_theme_text(age, (row.x + row.width) - 8.0f
			- vasal_editor_theme_measure(age, vasal_editor_theme_small,
				false), row.y + 7.0f, vasal_editor_theme_small, false,
			vasal_editor_theme_dim);
	}
}

// spec: editor.rules#5
void vasal_editor_server_view_update(vasal_editor_state_s* const state,
	const Rectangle area) {
	DrawRectangleRec(area, vasal_editor_theme_background);
	const float width = fminf(area.width, 720.0f);
	vasal_editor_form_s form;
	vasal_editor_form_begin(&form, area, &_vasal_editor_server_view_scroll,
		24.0f);
	form.x = area.x + ((area.width - width) / 2.0f) + 24.0f;
	form.width = width - 54.0f;
	vasal_editor_form_heading(&form, "Server");
	vasal_editor_form_paragraph(&form, "Upload the map and the rules to the "
		"game's server. It checks them and keeps them; every player who "
		"joins or reloads from then on plays them. Start the server with "
		"--admin-key and a key of letters and digits, and with --map and "
		"--rules naming the files it keeps them in.");
	vasal_editor_form_space(&form, 6.0f);
	_vasal_editor_server_view_where(state, &form);
	vasal_editor_form_space(&form, 6.0f);
	vasal_engine_map_finding_s findings[1];
	const size_t problems = vasal_engine_map_check(
		vasal_editor_document_map(), findings, 1U);
	if (0U < problems) {
		vasal_editor_form_paragraph(&form, TextFormat("The map has %u "
			"problems (see the map page); fix them before players play it.",
			(unsigned int)problems));
	}
	_vasal_editor_server_view_actions(state, &form);
	_vasal_editor_server_view_answers(&form);
	vasal_editor_form_end(&form);
}
