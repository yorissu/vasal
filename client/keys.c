
/// A double tap needs the same key twice, without shift: with shift, the
/// second press undoes the first, as toggling should.

#include "client/keys.h"

void vasal_client_keys_start(vasal_client_keys_s* const keys) {
	keys->last = vasal_engine_world_groups;
	keys->since = 0.0f;
}

void vasal_client_keys_wait(vasal_client_keys_s* const keys,
	const float seconds) {
	keys->since += seconds;
}

// spec: vintaines.select#1 vintaines.select#2 vintaines.select#5
// spec: vintaines.select#8
void vasal_client_keys_group(vasal_client_keys_s* const keys,
	vasal_client_game_s* const game, const size_t group, const bool_t shift) {
	const bool_t again = (keys->last == group)
		&& (keys->since <= vasal_client_keys_double_tap);
	if (again && !shift) {
		vasal_client_game_select_all(game, group);
	} else {
		(void)vasal_client_game_select(game, group, shift);
	}
	keys->last = group;
	keys->since = 0.0f;
}
