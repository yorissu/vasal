
/// The editor, a standalone page opened from disk. It starts with an empty
/// map and the standard rules, or with the drafts the browser kept, and
/// runs the frame: what the page delivered, the keys, the page shown (the
/// map with its panels, the rules, or the server), the bars and dialogs,
/// the commands asked for, and keeping drafts.

#include "editor/bar.h"
#include "editor/canvas.h"
#include "editor/dialog.h"
#include "editor/document.h"
#include "editor/files.h"
#include "editor/form.h"
#include "editor/keys.h"
#include "editor/panel.h"
#include "editor/ruleset.h"
#include "editor/rules_view.h"
#include "editor/server_view.h"
#include "editor/theme.h"

#include <emscripten/emscripten.h>
#include <raygui.h>
#include <raylib.h>

#include <string.h>

// How often drafts are kept when something changed, in seconds.
#define _vasal_editor_main_keep_every 2.0f

static vasal_editor_state_s _vasal_editor_main_state;
static Rectangle _vasal_editor_main_area;
static bool_t _vasal_editor_main_fitted = false;
static uint32_t _vasal_editor_main_kept_map = 0U;
static uint32_t _vasal_editor_main_kept_rules = 0U;
static float _vasal_editor_main_keep_timer = 0.0f;

// What reading a map or rules file can come to, in the order of their
// outcomes.
static const char* const _vasal_editor_main_problems[] = {
	"Opened.",
	"That is not a map or rules file.",
	"That file was made for another version of the game.",
	"That file is cut short.",
	"That file is corrupted.",
	"That file holds a value out of range."
};

// A map file arrived: open it.
// spec: editor.files#1 editor.files#2
static void _vasal_editor_main_open_map(vasal_editor_state_s* const state,
		const vasal_editor_files_arrival_s* const arrival) {
	const uint32_t outcome = vasal_editor_document_open(arrival->data,
		arrival->size);
	if (0U != outcome) {
		vasal_editor_state_say(state, _vasal_editor_main_problems[outcome],
			true);
		return;
	}
	state->map_saved = vasal_editor_document_revision();
	_vasal_editor_main_kept_map = state->map_saved;
	state->selected = false;
	state->painted = false;
	state->terrain = 0U;
	vasal_editor_canvas_fit(state, _vasal_editor_main_area);
	if (vasal_editor_files_origin_draft == arrival->origin) {
		vasal_editor_state_say(state, "Your map is back as you left it.",
			false);
	} else if (vasal_editor_files_origin_server == arrival->origin) {
		vasal_editor_state_say(state, "The server's map is open.", false);
	} else {
		vasal_editor_state_say(state, "Map opened.", false);
		state->tab = vasal_editor_state_tab_map;
	}
}

// A rules file arrived: open it.
// spec: editor.rules#2 editor.files#2
static void _vasal_editor_main_open_rules(vasal_editor_state_s* const state,
		const vasal_editor_files_arrival_s* const arrival) {
	const uint32_t outcome = vasal_editor_ruleset_open(arrival->data,
		arrival->size);
	if (0U != outcome) {
		vasal_editor_state_say(state, _vasal_editor_main_problems[outcome],
			true);
		return;
	}
	state->rules_saved = vasal_editor_ruleset_revision();
	_vasal_editor_main_kept_rules = state->rules_saved;
	state->unit = 0U;
	if (vasal_editor_files_origin_server == arrival->origin) {
		vasal_editor_state_say(state, "The server's rules are open.", false);
	} else if (vasal_editor_files_origin_user == arrival->origin) {
		vasal_editor_state_say(state, "Rules opened.", false);
		state->tab = vasal_editor_state_tab_rules;
	} else {
		// A draft comes back quietly; the map's message tells of both.
	}
}

// What the server answered.
// spec: editor.rules#5
static void _vasal_editor_main_answer(vasal_editor_state_s* const state,
		const vasal_editor_files_answer_s* const answer) {
	static const char* const what[] = {
		"map", "rules", "map", "rules"
	};
	const char* const name = what[answer->request];
	const bool_t upload = (vasal_editor_files_request_upload_map
		== answer->request) || (vasal_editor_files_request_upload_rules
		== answer->request);
	const char* text = TextFormat("The server answered %u: %s",
		(unsigned int)answer->status, answer->text);
	bool_t good = false;
	if (0U == answer->status) {
		text = TextFormat("The server at %s can't be reached.",
			state->address);
	} else if (200U == answer->status) {
		good = true;
		text = upload ? TextFormat("The server took the %s. Players get it "
			"when they join or reload.", name)
			: TextFormat("Got the server's %s.", name);
	} else if (403U == answer->status) {
		text = "The server refused the admin key, or has uploads off.";
	} else if (404U == answer->status) {
		text = TextFormat("The server has no %s file; it plays its own.",
			name);
	} else if (400U == answer->status) {
		text = TextFormat("The server refused the %s: %s", name,
			answer->text);
	} else {
		// Another status: the server's own words.
	}
	vasal_editor_server_view_log(text, good);
	vasal_editor_state_say(state, text, !good);
}

