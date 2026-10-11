
/// The tables describe every field; the standard rules fill them. Reading
/// checks each value against its field's range as it goes, then the rules
/// as a whole.

#include "engine/rules.h"
#include "engine/bytes.h"
#include "engine/crc.h"

static const uint8_t _vasal_engine_rules_magic[8] = {
	'v', 'a', 's', 'a', 'l', 'r', 'u', 'l'
};

static const vasal_engine_rules_field_s
	_vasal_engine_rules_unit_fields[vasal_engine_rules_unit_fields] = {
	{ "Ranged", "1: shoots from afar; 0: fights in melee", 0U, 1U },
	{ "Speed", "1/256 of a tile per tick; 20 is 1.6 tiles a second", 1U,
		128U },
	{ "Sight", "how far it sees, in tiles", 1U, 64U },
	{ "Skill", "weighs its first strike, parry and aim", 0U, 100U },
	{ "Parry", "chance to parry a strike against equal skill, percent",
		0U, 95U },
	{ "Block", "chance to block a shot that hits, percent", 0U, 95U },
	{ "Strike interval", "ticks between its strikes in a duel", 1U, 200U },
	{ "Kill chance", "percent of landed hits that kill; others wound",
		0U, 100U },
	{ "Range", "how far it shoots, in tiles (ranged types)", 0U, 40U },
	{ "Reload", "ticks between its shots (ranged types)", 1U, 400U },
	{ "Sidearm penalty", "percent its chances drop in melee (ranged)", 0U,
		90U },
	{ "Chase", "how far it leaves the formation for an enemy, tiles", 0U,
		40U },
	{ "Careful", "1: once wounded, it leaves fights and keeps its place",
		0U, 1U },
	{ "Cost", "gold to recruit one", 0U, 100000U },
	{ "Salary", "gold it is paid", 0U, 100000U }
};

static const vasal_engine_rules_field_s
	_vasal_engine_rules_combat_fields[vasal_engine_rules_combat_fields] = {
	{ "Duel distance", "1/256 of a tile; troops this close lock into a duel",
		32U, 1024U },
	{ "In combat after a shot", "ticks a troop stays in combat", 0U,
		2000U },
	{ "Mode sight", "tiles aggressive and fleeing vintaines look", 1U,
		64U },
	{ "Guard radius", "tiles a guarding vintaine defends around its post",
		1U, 64U },
	{ "Aim", "a shot's chance to hit at skill 0; skill adds half", 0U,
		100U }
};

static const vasal_engine_rules_field_s _vasal_engine_rules_settlement_fields
	[vasal_engine_rules_settlement_fields] = {
	{ "Capture radius", "tiles", 1U, 64U },
	{ "Troops to capture", "troops needed in the area", 1U, 1000U },
	{ "Capture time", "ticks", 1U, 100000U },
	{ "Capture bonus", "gold shared by the captors", 0U, 1000000U },
	{ "Income", "gold to the alliance's payout", 0U, 1000000U },
	{ "Offers", "bit 0 centenars, bit 1 + i unit type i", 0U,
		0x7fffffffU }
};

