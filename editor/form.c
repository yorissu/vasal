
/// raygui takes input in the call that draws a control, so a control that
/// must not take input is drawn while raygui is locked: outside the view,
/// while a dialog blocks the forms, or while a dropdown's list is open.

#include "editor/form.h"
#include "editor/theme.h"

#include <raygui.h>

#include <math.h>
#include <string.h>

// The share of a property row the label takes.
#define _vasal_editor_form_label_share 0.46f

#define _vasal_editor_form_items_capacity 1024U

static bool_t _vasal_editor_form_blocked = false;
// The box being edited, and whether it was drawn this frame.
static uint32_t _vasal_editor_form_editing = 0U;
static bool_t _vasal_editor_form_seen = false;
// The open dropdown: its id, where it is, its items and value, and whether
// it was drawn this frame; and a choice made in it for the next frame.
static uint32_t _vasal_editor_form_dropped_id = 0U;
static Rectangle _vasal_editor_form_dropped_bounds;
static char _vasal_editor_form_dropped_items
	[_vasal_editor_form_items_capacity];
static int _vasal_editor_form_dropped_value = 0;
static bool_t _vasal_editor_form_dropped_seen = false;
static bool_t _vasal_editor_form_dropped_fresh = false;
static uint32_t _vasal_editor_form_chosen_id = 0U;
static int _vasal_editor_form_chosen_value = 0;

// Locks raygui when controls must not take input, also when extra says so.
static void _vasal_editor_form_lock(const bool_t extra) {
	if (_vasal_editor_form_blocked || (0U != _vasal_editor_form_dropped_id)
			|| extra) {
		GuiLock();
	} else {
		GuiUnlock();
	}
}

void vasal_editor_form_frame(void) {
	_vasal_editor_form_seen = false;
	_vasal_editor_form_dropped_seen = false;
	_vasal_editor_form_lock(false);
}

void vasal_editor_form_block(const bool_t blocked) {
	_vasal_editor_form_blocked = blocked;
	_vasal_editor_form_lock(false);
}

bool_t vasal_editor_form_blocked(void) {
	return _vasal_editor_form_blocked;
}

bool_t vasal_editor_form_typing(void) {
	return 0U != _vasal_editor_form_editing;
}

bool_t vasal_editor_form_dropped(void) {
	return 0U != _vasal_editor_form_dropped_id;
}

void vasal_editor_form_release(void) {
	_vasal_editor_form_editing = 0U;
}

void vasal_editor_form_begin(vasal_editor_form_s* const form,
	const Rectangle view, float* const scroll, const float padding) {
	form->view = view;
	form->scroll = scroll;
	form->x = view.x + padding;
	form->width = view.width - (2.0f * padding) - 6.0f;
	form->top = view.y + padding - *scroll;
	form->y = form->top;
	form->help = true;
	BeginScissorMode((int)view.x, (int)view.y, (int)view.width,
		(int)view.height);
}

void vasal_editor_form_end(vasal_editor_form_s* const form) {
	EndScissorMode();
	_vasal_editor_form_lock(false);
	const float content = (form->y - form->top) + 24.0f;
	const float most = fmaxf(0.0f, content - form->view.height);
	const bool_t over = CheckCollisionPointRec(GetMousePosition(),
		form->view);
	if (over && !_vasal_editor_form_blocked
			&& (0U == _vasal_editor_form_dropped_id)) {
		*form->scroll -= GetMouseWheelMove() * 48.0f;
	}
	*form->scroll = fminf(fmaxf(*form->scroll, 0.0f), most);
	if (0.0f < most) {
		const float share = form->view.height / content;
		const float length = fmaxf(24.0f, form->view.height * share);
		const float position = (*form->scroll / most)
			* (form->view.height - length);
		DrawRectangleRounded((Rectangle){
			(form->view.x + form->view.width) - 5.0f,
			form->view.y + position, 3.0f, length }, 1.0f, 4,
			vasal_editor_theme_border);
	}
}

