
/// Forms: the editor's panels and pages lay out their controls from top to
/// bottom with a form, which scrolls with the wheel when its content is
/// taller than its view, clips what is outside, and keeps controls outside
/// the view from taking input. Property rows put a label on the left and
/// the control on the right, with an optional help text under them.
///
/// Text, number and dropdown controls are edited one at a time; the form
/// module remembers which, by an id the caller gives each such control.
/// A dropdown's list is drawn over everything at the end of the frame, and
/// the choice made in it comes back the next frame.

#ifndef vasal__editor__form_h
#define vasal__editor__form_h

#include "engine/types.h"

#include <raylib.h>

/// The height of a row of controls, and the gap after one, in pixels.
#define vasal_editor_form_row 28.0f
#define vasal_editor_form_gap 6.0f

/// A form being laid out.
typedef struct vasal_editor_form_s {
	/// The visible area, in window pixels.
	Rectangle view;
	/// Where the next control goes, and how wide controls are.
	float     x;
	float     y;
	float     width;
	/// Where the content starts, so its height is known at the end.
	float     top;
	/// The form's scroll offset, kept by the caller between frames.
	float*    scroll;
	/// Whether help texts are shown under property rows.
	bool_t    help;
} vasal_editor_form_s;

/// Starts a form over a view, scrolled by the caller's offset, with a
/// padding around the content. Clips drawing to the view until the end.
void vasal_editor_form_begin(vasal_editor_form_s* const form,
	const Rectangle view, float* const scroll, const float padding);

/// Ends the form: scrolls it with the wheel when the pointer is over it,
/// keeps the offset within the content and draws a scroll bar.
void vasal_editor_form_end(vasal_editor_form_s* const form);

/// Takes the next row of a height and returns its rectangle. Controls in
/// it take input only when it is wholly inside the view.
Rectangle vasal_editor_form_take(vasal_editor_form_s* const form,
	const float height);

/// One of count equal parts of a row, with gaps between them.
Rectangle vasal_editor_form_part(const Rectangle row, const size_t count,
	const size_t index);

/// Leaves a gap.
void vasal_editor_form_space(vasal_editor_form_s* const form,
	const float height);

/// A section heading: bold text and a line under it.
void vasal_editor_form_heading(vasal_editor_form_s* const form,
	const char* const text);

/// A line of text, in the normal or dim color.
void vasal_editor_form_label(vasal_editor_form_s* const form,
	const char* const text, const bool_t dim);

/// A paragraph wrapped into the form's width, small and dim.
void vasal_editor_form_paragraph(vasal_editor_form_s* const form,
	const char* const text);

/// A button filling a row. Returns whether it was clicked.
bool_t vasal_editor_form_button(vasal_editor_form_s* const form,
	const char* const text);

/// A property row with a number box between minimum and maximum, with
/// buttons to step it. Returns whether the value changed.
bool_t vasal_editor_form_number(vasal_editor_form_s* const form,
	const char* const label, const char* const help, int* const value,
	const int minimum, const int maximum, const uint32_t id);

/// A property row with a slider between minimum and maximum, the value
/// shown beside it with a suffix. Returns whether the value changed.
bool_t vasal_editor_form_slider(vasal_editor_form_s* const form,
	const char* const label, const char* const help, int* const value,
	const int minimum, const int maximum, const char* const suffix);

/// A property row with a text box over a name of capacity bytes, which
/// keeps to printable ASCII. Returns whether the name changed.
bool_t vasal_editor_form_text(vasal_editor_form_s* const form,
	const char* const label, char* const text, const size_t capacity,
	const uint32_t id);

/// A property row with a check box. Returns whether it changed.
bool_t vasal_editor_form_check(vasal_editor_form_s* const form,
	const char* const label, const char* const help, bool_t* const value);

/// A property row with a dropdown of items separated by ';'. Returns
/// whether a choice made in it arrived.
bool_t vasal_editor_form_choice(vasal_editor_form_s* const form,
	const char* const label, const char* const items, int* const active,
	const uint32_t id);

/// A text box anywhere: returns whether the text changed.
bool_t vasal_editor_form_text_box(const Rectangle bounds, char* const text,
	const size_t capacity, const uint32_t id);

/// A number box with step buttons anywhere: returns whether it changed.
bool_t vasal_editor_form_spinner(const Rectangle bounds, int* const value,
	const int minimum, const int maximum, const uint32_t id);

/// A dropdown anywhere: returns whether a choice made in it arrived.
bool_t vasal_editor_form_dropdown(const Rectangle bounds,
	const char* const items, int* const active, const uint32_t id);

/// Starts a frame: controls take input unless blocked or a dropdown is
/// open. Call once per frame, before everything else.
void vasal_editor_form_frame(void);

/// Draws the open dropdown's list over everything and takes the choice,
/// and stops editing a box that was not drawn this frame. Call once per
/// frame, after everything else.
void vasal_editor_form_overlay(void);

/// Whether text or a number is being typed into a box, so keys are not
/// shortcuts.
bool_t vasal_editor_form_typing(void);

/// Whether a dropdown is open, so clicks elsewhere are not for the map.
bool_t vasal_editor_form_dropped(void);

/// Stops editing whatever box is being edited.
void vasal_editor_form_release(void);

/// Blocks or unblocks every form control, while a dialog is open.
void vasal_editor_form_block(const bool_t blocked);

/// Whether controls are blocked.
bool_t vasal_editor_form_blocked(void);

#endif