// The standard unit types: name, color, then the fields in the order of
// vasal_engine_rules_unit_e (placeholders until tuning).
static const vasal_engine_rules_unit_s _vasal_engine_rules_standard_units[] = {
	{ "spearman", 0xc8b48cffU,
		{ 0U, 20U, 18U, 50U, 38U, 20U, 20U, 40U, 0U, 1U, 0U, 8U, 0U, 30U,
			2U } },
	{ "swordsman", 0xa0a8b8ffU,
		{ 0U, 20U, 18U, 55U, 40U, 30U, 20U, 45U, 0U, 1U, 0U, 8U, 0U, 40U,
			3U } },
	{ "archer", 0x6c9a4cffU,
		{ 1U, 22U, 20U, 50U, 30U, 10U, 20U, 35U, 14U, 40U, 50U, 6U, 1U, 35U,
			3U } },
	{ "thrower", 0x9a7a4cffU,
		{ 1U, 22U, 18U, 50U, 32U, 15U, 20U, 45U, 8U, 30U, 40U, 6U, 1U, 30U,
			2U } },
	{ "scout", 0x8cb4c8ffU,
		{ 0U, 30U, 30U, 35U, 30U, 10U, 20U, 35U, 0U, 1U, 0U, 10U, 1U, 25U,
			2U } },
	{ "captain", 0xe8c040ffU,
		{ 0U, 22U, 22U, 65U, 42U, 30U, 18U, 45U, 0U, 1U, 0U, 8U, 0U, 200U,
			10U } },
	{ "bandit", 0x7a5a48ffU,
		{ 0U, 20U, 16U, 40U, 32U, 10U, 20U, 40U, 0U, 1U, 0U, 10U, 0U, 0U,
			0U } }
};

#define _vasal_engine_rules_standard_count                                    \
	(sizeof(_vasal_engine_rules_standard_units)                               \
		/ sizeof(_vasal_engine_rules_standard_units[0]))

// The standard settlement types' defaults, in the order of
// vasal_engine_map_settlement_type_e, and the fields of
// vasal_engine_rules_settlement_e: what each offers matches the map flags
// (bit 1 spearman, 2 swordsman, 3 archer, 4 thrower, 5 scout).
static const uint32_t _vasal_engine_rules_standard_settlements
	[vasal_engine_map_settlement_types][vasal_engine_rules_settlement_fields] =
{
	{ 5U, 10U, 200U, 50U, 10U, 0x02U },
	{ 7U, 20U, 300U, 100U, 20U, 0x02U | 0x08U },
	{ 8U, 30U, 400U, 150U, 30U, 0x04U | 0x01U },
	{ 10U, 40U, 600U, 200U, 40U, 0x04U | 0x08U | 0x01U }
};

const vasal_engine_rules_field_s* vasal_engine_rules_unit_field(
	const vasal_engine_rules_unit_e field) {
	return &_vasal_engine_rules_unit_fields[(size_t)field
		% (size_t)vasal_engine_rules_unit_fields];
}

const vasal_engine_rules_field_s* vasal_engine_rules_combat_field(
	const vasal_engine_rules_combat_e field) {
	return &_vasal_engine_rules_combat_fields[(size_t)field
		% (size_t)vasal_engine_rules_combat_fields];
}

const vasal_engine_rules_field_s* vasal_engine_rules_settlement_field(
	const vasal_engine_rules_settlement_e field) {
	return &_vasal_engine_rules_settlement_fields[(size_t)field
		% (size_t)vasal_engine_rules_settlement_fields];
}

// A slot.
static vasal_engine_rules_slot_s _vasal_engine_rules_slot(const uint8_t unit,
		const uint8_t count) {
	const vasal_engine_rules_slot_s slot = { unit, count };
	return slot;
}

