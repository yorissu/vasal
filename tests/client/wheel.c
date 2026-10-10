
/// Tests of the right button and the action wheel. Positions are in window
/// pixels; the button goes down at (400, 300).

#include "client/wheel.h"

#include "cairo.h"

// Presses the button at (400, 300) and holds it for some seconds, in
// frames of 1/60 second, with the pointer still.
static vasal_client_wheel_event_e _vasal_tests_client_wheel_hold(
		vasal_client_wheel_s* const wheel, const float seconds,
		vasal_client_game_action_e* const chosen) {
	vasal_client_wheel_event_e event = vasal_client_wheel_update(wheel, true,
		false, 400.0f, 300.0f, 0.0f, chosen);
	for (float held = 0.0f; held < seconds; held += 1.0f / 60.0f) {
		const vasal_client_wheel_event_e now = vasal_client_wheel_update(wheel,
			false, false, 400.0f, 300.0f, 1.0f / 60.0f, chosen);
		if (vasal_client_wheel_event_none != now) {
			event = now;
		}
	}
	return event;
}

// spec: vintaines.actions#1
cairo_test_new(client_wheel, quick_click_orders_current_action) {
	vasal_client_wheel_s wheel = { 0.0f, 0.0f, 0.0f, false, false };
	vasal_client_game_action_e chosen = vasal_client_game_action_move;
	cairo_assert_eq(_vasal_tests_client_wheel_hold(&wheel, 0.1f, &chosen),
		vasal_client_wheel_event_none);
	cairo_assert_false(wheel.open);
	cairo_assert_eq(vasal_client_wheel_update(&wheel, false, true, 410.0f,
		305.0f, 1.0f / 60.0f, &chosen), vasal_client_wheel_event_quick);
	cairo_assert_eq(chosen, vasal_client_game_action_move);
	cairo_assert_nreq(wheel.press_x, 400.0f);
	cairo_assert_nreq(wheel.press_y, 300.0f);
}

// spec: vintaines.actions#2
cairo_test_new(client_wheel, holding_opens_without_order) {
	vasal_client_wheel_s wheel = { 0.0f, 0.0f, 0.0f, false, false };
	vasal_client_game_action_e chosen = vasal_client_game_action_move;
	cairo_assert_eq(_vasal_tests_client_wheel_hold(&wheel,
		vasal_client_wheel_hold + 0.1f, &chosen),
		vasal_client_wheel_event_none);
	cairo_assert_true(wheel.open);
}

// spec: vintaines.actions#3
cairo_test_new(client_wheel, release_over_action_chooses_it) {
	vasal_client_wheel_s wheel = { 0.0f, 0.0f, 0.0f, false, false };
	vasal_client_game_action_e chosen = vasal_client_game_action_move;
	(void)_vasal_tests_client_wheel_hold(&wheel,
		vasal_client_wheel_hold + 0.1f, &chosen);
	const float x = 400.0f + vasal_client_wheel_radius;
	const float y = 300.0f + vasal_client_wheel_radius;
	cairo_assert_eq(vasal_client_wheel_action(&wheel, x, y),
		vasal_client_game_action_attack_beyond);
	cairo_assert_eq(vasal_client_wheel_update(&wheel, false, true, x, y,
		1.0f / 60.0f, &chosen), vasal_client_wheel_event_chosen);
	cairo_assert_eq(chosen, vasal_client_game_action_attack_beyond);
	cairo_assert_false(wheel.open);
	cairo_assert_nreq(wheel.press_x, 400.0f);
	cairo_assert_nreq(wheel.press_y, 300.0f);
}

// spec: vintaines.actions#3
cairo_test_new(client_wheel, sectors) {
	const vasal_client_wheel_s wheel = { 400.0f, 300.0f, 1.0f, true, true };
	const float r = vasal_client_wheel_radius;
	cairo_assert_eq(vasal_client_wheel_action(&wheel, 400.0f, 300.0f - r),
		vasal_client_game_action_move);
	cairo_assert_eq(vasal_client_wheel_action(&wheel, 400.0f - r,
		300.0f + r), vasal_client_game_action_attack_to_point);
	cairo_assert_eq(vasal_client_wheel_action(&wheel, 400.0f + r,
		300.0f + r), vasal_client_game_action_attack_beyond);
}

// spec: vintaines.actions#4
cairo_test_new(client_wheel, release_away_cancels) {
	vasal_client_wheel_s wheel = { 0.0f, 0.0f, 0.0f, false, false };
	vasal_client_game_action_e chosen = vasal_client_game_action_move;
	(void)_vasal_tests_client_wheel_hold(&wheel,
		vasal_client_wheel_hold + 0.1f, &chosen);
	cairo_assert_eq(vasal_client_wheel_update(&wheel, false, true, 402.0f,
		301.0f, 1.0f / 60.0f, &chosen), vasal_client_wheel_event_cancel);
	cairo_assert_eq(chosen, vasal_client_game_action_move);
	(void)_vasal_tests_client_wheel_hold(&wheel,
		vasal_client_wheel_hold + 0.1f, &chosen);
	cairo_assert_eq(vasal_client_wheel_update(&wheel, false, true, 1000.0f,
		300.0f, 1.0f / 60.0f, &chosen), vasal_client_wheel_event_cancel);
	cairo_assert_eq(chosen, vasal_client_game_action_move);
}

// spec: vintaines.actions#1
cairo_test_new(client_wheel, release_without_press_does_nothing) {
	vasal_client_wheel_s wheel = { 0.0f, 0.0f, 0.0f, false, false };
	vasal_client_game_action_e chosen = vasal_client_game_action_move;
	cairo_assert_eq(vasal_client_wheel_update(&wheel, false, true, 400.0f,
		300.0f, 1.0f / 60.0f, &chosen), vasal_client_wheel_event_none);
}