Rectangle vasal_editor_form_take(vasal_editor_form_s* const form,
	const float height) {
	const Rectangle bounds = { form->x, form->y, form->width, height };
	form->y += height + vasal_editor_form_gap;
	const bool_t inside = (bounds.y >= form->view.y)
		&& ((bounds.y + bounds.height)
			<= (form->view.y + form->view.height));
	_vasal_editor_form_lock(!inside);
	return bounds;
}

Rectangle vasal_editor_form_part(const Rectangle row, const size_t count,
	const size_t index) {
	const float gaps = vasal_editor_form_gap * ((float)count - 1.0f);
	const float width = (row.width - gaps) / (float)count;
	const Rectangle part = {
		row.x + ((width + vasal_editor_form_gap) * (float)index), row.y,
		width, row.height
	};
	return part;
}

void vasal_editor_form_space(vasal_editor_form_s* const form,
	const float height) {
	form->y += height;
}

void vasal_editor_form_heading(vasal_editor_form_s* const form,
	const char* const text) {
	form->y += 6.0f;
	const Rectangle row = vasal_editor_form_take(form, 22.0f);
	vasal_editor_theme_text(text, row.x, row.y + 2.0f,
		vasal_editor_theme_normal, true, vasal_editor_theme_foreground);
	DrawRectangleRec((Rectangle){ row.x, row.y + row.height, row.width,
		1.0f }, vasal_editor_theme_border);
	form->y += 4.0f;
}

void vasal_editor_form_label(vasal_editor_form_s* const form,
	const char* const text, const bool_t dim) {
	const Rectangle row = vasal_editor_form_take(form, 18.0f);
	vasal_editor_theme_text(text, row.x, row.y, vasal_editor_theme_normal,
		false, dim ? vasal_editor_theme_dim : vasal_editor_theme_foreground);
}

void vasal_editor_form_paragraph(vasal_editor_form_s* const form,
	const char* const text) {
	const float height = vasal_editor_theme_wrapped_height(text, form->width,
		vasal_editor_theme_small);
	const Rectangle row = vasal_editor_form_take(form, height);
	(void)vasal_editor_theme_wrapped(text, row.x, row.y, row.width,
		vasal_editor_theme_small, vasal_editor_theme_dim);
}

bool_t vasal_editor_form_button(vasal_editor_form_s* const form,
	const char* const text) {
	return 0 != GuiButton(vasal_editor_form_take(form, vasal_editor_form_row),
		text);
}

// Takes a property row: draws the label and returns where the control
// goes. The help text goes under it.
static Rectangle _vasal_editor_form_property(vasal_editor_form_s* const form,
		const char* const label, const char* const help) {
	const Rectangle row = vasal_editor_form_take(form, vasal_editor_form_row);
	const float label_width = row.width * _vasal_editor_form_label_share;
	vasal_editor_theme_text(label, row.x, row.y + 6.0f,
		vasal_editor_theme_normal, false, vasal_editor_theme_foreground);
	if (form->help && (NULL != help)) {
		const float height = vasal_editor_theme_wrapped_height(help,
			form->width, vasal_editor_theme_small);
		(void)vasal_editor_theme_wrapped(help, row.x, form->y - 4.0f,
			form->width, vasal_editor_theme_small, vasal_editor_theme_dim);
		form->y += height;
	}
	const Rectangle control = {
		row.x + label_width, row.y, row.width - label_width, row.height
	};
	return control;
}

// Marks a box as drawn this frame when it is the one being edited.
static bool_t _vasal_editor_form_edited(const uint32_t id) {
	const bool_t editing = id == _vasal_editor_form_editing;
	if (editing) {
		_vasal_editor_form_seen = true;
	}
	return editing;
}

// raygui says a box was clicked into or out of: switch editing.
static void _vasal_editor_form_toggle(const uint32_t id) {
	_vasal_editor_form_editing = (id == _vasal_editor_form_editing) ? 0U : id;
	_vasal_editor_form_seen = true;
}