// spec: rules.content#1 rules.content#3
void vasal_engine_rules_standard(vasal_engine_rules_s* const rules) {
	static const char name[] = "standard";
	for (size_t i = 0U; i < vasal_engine_map_name_capacity; i++) {
		rules->name[i] = '\0';
		if (i < sizeof(name)) {
			rules->name[i] = name[i];
		}
	}
	rules->unit_count = (uint8_t)_vasal_engine_rules_standard_count;
	for (size_t i = 0U; i < vasal_engine_rules_units_max; i++) {
		if (i < _vasal_engine_rules_standard_count) {
			rules->units[i] = _vasal_engine_rules_standard_units[i];
		} else {
			rules->units[i] = _vasal_engine_rules_standard_units[0];
		}
	}
	rules->combat[vasal_engine_rules_combat_duel] = 154U;
	rules->combat[vasal_engine_rules_combat_linger] = 60U;
	rules->combat[vasal_engine_rules_combat_sight] = 18U;
	rules->combat[vasal_engine_rules_combat_guard] = 10U;
	rules->combat[vasal_engine_rules_combat_aim] = 30U;
	for (size_t t = 0U; t < (size_t)vasal_engine_map_settlement_types; t++) {
		for (size_t f = 0U; f < (size_t)vasal_engine_rules_settlement_fields;
				f++) {
			rules->settlements[t][f] =
				_vasal_engine_rules_standard_settlements[t][f];
		}
	}
	for (size_t v = 0U; v < vasal_engine_rules_vintaines; v++) {
		for (size_t s = 0U; s < vasal_engine_rules_slots; s++) {
			rules->start[v][s] =
				_vasal_engine_rules_slot(vasal_engine_rules_none, 0U);
		}
	}
	// 20 spearmen, 20 swordsmen, 16 archers, 8 throwers and 4 scouts; the
	// fifth vintaine starts empty, so the rules of empty vintaines show.
	rules->start[0][0] = _vasal_engine_rules_slot(0U, 20U);
	rules->start[1][0] = _vasal_engine_rules_slot(1U, 20U);
	rules->start[2][0] = _vasal_engine_rules_slot(2U, 16U);
	rules->start[3][0] = _vasal_engine_rules_slot(3U, 8U);
	rules->start[3][1] = _vasal_engine_rules_slot(4U, 4U);
	rules->centenar = 5U;
	for (size_t s = 0U; s < vasal_engine_rules_slots; s++) {
		rules->camp[s] = _vasal_engine_rules_slot(vasal_engine_rules_none, 0U);
	}
	rules->camp[0] = _vasal_engine_rules_slot(6U, 10U);
	rules->camp[1] = _vasal_engine_rules_slot(2U, 4U);
}

// Whether slots name unit types that exist and hold at most a vintaine.
static bool_t _vasal_engine_rules_slots_valid(
		const vasal_engine_rules_s* const rules,
		const vasal_engine_rules_slot_s* const slots) {
	uint32_t total = 0U;
	bool_t valid = true;
	for (size_t s = 0U; s < vasal_engine_rules_slots; s++) {
		total += slots[s].count;
		if ((0U < slots[s].count) && (slots[s].unit >= rules->unit_count)) {
			valid = false;
		}
	}
	return valid && (vasal_engine_rules_vintaine_max >= total);
}

// Whether a value lies in a field's range.
static bool_t _vasal_engine_rules_within(
		const vasal_engine_rules_field_s* const field, const uint32_t value) {
	return (field->minimum <= value) && (field->maximum >= value);
}

// spec: rules.content#2
bool_t vasal_engine_rules_valid(const vasal_engine_rules_s* const rules) {
	bool_t valid = (0U < rules->unit_count)
		&& (vasal_engine_rules_units_max >= rules->unit_count);
	for (size_t u = 0U; valid && (u < rules->unit_count); u++) {
		for (size_t f = 0U; f < (size_t)vasal_engine_rules_unit_fields; f++) {
			valid = valid && _vasal_engine_rules_within(
				&_vasal_engine_rules_unit_fields[f], rules->units[u].values[f]);
		}
	}
	for (size_t f = 0U; f < (size_t)vasal_engine_rules_combat_fields; f++) {
		valid = valid && _vasal_engine_rules_within(
			&_vasal_engine_rules_combat_fields[f], rules->combat[f]);
	}
	// Bits for centenars and the unit types that exist.
	const uint32_t offered = ((uint32_t)1U << (rules->unit_count + 1U)) - 1U;
	for (size_t t = 0U; t < (size_t)vasal_engine_map_settlement_types; t++) {
		for (size_t f = 0U; f < (size_t)vasal_engine_rules_settlement_fields;
				f++) {
			valid = valid && _vasal_engine_rules_within(
				&_vasal_engine_rules_settlement_fields[f],
				rules->settlements[t][f]);
		}
		valid = valid && (0U == (rules->settlements[t]
			[vasal_engine_rules_settlement_flags] & ~offered));
	}
	for (size_t v = 0U; v < vasal_engine_rules_vintaines; v++) {
		valid = valid && _vasal_engine_rules_slots_valid(rules,
			rules->start[v]);
	}
	valid = valid && _vasal_engine_rules_slots_valid(rules, rules->camp);
	return valid && ((vasal_engine_rules_none == rules->centenar)
		|| (rules->centenar < rules->unit_count));
}

