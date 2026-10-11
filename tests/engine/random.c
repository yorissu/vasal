
/// Tests of the seeded random streams.

#include "engine/random.h"

#include "cairo.h"

// spec: simulation.random#1
cairo_test_new(engine_random, the_same_seed_the_same_sequence) {
	vasal_engine_random_s a = vasal_engine_random_start(42U,
		vasal_engine_random_combat);
	vasal_engine_random_s b = vasal_engine_random_start(42U,
		vasal_engine_random_combat);
	for (size_t i = 0U; i < 100U; i++) {
		cairo_assert_eq(vasal_engine_random_next(&a),
			vasal_engine_random_next(&b));
	}
	vasal_engine_random_s c = vasal_engine_random_start(43U,
		vasal_engine_random_combat);
	vasal_engine_random_s d = vasal_engine_random_start(42U,
		vasal_engine_random_combat);
	size_t same = 0U;
	for (size_t i = 0U; i < 100U; i++) {
		same += (vasal_engine_random_next(&c) == vasal_engine_random_next(&d))
			? 1U : 0U;
	}
	cairo_assert_lt(same, 3U);
}

// spec: simulation.random#2
cairo_test_new(engine_random, streams_do_not_disturb_each_other) {
	vasal_engine_random_s combat = vasal_engine_random_start(7U,
		vasal_engine_random_combat);
	vasal_engine_random_s npcs = vasal_engine_random_start(7U,
		vasal_engine_random_npcs);
	vasal_engine_random_s alone = vasal_engine_random_start(7U,
		vasal_engine_random_npcs);
	for (size_t i = 0U; i < 50U; i++) {
		(void)vasal_engine_random_next(&combat);
	}
	for (size_t i = 0U; i < 50U; i++) {
		cairo_assert_eq(vasal_engine_random_next(&npcs),
			vasal_engine_random_next(&alone));
	}
}

// spec: combat.random#1
cairo_test_new(engine_random, chances_come_out_about_right) {
	vasal_engine_random_s random = vasal_engine_random_start(1U,
		vasal_engine_random_combat);
	uint32_t hits = 0U;
	for (size_t i = 0U; i < 10000U; i++) {
		hits += vasal_engine_random_chance(&random, 30U) ? 1U : 0U;
		cairo_assert_lt(vasal_engine_random_below(&random, 7U), 7U);
	}
	cairo_assert_gt(hits, 2800U);
	cairo_assert_lt(hits, 3200U);
	cairo_assert_false(vasal_engine_random_chance(&random, 0U));
	cairo_assert_true(vasal_engine_random_chance(&random, 100U));
}