bool_t vasal_editor_form_spinner(const Rectangle bounds, int* const value,
	const int minimum, const int maximum, const uint32_t id) {
	const int before = *value;
	if (0 != GuiSpinner(bounds, NULL, value, minimum, maximum,
			_vasal_editor_form_edited(id))) {
		_vasal_editor_form_toggle(id);
	}
	if (*value < minimum) {
		*value = minimum;
	}
	if (*value > maximum) {
		*value = maximum;
	}
	return before != *value;
}

bool_t vasal_editor_form_number(vasal_editor_form_s* const form,
	const char* const label, const char* const help, int* const value,
	const int minimum, const int maximum, const uint32_t id) {
	const Rectangle control = _vasal_editor_form_property(form, label, help);
	return vasal_editor_form_spinner(control, value, minimum, maximum, id);
}

bool_t vasal_editor_form_slider(vasal_editor_form_s* const form,
	const char* const label, const char* const help, int* const value,
	const int minimum, const int maximum, const char* const suffix) {
	const Rectangle control = _vasal_editor_form_property(form, label, help);
	const float number_width = 64.0f;
	float position = (float)*value;
	(void)GuiSliderBar((Rectangle){ control.x, control.y + 9.0f,
		control.width - number_width - 8.0f, control.height - 18.0f }, NULL,
		NULL, &position, (float)minimum, (float)maximum);
	vasal_editor_theme_text(TextFormat("%d%s", (int)lroundf(position),
		suffix), (control.x + control.width) - number_width, control.y + 6.0f,
		vasal_editor_theme_normal, false, vasal_editor_theme_foreground);
	const int after = (int)lroundf(position);
	const bool_t changed = after != *value;
	*value = after;
	return changed;
}

bool_t vasal_editor_form_text_box(const Rectangle bounds, char* const text,
	const size_t capacity, const uint32_t id) {
	char edited[256];
	const size_t size = (capacity < sizeof(edited)) ? capacity
		: sizeof(edited);
	(void)memcpy(edited, text, size);
	edited[size - 1U] = '\0';
	if (0 != GuiTextBox(bounds, edited, (int)size,
			_vasal_editor_form_edited(id))) {
		_vasal_editor_form_toggle(id);
	}
	// Names hold printable ASCII only, like the files.
	for (size_t i = 0U; '\0' != edited[i]; i++) {
		if ((edited[i] < ' ') || (edited[i] > '~')) {
			edited[i] = '_';
		}
	}
	if (0 == strncmp(edited, text, size)) {
		return false;
	}
	(void)memset(text, 0, capacity);
	for (size_t i = 0U; ('\0' != edited[i]) && ((i + 1U) < capacity); i++) {
		text[i] = edited[i];
	}
	return true;
}

bool_t vasal_editor_form_text(vasal_editor_form_s* const form,
	const char* const label, char* const text, const size_t capacity,
	const uint32_t id) {
	const Rectangle control = _vasal_editor_form_property(form, label, NULL);
	return vasal_editor_form_text_box(control, text, capacity, id);
}

bool_t vasal_editor_form_check(vasal_editor_form_s* const form,
	const char* const label, const char* const help, bool_t* const value) {
	const Rectangle control = _vasal_editor_form_property(form, label, help);
	// A switch: a track, filled when on, with a knob at its end.
	const Rectangle track = { control.x, control.y + 5.0f, 38.0f, 20.0f };
	static const int disabled_state = STATE_DISABLED;
	const bool_t disabled = disabled_state == GuiGetState();
	const bool_t enabled = !GuiIsLocked() && !disabled;
	const bool_t hovered = enabled
		&& CheckCollisionPointRec(GetMousePosition(), track);
	const bool_t flipped = hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
	if (flipped) {
		*value = !*value;
	}
	Color fill = *value ? vasal_editor_theme_accent : vasal_editor_theme_raised;
	if (disabled) {
		fill = vasal_editor_theme_panel;
	}
	DrawRectangleRounded(track, 1.0f, 8, fill);
	DrawRectangleRoundedLinesEx(track, 1.0f, 8, 1.0f, hovered
		? vasal_editor_theme_accent : vasal_editor_theme_border);
	const float knob = *value ? (track.x + track.width - 10.0f)
		: (track.x + 10.0f);
	DrawCircleV((Vector2){ knob, track.y + 10.0f }, 7.0f,
		vasal_editor_theme_foreground);
	return flipped;
}