static void _vasal_engine_rules_write_name(
		vasal_engine_bytes_writer_s* const writer, const char* const name) {
	bool_t ended = false;
	for (size_t i = 0U; i < vasal_engine_map_name_capacity; i++) {
		ended = ended || ('\0' == name[i]);
		const uint8_t byte = ended ? 0U : (uint8_t)name[i];
		vasal_engine_bytes_write_u8(writer, byte);
	}
}

static void _vasal_engine_rules_write_slots(
		vasal_engine_bytes_writer_s* const writer,
		const vasal_engine_rules_slot_s* const slots) {
	for (size_t s = 0U; s < vasal_engine_rules_slots; s++) {
		vasal_engine_bytes_write_u8(writer, slots[s].unit);
		vasal_engine_bytes_write_u8(writer, slots[s].count);
	}
}

// spec: rules.content#2
size_t vasal_engine_rules_write(const vasal_engine_rules_s* const rules,
	uint8_t* const buffer, const size_t capacity) {
	if (!vasal_engine_rules_valid(rules)) {
		return 0U;
	}
	vasal_engine_bytes_writer_s writer =
		vasal_engine_bytes_writer(buffer, capacity);
	vasal_engine_bytes_write(&writer, _vasal_engine_rules_magic,
		sizeof(_vasal_engine_rules_magic));
	vasal_engine_bytes_write_u16(&writer, vasal_engine_rules_version);
	_vasal_engine_rules_write_name(&writer, rules->name);
	vasal_engine_bytes_write_u8(&writer, rules->unit_count);
	for (size_t u = 0U; u < rules->unit_count; u++) {
		const vasal_engine_rules_unit_s* const unit = &rules->units[u];
		_vasal_engine_rules_write_name(&writer, unit->name);
		vasal_engine_bytes_write_u32(&writer, unit->color);
		for (size_t f = 0U; f < (size_t)vasal_engine_rules_unit_fields; f++) {
			vasal_engine_bytes_write_u32(&writer, unit->values[f]);
		}
	}
	for (size_t f = 0U; f < (size_t)vasal_engine_rules_combat_fields; f++) {
		vasal_engine_bytes_write_u32(&writer, rules->combat[f]);
	}
	for (size_t t = 0U; t < (size_t)vasal_engine_map_settlement_types; t++) {
		for (size_t f = 0U; f < (size_t)vasal_engine_rules_settlement_fields;
				f++) {
			vasal_engine_bytes_write_u32(&writer, rules->settlements[t][f]);
		}
	}
	for (size_t v = 0U; v < vasal_engine_rules_vintaines; v++) {
		_vasal_engine_rules_write_slots(&writer, rules->start[v]);
	}
	vasal_engine_bytes_write_u8(&writer, rules->centenar);
	_vasal_engine_rules_write_slots(&writer, rules->camp);
	if (writer.failed || ((writer.size + 4U) > capacity)) {
		return 0U;
	}
	const uint32_t crc = vasal_engine_crc_compute(buffer, writer.size);
	vasal_engine_bytes_write_u32(&writer, crc);
	return writer.size;
}

