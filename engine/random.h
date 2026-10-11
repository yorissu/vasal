
/// Seeded random numbers: PCG32 generators owned by the match, never a
/// global one or the clock. A match has one seed; each system draws from
/// its own stream of it, so drawing more in one system changes nothing in
/// another.

#ifndef vasal__engine__random_h
#define vasal__engine__random_h

#include "engine/types.h"

/// The streams of a match.
typedef enum vasal_engine_random_stream_e {
	vasal_engine_random_combat,
	vasal_engine_random_npcs,
	vasal_engine_random_streams
} vasal_engine_random_stream_e;

/// A generator.
typedef struct vasal_engine_random_s {
	uint64_t state;
	uint64_t increment;
} vasal_engine_random_s;

/// A generator of a stream of a seed.
vasal_engine_random_s vasal_engine_random_start(const uint64_t seed,
	const vasal_engine_random_stream_e stream);

/// The next number, of 32 bits.
uint32_t vasal_engine_random_next(vasal_engine_random_s* const random);

/// A number from 0 to below bound (bound > 0), without bias.
uint32_t vasal_engine_random_below(vasal_engine_random_s* const random,
	const uint32_t bound);

/// Whether a roll succeeds with a chance in percent (0 never, 100
/// always).
bool_t vasal_engine_random_chance(vasal_engine_random_s* const random,
	const uint32_t percent);

#endif