bool_t vasal_editor_form_dropdown(const Rectangle bounds,
	const char* const items, int* const active, const uint32_t id) {
	bool_t changed = false;
	if (id == _vasal_editor_form_chosen_id) {
		changed = *active != _vasal_editor_form_chosen_value;
		*active = _vasal_editor_form_chosen_value;
		_vasal_editor_form_chosen_id = 0U;
	}
	int shown = *active;
	if ((id != _vasal_editor_form_dropped_id)
			&& (0 != GuiDropdownBox(bounds, items, &shown, false))) {
		// It opens; its list shows from the next frame, as the press that
		// opened it would close it again.
		_vasal_editor_form_dropped_id = id;
		_vasal_editor_form_dropped_fresh = true;
		_vasal_editor_form_editing = 0U;
	}
	if (id == _vasal_editor_form_dropped_id) {
		// Drawn open by the overlay, at the end of the frame.
		_vasal_editor_form_dropped_bounds = bounds;
		const size_t length = strlen(items);
		const size_t kept = (length < _vasal_editor_form_items_capacity)
			? length : (_vasal_editor_form_items_capacity - 1U);
		(void)memcpy(_vasal_editor_form_dropped_items, items, kept);
		_vasal_editor_form_dropped_items[kept] = '\0';
		_vasal_editor_form_dropped_value = *active;
		_vasal_editor_form_dropped_seen = true;
	}
	return changed;
}

bool_t vasal_editor_form_choice(vasal_editor_form_s* const form,
	const char* const label, const char* const items, int* const active,
	const uint32_t id) {
	const Rectangle control = _vasal_editor_form_property(form, label, NULL);
	return vasal_editor_form_dropdown(control, items, active, id);
}

void vasal_editor_form_overlay(void) {
	if (!_vasal_editor_form_seen) {
		_vasal_editor_form_editing = 0U;
	}
	if (0U == _vasal_editor_form_dropped_id) {
		return;
	}
	if (!_vasal_editor_form_dropped_seen) {
		_vasal_editor_form_dropped_id = 0U;
		return;
	}
	if (_vasal_editor_form_dropped_fresh) {
		_vasal_editor_form_dropped_fresh = false;
		return;
	}
	GuiUnlock();
	// A list that would leave the window opens upward.
	int count = 1;
	for (size_t i = 0U; '\0' != _vasal_editor_form_dropped_items[i]; i++) {
		if (';' == _vasal_editor_form_dropped_items[i]) {
			count++;
		}
	}
	const Rectangle bounds = _vasal_editor_form_dropped_bounds;
	const float bottom = bounds.y + (((float)count + 1.0f) * bounds.height);
	GuiSetStyle(DROPDOWNBOX, DROPDOWN_ROLL_UP,
		(bottom > (float)GetScreenHeight()) ? 1 : 0);
	int value = _vasal_editor_form_dropped_value;
	const int result = GuiDropdownBox(bounds,
		_vasal_editor_form_dropped_items, &value, true);
	if (value != _vasal_editor_form_dropped_value) {
		_vasal_editor_form_chosen_id = _vasal_editor_form_dropped_id;
		_vasal_editor_form_chosen_value = value;
	}
	if (0 != result) {
		_vasal_editor_form_dropped_id = 0U;
	}
	_vasal_editor_form_lock(false);
}