// Reads a name: printable ASCII, then zeros up to the capacity. Returns
// false when it isn't.
static bool_t _vasal_engine_rules_read_name(
		vasal_engine_bytes_reader_s* const reader, char* const name) {
	bool_t ended = false;
	bool_t valid = true;
	for (size_t i = 0U; i < vasal_engine_map_name_capacity; i++) {
		const uint8_t byte = vasal_engine_bytes_read_u8(reader);
		if (0U == byte) {
			ended = true;
		} else if (ended || (byte < 0x20U) || (byte > 0x7eU)) {
			valid = false;
		} else {
			// A character of the name.
		}
		name[i] = (char)byte;
	}
	name[vasal_engine_map_name_capacity - 1U] = '\0';
	return valid && ended;
}

static void _vasal_engine_rules_read_slots(
		vasal_engine_bytes_reader_s* const reader,
		vasal_engine_rules_slot_s* const slots) {
	for (size_t s = 0U; s < vasal_engine_rules_slots; s++) {
		slots[s].unit = vasal_engine_bytes_read_u8(reader);
		slots[s].count = vasal_engine_bytes_read_u8(reader);
	}
}

// Reads everything after the version; the checks follow.
static bool_t _vasal_engine_rules_read_body(
		vasal_engine_bytes_reader_s* const reader,
		vasal_engine_rules_s* const rules) {
	bool_t named = _vasal_engine_rules_read_name(reader, rules->name);
	rules->unit_count = vasal_engine_bytes_read_u8(reader);
	if (vasal_engine_rules_units_max < rules->unit_count) {
		return false;
	}
	for (size_t u = 0U; u < rules->unit_count; u++) {
		vasal_engine_rules_unit_s* const unit = &rules->units[u];
		named = _vasal_engine_rules_read_name(reader, unit->name) && named;
		unit->color = vasal_engine_bytes_read_u32(reader);
		for (size_t f = 0U; f < (size_t)vasal_engine_rules_unit_fields; f++) {
			unit->values[f] = vasal_engine_bytes_read_u32(reader);
		}
	}
	for (size_t f = 0U; f < (size_t)vasal_engine_rules_combat_fields; f++) {
		rules->combat[f] = vasal_engine_bytes_read_u32(reader);
	}
	for (size_t t = 0U; t < (size_t)vasal_engine_map_settlement_types; t++) {
		for (size_t f = 0U; f < (size_t)vasal_engine_rules_settlement_fields;
				f++) {
			rules->settlements[t][f] = vasal_engine_bytes_read_u32(reader);
		}
	}
	for (size_t v = 0U; v < vasal_engine_rules_vintaines; v++) {
		_vasal_engine_rules_read_slots(reader, rules->start[v]);
	}
	rules->centenar = vasal_engine_bytes_read_u8(reader);
	_vasal_engine_rules_read_slots(reader, rules->camp);
	return named;
}

// spec: rules.content#2
vasal_engine_rules_read_e vasal_engine_rules_read(const uint8_t* const data,
	const size_t size, vasal_engine_rules_s* const rules,
	uint16_t* const version) {
	vasal_engine_bytes_reader_s reader = vasal_engine_bytes_reader(data, size);
	uint8_t magic[sizeof(_vasal_engine_rules_magic)];
	vasal_engine_bytes_read(&reader, magic, sizeof(magic));
	for (size_t i = 0U; i < sizeof(magic); i++) {
		if (magic[i] != _vasal_engine_rules_magic[i]) {
			return vasal_engine_rules_read_magic;
		}
	}
	*version = vasal_engine_bytes_read_u16(&reader);
	if (reader.failed) {
		return vasal_engine_rules_read_length;
	}
	if (vasal_engine_rules_version != *version) {
		return vasal_engine_rules_read_version;
	}
	if ((4U > size) || (vasal_engine_rules_capacity < size)) {
		return vasal_engine_rules_read_length;
	}
	vasal_engine_bytes_reader_s tail =
		vasal_engine_bytes_reader(&data[size - 4U], 4U);
	if (vasal_engine_bytes_read_u32(&tail)
			!= vasal_engine_crc_compute(data, size - 4U)) {
		return vasal_engine_rules_read_checksum;
	}
	reader.size = size - 4U;
	const bool_t named = _vasal_engine_rules_read_body(&reader, rules);
	if (reader.failed || (reader.at != reader.size)) {
		return vasal_engine_rules_read_length;
	}
	if (!named || !vasal_engine_rules_valid(rules)) {
		return vasal_engine_rules_read_value;
	}
	return vasal_engine_rules_read_ok;
}

