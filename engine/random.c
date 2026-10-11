
/// PCG32 (O'Neill's XSH RR variant): a 64-bit linear congruential state,
/// output permuted to 32 bits. A stream is chosen by the increment, which
/// must be odd.

#include "engine/random.h"

#define _vasal_engine_random_multiplier 6364136223846793005U

// spec: simulation.random#1 simulation.random#2
vasal_engine_random_s vasal_engine_random_start(const uint64_t seed,
	const vasal_engine_random_stream_e stream) {
	vasal_engine_random_s random;
	random.state = 0U;
	random.increment = (((uint64_t)stream + 1U) << 1U) | 1U;
	(void)vasal_engine_random_next(&random);
	random.state += seed;
	(void)vasal_engine_random_next(&random);
	return random;
}

uint32_t vasal_engine_random_next(vasal_engine_random_s* const random) {
	const uint64_t old = random->state;
	random->state = (old * _vasal_engine_random_multiplier)
		+ random->increment;
	const uint64_t mixed = ((old >> 18U) ^ old) >> 27U;
	const uint32_t shifted = (uint32_t)(mixed & 0xffffffffU);
	const uint32_t rotation = (uint32_t)(old >> 59U);
	return (shifted >> rotation) | (shifted << ((32U - rotation) & 31U));
}

uint32_t vasal_engine_random_below(vasal_engine_random_s* const random,
	const uint32_t bound) {
	// Numbers below the threshold would make the low results likelier.
	const uint32_t threshold = (uint32_t)(0U - bound) % bound;
	uint32_t value = vasal_engine_random_next(random);
	while (value < threshold) {
		value = vasal_engine_random_next(random);
	}
	return value % bound;
}

// spec: combat.random#1
bool_t vasal_engine_random_chance(vasal_engine_random_s* const random,
	const uint32_t percent) {
	return vasal_engine_random_below(random, 100U) < percent;
}
