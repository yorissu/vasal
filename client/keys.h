
/// The group keys, free of any input library so they can be tested: a key
/// selects its group, with shift adds or removes it, and pressed twice
/// quickly selects every group. Tab moves the focus.

#ifndef vasal__client__keys_h
#define vasal__client__keys_h

#include "client/game.h"

/// How quickly a key must be pressed again to select every group, in
/// seconds (placeholder until tuning).
#define vasal_client_keys_double_tap 0.3f

/// The keys' state between frames.
typedef struct vasal_client_keys_s {
	/// The group key pressed last, or vasal_engine_world_groups for none,
	/// and how long ago, in seconds.
	size_t last;
	float  since;
} vasal_client_keys_s;

/// Starts with no key pressed yet.
void vasal_client_keys_start(vasal_client_keys_s* const keys);

/// Lets time pass, in seconds.
void vasal_client_keys_wait(vasal_client_keys_s* const keys,
	const float seconds);

/// The key of a group was pressed, with or without shift.
void vasal_client_keys_group(vasal_client_keys_s* const keys,
	vasal_client_game_s* const game, const size_t group, const bool_t shift);

#endif
