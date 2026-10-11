
/// The rules being edited, beside the map (engine/rules.h): the unit
/// types, settlement types, starting army, bandit camps and combat values,
/// and their own history for undo and redo. Every change happens inside an
/// operation, as for the map: begin, change the rules directly, end.

#ifndef vasal__editor__ruleset_h
#define vasal__editor__ruleset_h

#include "engine/rules.h"

/// The rules being edited.
vasal_engine_rules_s* vasal_editor_ruleset_rules(void);

/// Starts from the standard rules and forgets the history.
void vasal_editor_ruleset_create(void);

/// Replaces the rules with a rules file's content and forgets the history.
/// Returns the outcome of reading it; unless it is ok, the rules are
/// unchanged.
uint32_t vasal_editor_ruleset_open(const uint8_t* const data,
	const size_t size);

/// Starts an operation. Consecutive operations with the same key, other
/// than 0, are merged into one.
void vasal_editor_ruleset_begin(const uint32_t key);

/// Ends the operation; it is dropped when it changed nothing, and undone
/// when it left the rules invalid (engine/rules.h). Returns false then.
bool_t vasal_editor_ruleset_end(void);

/// Undoes or redoes one operation. Returns false when there is none.
bool_t vasal_editor_ruleset_undo(void);
bool_t vasal_editor_ruleset_redo(void);

/// A number that changes whenever the rules change, so the editor knows
/// when they were changed since they were last saved.
uint32_t vasal_editor_ruleset_revision(void);

/// Adds a unit type at the end, as one operation: a melee type with the
/// first standard type's values, named "unit <n>". Returns its index, or
/// vasal_engine_rules_none when there are vasal_engine_rules_units_max.
uint8_t vasal_editor_ruleset_add_unit(void);

/// Adds a copy of a unit type at the end, named after it, as one
/// operation. Returns its index, or vasal_engine_rules_none when it can't.
uint8_t vasal_editor_ruleset_duplicate_unit(const uint8_t unit);

/// Removes a unit type, as one operation. The slots of it are emptied, a
/// centenar of it becomes none, the settlement types stop offering it, and
/// every reference to a later type follows it down by one. The last type
/// can't be removed. Returns false when nothing was removed.
bool_t vasal_editor_ruleset_remove_unit(const uint8_t unit);

/// Swaps a unit type with the next one, as one operation, references
/// included. Returns false when it is the last.
bool_t vasal_editor_ruleset_swap_units(const uint8_t unit);

/// What a settlement's flags (bit 0 centenars, bit 1 + i unit type i)
/// become when unit type removed is removed, or units a and b swap (pass
/// vasal_engine_rules_none as removed then).
uint32_t vasal_editor_ruleset_flags_after(const uint32_t flags,
	const uint8_t removed, const uint8_t a, const uint8_t b);

#endif