// Takes what the page delivered: a file, and an answer.
static void _vasal_editor_main_take(vasal_editor_state_s* const state) {
	vasal_editor_files_arrival_s arrival;
	vasal_editor_files_answer_s answer;
	bool_t answered = false;
	const bool_t arrived = vasal_editor_files_take(&arrival, &answered,
		&answer);
	if (answered) {
		_vasal_editor_main_answer(state, &answer);
	}
	if (!arrived) {
		return;
	}
	if (0U == arrival.size) {
		vasal_editor_state_say(state, "That file is too large to open.",
			true);
		return;
	}
	const vasal_editor_files_kind_e kind = vasal_editor_files_kind(
		arrival.data, arrival.size);
	if (vasal_editor_files_kind_map == kind) {
		_vasal_editor_main_open_map(state, &arrival);
	} else if (vasal_editor_files_kind_rules == kind) {
		_vasal_editor_main_open_rules(state, &arrival);
	} else {
		vasal_editor_state_say(state, "That is not a map or rules file.",
			true);
	}
}

// Saves the map, saying how it went.
// spec: editor.files editor.validation
static void _vasal_editor_main_save_map(vasal_editor_state_s* const state) {
	const vasal_engine_map_s* const map = vasal_editor_document_map();
	if (!vasal_editor_files_save_map(map)) {
		vasal_editor_state_say(state, "The map could not be saved.", true);
		return;
	}
	state->map_saved = vasal_editor_document_revision();
	vasal_engine_map_finding_s findings[1];
	const size_t problems = vasal_engine_map_check(map, findings, 1U);
	if (0U < problems) {
		vasal_editor_state_say(state, TextFormat("Map saved as a draft: it "
			"has %u problems.", (unsigned int)problems), true);
	} else {
		vasal_editor_state_say(state, "Map saved.", false);
	}
}

static void _vasal_editor_main_save_rules(vasal_editor_state_s* const state) {
	if (!vasal_editor_files_save_rules(vasal_editor_ruleset_rules())) {
		vasal_editor_state_say(state, "The rules could not be saved.", true);
		return;
	}
	state->rules_saved = vasal_editor_ruleset_revision();
	vasal_editor_state_say(state, "Rules saved.", false);
}

// Uploads the map or the rules, or both.
// spec: editor.rules#5
static void _vasal_editor_main_upload(vasal_editor_state_s* const state,
		const bool_t map, const bool_t rules) {
	if (('\0' == state->address[0]) || ('\0' == state->key[0])) {
		state->tab = vasal_editor_state_tab_server;
		vasal_editor_state_say(state, "Give the server's address and admin "
			"key first.", true);
		return;
	}
	bool_t written = true;
	if (rules) {
		written = vasal_editor_files_upload_rules(vasal_editor_ruleset_rules(),
			state->address, state->key);
	}
	if (map) {
		written = vasal_editor_files_upload_map(vasal_editor_document_map(),
			state->address, state->key) && written;
	}
	vasal_editor_state_say(state, written ? "Uploading..."
		: "That could not be written into a file.", !written);
}

// Undoes or redoes on the page shown.
// spec: editor.tools#3 editor.rules#3
static void _vasal_editor_main_history(vasal_editor_state_s* const state,
		const bool_t undoing) {
	bool_t done = false;
	if (vasal_editor_state_tab_rules == state->tab) {
		done = undoing ? vasal_editor_ruleset_undo()
			: vasal_editor_ruleset_redo();
	} else {
		done = undoing ? vasal_editor_document_undo()
			: vasal_editor_document_redo();
		state->painted = false;
	}
	if (!done) {
		vasal_editor_state_say(state, undoing ? "Nothing to undo."
			: "Nothing to redo.", false);
	}
}

