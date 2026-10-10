
/// Insertion sort: a centaine is small, and between frames the order
/// barely changes, which is insertion sort's best case.

#include "client/depth.h"

// spec: client.render#2
void vasal_client_depth_sort(vasal_client_depth_troop_s* const troops,
	const size_t count) {
	for (size_t i = 1U; i < count; i++) {
		const vasal_client_depth_troop_s troop = troops[i];
		size_t at = i;
		while ((0U < at) && ((troops[at - 1U].x + troops[at - 1U].y)
				> (troop.x + troop.y))) {
			troops[at] = troops[at - 1U];
			at--;
		}
		troops[at] = troop;
	}
}
