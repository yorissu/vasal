
/// Tests of which sprite settlements and lone structures show.

#include "client/sprites.h"

#include "cairo.h"

// spec: client.render#7
cairo_test_new(client_sprites, settlements_show_their_types_art) {
	vasal_engine_map_settlement_s settlement;
	// The sheet's rows: villages 0 to 7, towns 8 to 13, cities 14 to 19,
	// castles 20 to 26.
	static const size_t first[vasal_engine_map_settlement_types] = {
		0U, 8U, 20U, 14U
	};
	static const size_t count[vasal_engine_map_settlement_types] = {
		8U, 6U, 7U, 6U
	};
	for (size_t type = 0U; type < vasal_engine_map_settlement_types;
			type++) {
		vasal_engine_map_settlement_defaults(&settlement,
			(vasal_engine_map_settlement_type_e)type);
		for (uint16_t x = 0U; x < 50U; x++) {
			settlement.position.x = x;
			settlement.position.y = (uint16_t)(x * 3U);
			const size_t sprite = vasal_client_sprites_settlement(&settlement);
			cairo_assert_ge(sprite, first[type]);
			cairo_assert_lt(sprite, first[type] + count[type]);
		}
	}
}

// spec: client.render#7
cairo_test_new(client_sprites, the_same_place_always_looks_the_same) {
	vasal_engine_map_settlement_s settlement;
	vasal_engine_map_settlement_defaults(&settlement,
		vasal_engine_map_settlement_town);
	settlement.position.x = 120U;
	settlement.position.y = 45U;
	const size_t sprite = vasal_client_sprites_settlement(&settlement);
	cairo_assert_eq(vasal_client_sprites_settlement(&settlement), sprite);
	// Different places use different villages.
	size_t seen = 0U;
	vasal_engine_map_settlement_defaults(&settlement,
		vasal_engine_map_settlement_village);
	for (uint16_t x = 0U; x < 64U; x++) {
		settlement.position.x = x;
		seen |= (size_t)1U << vasal_client_sprites_settlement(&settlement);
	}
	cairo_assert_gt(seen & 0xffU, 0x0fU);
}

// spec: client.render#7
cairo_test_new(client_sprites, decorations_show_lone_structures) {
	vasal_engine_map_decoration_s decoration = { { 10U, 10U }, 0U };
	cairo_assert_eq(vasal_client_sprites_decoration(&decoration), 27U);
	decoration.kind = 8U;
	cairo_assert_eq(vasal_client_sprites_decoration(&decoration), 35U);
	// Kinds beyond the sheet wrap around.
	decoration.kind = 13U;
	cairo_assert_eq(vasal_client_sprites_decoration(&decoration), 30U);
}

// spec: client.render#7
cairo_test_new(client_sprites, feet_stand_inside_their_sprites) {
	for (size_t sprite = 0U; sprite < vasal_client_sprites_count; sprite++) {
		const vasal_client_sprites_rect_s rect =
			vasal_client_sprites_rect(sprite);
		cairo_assert_gt(rect.width, 0);
		cairo_assert_le(rect.x + rect.width, 2048);
		cairo_assert_le(rect.y + rect.height, 1024);
		float x = 0.0f;
		float y = 0.0f;
		vasal_client_sprites_foot(sprite, &x, &y);
		cairo_assert_gt(x, 0.0f);
		cairo_assert_lt(y, (float)rect.height);
		cairo_assert_gt(y, (float)rect.height * 0.5f);
	}
}
