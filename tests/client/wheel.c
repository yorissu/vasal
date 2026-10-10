
/// Tests of the mouse buttons and the action wheel. Positions are in
/// window pixels.

#include "client/layout.h"
#include "client/wheel.h"

#include "cairo.h"

// spec: vintaines.actions#1
cairo_test_new(client_wheel, right_click_orders_the_current_action) {
	vasal_client_wheel_s wheel = { 0.0f, 0.0f, false };
	vasal_client_game_action_e current = vasal_client_game_action_move;
	cairo_assert_true(vasal_client_wheel_buttons(&wheel, false, true, 400.0f,
		300.0f, false, &current));
	cairo_assert_false(wheel.open);
	cairo_assert_eq(current, vasal_client_game_action_move);
	// An open wheel closes.
	wheel.open = true;
	cairo_assert_true(vasal_client_wheel_buttons(&wheel, false, true, 400.0f,
		300.0f, false, &current));
	cairo_assert_false(wheel.open);
}

// spec: vintaines.actions#2
cairo_test_new(client_wheel, left_click_opens_without_order) {
	vasal_client_wheel_s wheel = { 0.0f, 0.0f, false };
	vasal_client_game_action_e current = vasal_client_game_action_move;
	cairo_assert_false(vasal_client_wheel_buttons(&wheel, true, false,
		400.0f, 300.0f, false, &current));
	cairo_assert_true(wheel.open);
	cairo_assert_nreq(wheel.x, 400.0f);
	cairo_assert_nreq(wheel.y, 300.0f);
}

// spec: vintaines.actions#3
cairo_test_new(client_wheel, left_click_on_an_action_picks_it) {
	vasal_client_wheel_s wheel = { 400.0f, 300.0f, true };
	vasal_client_game_action_e current = vasal_client_game_action_count;
	const float x = 400.0f;
	const float y = 300.0f - (0.6f * vasal_client_wheel_radius);
	cairo_assert_eq(vasal_client_wheel_action(&wheel, x, y),
		vasal_client_game_action_move);
	cairo_assert_false(vasal_client_wheel_buttons(&wheel, true, false, x, y,
		false, &current));
	cairo_assert_eq(current, vasal_client_game_action_move);
	cairo_assert_false(wheel.open);
}

// spec: vintaines.actions#3 vintaines.actions#6
cairo_test_new(client_wheel, every_part_of_the_ring_is_an_action) {
	const vasal_client_wheel_s wheel = { 400.0f, 300.0f, true };
	const float r = vasal_client_wheel_radius;
	cairo_assert_eq((size_t)vasal_client_game_action_count, 1U);
	cairo_assert_eq(vasal_client_wheel_action(&wheel, 400.0f, 300.0f - r),
		vasal_client_game_action_move);
	cairo_assert_eq(vasal_client_wheel_action(&wheel, 400.0f, 300.0f + r),
		vasal_client_game_action_move);
	float dx = 0.0f;
	float dy = 0.0f;
	vasal_client_wheel_place(vasal_client_game_action_move, &dx, &dy);
	cairo_assert_nr(dx, 0.0f, 0.001f);
	cairo_assert_lt(dy, 0.0f);
}

// spec: vintaines.actions#4
cairo_test_new(client_wheel, left_click_away_closes) {
	vasal_client_wheel_s wheel = { 400.0f, 300.0f, true };
	vasal_client_game_action_e current = vasal_client_game_action_move;
	cairo_assert_false(vasal_client_wheel_buttons(&wheel, true, false,
		402.0f, 301.0f, false, &current));
	cairo_assert_false(wheel.open);
	cairo_assert_eq(current, vasal_client_game_action_move);
	wheel.open = true;
	cairo_assert_false(vasal_client_wheel_buttons(&wheel, true, false,
		1000.0f, 300.0f, false, &current));
	cairo_assert_false(wheel.open);
	cairo_assert_eq(current, vasal_client_game_action_move);
}

// spec: vintaines.actions#7
cairo_test_new(client_wheel, interface_takes_no_clicks) {
	vasal_client_wheel_s wheel = { 0.0f, 0.0f, false };
	vasal_client_game_action_e current = vasal_client_game_action_move;
	cairo_assert_false(vasal_client_wheel_buttons(&wheel, true, false, 40.0f,
		680.0f, true, &current));
	cairo_assert_false(wheel.open);
	cairo_assert_false(vasal_client_wheel_buttons(&wheel, false, true, 40.0f,
		680.0f, true, &current));
	// The groups' boxes and the minimap are the interface; the middle of
	// the window is the map.
	int32_t left = 0;
	int32_t top = 0;
	vasal_client_layout_group(720.0f, 3U, &left, &top);
	cairo_assert_true(vasal_client_layout_on_interface(1280.0f, 720.0f,
		(float)left + 5.0f, (float)top + 5.0f));
	cairo_assert_true(vasal_client_layout_on_interface(1280.0f, 720.0f,
		1200.0f, 680.0f));
	cairo_assert_false(vasal_client_layout_on_interface(1280.0f, 720.0f,
		640.0f, 360.0f));
}

// spec: vintaines.actions#7
cairo_test_new(client_wheel, neither_button_does_the_other_ones_work) {
	vasal_client_wheel_s wheel = { 0.0f, 0.0f, false };
	vasal_client_game_action_e current = vasal_client_game_action_move;
	// A left click never orders, whether it opens, picks or closes.
	for (size_t i = 0U; i < 3U; i++) {
		cairo_assert_false(vasal_client_wheel_buttons(&wheel, true, false,
			400.0f, 300.0f - (0.6f * vasal_client_wheel_radius), false,
			&current));
	}
	// A right click never opens the wheel.
	wheel.open = false;
	(void)vasal_client_wheel_buttons(&wheel, false, true, 400.0f, 300.0f,
		false, &current);
	cairo_assert_false(wheel.open);
	// No button, nothing.
	cairo_assert_false(vasal_client_wheel_buttons(&wheel, false, false,
		400.0f, 300.0f, false, &current));
	cairo_assert_false(wheel.open);
}
