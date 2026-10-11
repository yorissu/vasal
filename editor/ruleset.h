
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

#endif
