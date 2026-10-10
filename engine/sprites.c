
/// The rectangles were found by cutting the sprites out of the artist's
/// sheet, whose background was a painted checkerboard: everything the
/// checkerboard reached from the sheet's edges, stopping at the sprites'
/// dark outlines, became transparent.

#include "engine/sprites.h"

// The first sprite of each row of the sheet.
#define _vasal_engine_sprites_villages 0U
#define _vasal_engine_sprites_towns 8U
#define _vasal_engine_sprites_cities 14U
#define _vasal_engine_sprites_castles 20U
#define _vasal_engine_sprites_lone 27U

static const vasal_engine_sprites_rect_s
	_vasal_engine_sprites_rects[vasal_engine_sprites_count] = {
	// Villages.
	{ 37, 90, 141, 106 }, { 237, 67, 115, 131 }, { 428, 80, 148, 117 },
	{ 641, 54, 153, 142 }, { 856, 94, 132, 100 }, { 1026, 86, 131, 109 },
	{ 1210, 86, 113, 107 }, { 1374, 87, 131, 109 },
	// Towns.
	{ 60, 240, 162, 137 }, { 289, 230, 180, 146 }, { 539, 236, 171, 148 },
	{ 776, 237, 175, 142 }, { 1017, 244, 185, 136 }, { 1291, 245, 173, 134 },
	// Cities.
	{ 38, 418, 185, 170 }, { 285, 418, 197, 164 }, { 541, 435, 191, 147 },
	{ 791, 421, 174, 164 }, { 1028, 429, 182, 157 }, { 1264, 425, 236, 166 },
	// Castles.
	{ 21, 636, 151, 148 }, { 222, 637, 175, 144 }, { 446, 615, 191, 174 },
	{ 696, 620, 156, 166 }, { 923, 628, 186, 157 }, { 1163, 612, 157, 172 },
	{ 1363, 649, 145, 135 },
	// Structures.
	{ 41, 817, 76, 150 }, { 188, 824, 78, 138 }, { 342, 826, 62, 138 },
	{ 474, 823, 71, 142 }, { 617, 830, 137, 132 }, { 788, 844, 113, 119 },
	{ 939, 837, 115, 126 }, { 1096, 830, 125, 137 }, { 1249, 860, 134, 102 },
	{ 1401, 854, 113, 106 }
};

// Each settlement type's first sprite and how many it has, in the order
// of vasal_engine_map_settlement_type_e: village, town, castle, city.
static const size_t
	_vasal_engine_sprites_types[vasal_engine_map_settlement_types][2] = {
	{ _vasal_engine_sprites_villages, 8U },
	{ _vasal_engine_sprites_towns, 6U },
	{ _vasal_engine_sprites_castles, 7U },
	{ _vasal_engine_sprites_cities, 6U }
};

vasal_engine_sprites_rect_s vasal_engine_sprites_rect(const size_t sprite) {
	return _vasal_engine_sprites_rects[sprite % vasal_engine_sprites_count];
}

// spec: client.render#7 editor.tools#6
size_t vasal_engine_sprites_settlement(
	const vasal_engine_map_settlement_s* const settlement) {
	size_t type = (size_t)settlement->type;
	if (type >= (size_t)vasal_engine_map_settlement_types) {
		type = (size_t)vasal_engine_map_settlement_village;
	}
	// A hash of the position, the same on every machine.
	const uint32_t hash = ((uint32_t)settlement->position.x * 73856093U)
		^ ((uint32_t)settlement->position.y * 19349663U);
	const uint32_t mixed = hash >> 4U;
	return _vasal_engine_sprites_types[type][0]
		+ ((size_t)mixed % _vasal_engine_sprites_types[type][1]);
}

// spec: client.render#7 editor.tools#6
size_t vasal_engine_sprites_decoration(
	const vasal_engine_map_decoration_s* const decoration) {
	return _vasal_engine_sprites_lone
		+ ((size_t)decoration->kind % vasal_engine_sprites_structures);
}

void vasal_engine_sprites_foot(const size_t sprite, int32_t* const x,
	int32_t* const y) {
	const vasal_engine_sprites_rect_s rect = vasal_engine_sprites_rect(sprite);
	*x = rect.width / 2;
	*y = rect.height - (rect.width / 4);
}

size_t vasal_engine_sprites_type(
	const vasal_engine_map_settlement_type_e type) {
	size_t index = (size_t)type;
	if (index >= (size_t)vasal_engine_map_settlement_types) {
		index = 0U;
	}
	return _vasal_engine_sprites_types[index][0];
}