// Does what waited on the discard dialog.
static void _vasal_editor_main_carry(vasal_editor_state_s* const state) {
	if (vasal_editor_state_pending_new_rules == state->pending) {
		vasal_editor_ruleset_create();
		state->rules_saved = vasal_editor_ruleset_revision();
		state->unit = 0U;
		vasal_editor_state_say(state, "Standard rules.", false);
	} else if (vasal_editor_state_pending_open == state->pending) {
		vasal_editor_files_choose();
	} else {
		// Nothing waits.
	}
	state->pending = vasal_editor_state_pending_none;
}

// Asks before throwing away unsaved changes, or goes ahead at once.
static void _vasal_editor_main_ask(vasal_editor_state_s* const state,
		const vasal_editor_state_pending_e pending, const bool_t unsaved) {
	state->pending = pending;
	if (unsaved) {
		state->dialog = vasal_editor_state_dialog_discard;
	} else {
		_vasal_editor_main_carry(state);
	}
}

// Does the files' commands.
static void _vasal_editor_main_file_command(vasal_editor_state_s* const state,
		const vasal_editor_state_command_e command) {
	if (vasal_editor_state_command_new_map == command) {
		state->dialog = vasal_editor_state_dialog_new_map;
	} else if (vasal_editor_state_command_new_rules == command) {
		_vasal_editor_main_ask(state, vasal_editor_state_pending_new_rules,
			vasal_editor_state_rules_unsaved(state));
	} else if (vasal_editor_state_command_open == command) {
		_vasal_editor_main_ask(state, vasal_editor_state_pending_open,
			vasal_editor_state_map_unsaved(state)
			|| vasal_editor_state_rules_unsaved(state));
	} else if (vasal_editor_state_command_save == command) {
		if (vasal_editor_state_tab_rules == state->tab) {
			_vasal_editor_main_save_rules(state);
		} else {
			_vasal_editor_main_save_map(state);
		}
	} else if (vasal_editor_state_command_save_map == command) {
		_vasal_editor_main_save_map(state);
	} else if (vasal_editor_state_command_save_rules == command) {
		_vasal_editor_main_save_rules(state);
	} else {
		// Not a file command.
	}
}

// Does the command asked for this frame.
static void _vasal_editor_main_command(vasal_editor_state_s* const state) {
	const vasal_editor_state_command_e command = state->command;
	state->command = vasal_editor_state_command_none;
	_vasal_editor_main_file_command(state, command);
	if (vasal_editor_state_command_upload_map == command) {
		_vasal_editor_main_upload(state, true, false);
	} else if (vasal_editor_state_command_upload_rules == command) {
		_vasal_editor_main_upload(state, false, true);
	} else if (vasal_editor_state_command_upload_both == command) {
		_vasal_editor_main_upload(state, true, true);
	} else if (vasal_editor_state_command_fetch_map == command) {
		vasal_editor_files_fetch(vasal_editor_files_kind_map, state->address);
	} else if (vasal_editor_state_command_fetch_rules == command) {
		vasal_editor_files_fetch(vasal_editor_files_kind_rules,
			state->address);
	} else if (vasal_editor_state_command_undo == command) {
		_vasal_editor_main_history(state, true);
	} else if (vasal_editor_state_command_redo == command) {
		_vasal_editor_main_history(state, false);
	} else if (vasal_editor_state_command_fit == command) {
		vasal_editor_canvas_fit(state, _vasal_editor_main_area);
	} else {
		// A file command, or none.
	}
}

// Keeps drafts of what changed, every few seconds.
// spec: editor.files#3
static void _vasal_editor_main_keep(const float seconds) {
	_vasal_editor_main_keep_timer += seconds;
	if ((_vasal_editor_main_keep_every > _vasal_editor_main_keep_timer)
			|| _vasal_editor_main_state.dragging) {
		return;
	}
	_vasal_editor_main_keep_timer = 0.0f;
	const uint32_t map = vasal_editor_document_revision();
	if (map != _vasal_editor_main_kept_map) {
		vasal_editor_files_keep_map(vasal_editor_document_map());
		_vasal_editor_main_kept_map = map;
	}
	const uint32_t rules = vasal_editor_ruleset_revision();
	if (rules != _vasal_editor_main_kept_rules) {
		vasal_editor_files_keep_rules(vasal_editor_ruleset_rules());
		_vasal_editor_main_kept_rules = rules;
	}
}