// Whether two names are the same up to their end.
static bool_t _vasal_engine_rules_same_name(const char* const a,
		const char* const b) {
	bool_t same = true;
	bool_t ended = false;
	for (size_t i = 0U; same && !ended
			&& (i < vasal_engine_map_name_capacity); i++) {
		same = a[i] == b[i];
		ended = '\0' == a[i];
	}
	return same;
}

static bool_t _vasal_engine_rules_same_slots(
		const vasal_engine_rules_slot_s* const a,
		const vasal_engine_rules_slot_s* const b) {
	bool_t same = true;
	for (size_t s = 0U; s < vasal_engine_rules_slots; s++) {
		same = same && (a[s].unit == b[s].unit) && (a[s].count == b[s].count);
	}
	return same;
}

bool_t vasal_engine_rules_equal(const vasal_engine_rules_s* const a,
	const vasal_engine_rules_s* const b) {
	bool_t same = _vasal_engine_rules_same_name(a->name, b->name)
		&& (a->unit_count == b->unit_count) && (a->centenar == b->centenar)
		&& _vasal_engine_rules_same_slots(a->camp, b->camp);
	for (size_t u = 0U; same && (u < a->unit_count); u++) {
		same = _vasal_engine_rules_same_name(a->units[u].name,
			b->units[u].name) && (a->units[u].color == b->units[u].color);
		for (size_t f = 0U; f < (size_t)vasal_engine_rules_unit_fields; f++) {
			same = same && (a->units[u].values[f] == b->units[u].values[f]);
		}
	}
	for (size_t f = 0U; f < (size_t)vasal_engine_rules_combat_fields; f++) {
		same = same && (a->combat[f] == b->combat[f]);
	}
	for (size_t t = 0U; t < (size_t)vasal_engine_map_settlement_types; t++) {
		for (size_t f = 0U; f < (size_t)vasal_engine_rules_settlement_fields;
				f++) {
			same = same && (a->settlements[t][f] == b->settlements[t][f]);
		}
	}
	for (size_t v = 0U; v < vasal_engine_rules_vintaines; v++) {
		same = same && _vasal_engine_rules_same_slots(a->start[v], b->start[v]);
	}
	return same;
}

uint32_t vasal_engine_rules_get(const vasal_engine_rules_s* const rules,
	const size_t unit, const vasal_engine_rules_unit_e field) {
	return rules->units[unit % vasal_engine_rules_units_max].values[(size_t)
		field % (size_t)vasal_engine_rules_unit_fields];
}

// spec: rules.content#4 settlements.types#1
void vasal_engine_rules_settlement_defaults(
	const vasal_engine_rules_s* const rules,
	vasal_engine_map_settlement_s* const settlement,
	const vasal_engine_map_settlement_type_e type) {
	size_t index = (size_t)type;
	if (index >= (size_t)vasal_engine_map_settlement_types) {
		index = 0U;
	}
	const uint32_t* const values = rules->settlements[index];
	settlement->type = (uint8_t)index;
	settlement->radius =
		(uint16_t)values[vasal_engine_rules_settlement_radius];
	settlement->requirement =
		(uint16_t)values[vasal_engine_rules_settlement_requirement];
	settlement->capture_time = values[vasal_engine_rules_settlement_capture];
	settlement->bonus = values[vasal_engine_rules_settlement_bonus];
	settlement->income = values[vasal_engine_rules_settlement_income];
	settlement->flags = values[vasal_engine_rules_settlement_flags];
}
