
/// Tests of the drawing order.

#include "client/depth.h"

#include "cairo.h"

// spec: client.render#2
cairo_test_new(client_depth, lower_ones_drawn_later) {
	vasal_client_depth_troop_s troops[5] = {
		{ 1.0f, 9.0f, false }, { 2.0f, 3.0f, false }, { 3.0f, 7.5f, true },
		{ 4.0f, 3.0f, false }, { 5.0f, 0.5f, false }
	};
	vasal_client_depth_sort(troops, 5U);
	cairo_assert_nreq(troops[0].x, 2.0f);
	cairo_assert_nreq(troops[1].x, 5.0f);
	cairo_assert_nreq(troops[2].x, 4.0f);
	cairo_assert_nreq(troops[3].x, 1.0f);
	cairo_assert_nreq(troops[4].x, 3.0f);
	cairo_assert_true(troops[4].centenar);
}

// spec: client.render#2
cairo_test_new(client_depth, empty_and_single) {
	vasal_client_depth_troop_s troop = { 1.0f, 2.0f, false };
	vasal_client_depth_sort(&troop, 0U);
	vasal_client_depth_sort(&troop, 1U);
	cairo_assert_nreq(troop.y, 2.0f);
}