// Draws the page shown into the space between the bars, and acts on it.
static void _vasal_editor_main_page(vasal_editor_state_s* const state,
		const Rectangle space, const float seconds, const bool_t input) {
	const Rectangle left = {
		space.x, space.y, vasal_editor_panel_width, space.height
	};
	if (vasal_editor_state_tab_map == state->tab) {
		const Rectangle right = {
			(space.x + space.width) - vasal_editor_panel_width, space.y,
			vasal_editor_panel_width, space.height
		};
		_vasal_editor_main_area = (Rectangle){
			left.x + left.width, space.y,
			space.width - left.width - right.width, space.height
		};
		vasal_editor_canvas_update(state, _vasal_editor_main_area, seconds,
			input);
		vasal_editor_canvas_draw(state, _vasal_editor_main_area);
		vasal_editor_panel_left(state, left);
		vasal_editor_panel_right(state, right);
	} else if (vasal_editor_state_tab_rules == state->tab) {
		const Rectangle page = {
			left.x + left.width, space.y, space.width - left.width,
			space.height
		};
		vasal_editor_rules_view_update(state, left, page);
	} else {
		vasal_editor_server_view_update(state, space);
	}
}

// Runs and draws one frame. The browser calls it once per display refresh.
static void _vasal_editor_main_update(void) {
	vasal_editor_state_s* const state = &_vasal_editor_main_state;
	const float seconds = GetFrameTime();
	const float width = (float)GetScreenWidth();
	const float height = (float)GetScreenHeight();
	const bool_t dialog = vasal_editor_state_dialog_none != state->dialog;
	vasal_editor_form_block(dialog);
	vasal_editor_form_frame();
	_vasal_editor_main_take(state);
	vasal_editor_keys_update(state, seconds);
	if (!_vasal_editor_main_fitted && (0.0f < _vasal_editor_main_area.width)) {
		vasal_editor_canvas_fit(state, _vasal_editor_main_area);
		_vasal_editor_main_fitted = true;
	}
	const Rectangle space = {
		0.0f, vasal_editor_bar_top, width,
		height - vasal_editor_bar_top - vasal_editor_bar_bottom
	};

	BeginDrawing();
	ClearBackground(vasal_editor_theme_background);
	_vasal_editor_main_page(state, space, seconds, !dialog
		&& !vasal_editor_form_dropped());
	vasal_editor_bar_top_update(state, width);
	vasal_editor_bar_status(state, width, height);
	vasal_editor_bar_message(state, space, seconds);
	if (dialog) {
		vasal_editor_form_block(false);
		if (vasal_editor_dialog_update(state, width, height)) {
			_vasal_editor_main_carry(state);
		}
	}
	vasal_editor_form_overlay();
	EndDrawing();

	_vasal_editor_main_command(state);
	_vasal_editor_main_keep(seconds);
}

// spec: editor.platform
int main(void) {
	// A resizable window follows the browser window: raylib sizes the
	// canvas to it at start and whenever it changes.
	SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
	InitWindow(1440, 860, "vasal editor");
	SetExitKey(KEY_NULL);
	vasal_editor_theme_load();
	(void)vasal_editor_document_create(512U, 512U);
	static const char name[] = "new map";
	(void)memcpy(vasal_editor_document_map()->name, name, sizeof(name));
	vasal_editor_ruleset_create();
	vasal_editor_state_s* const state = &_vasal_editor_main_state;
	(void)memset(state, 0, sizeof(*state));
	state->tab = vasal_editor_state_tab_map;
	state->tool = vasal_editor_state_tool_paint;
	state->terrain = (uint8_t)vasal_engine_map_standard_forest;
	state->brush.radius = 3;
	state->zoom = 1.0f;
	state->minimap = true;
	state->map_saved = vasal_editor_document_revision();
	state->rules_saved = vasal_editor_ruleset_revision();
	_vasal_editor_main_kept_map = state->map_saved;
	_vasal_editor_main_kept_rules = state->rules_saved;
	static const char server[] = "localhost:8080";
	(void)memcpy(state->address, server, sizeof(server));
	char address[vasal_editor_state_address_capacity];
	state->remember_key = vasal_editor_files_recall(address, sizeof(address),
		state->key, sizeof(state->key));
	if ('\0' != address[0]) {
		(void)memcpy(state->address, address, sizeof(address));
	}
	vasal_editor_files_restore();
	vasal_editor_state_say(state, "Press F1 for every key.", false);
	emscripten_set_main_loop(_vasal_editor_main_update, 0, true);
	return 0;
}
