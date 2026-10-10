
/// The map being edited, and its history for undo and redo. Every change
/// happens inside an operation: begin it, change the map (tiles through
/// vasal_editor_document_paint, anything else directly), end it. An
/// operation remembers the tiles it changed and a copy of everything else
/// as it was, so undoing and redoing are exact.

#ifndef vasal__editor__document_h
#define vasal__editor__document_h

#include "engine/map.h"

/// How many operations can be undone.
#define vasal_editor_document_history 24U

/// The map being edited.
vasal_engine_map_s* vasal_editor_document_map(void);

/// Starts a new map of grass, with the standard terrain types ready to
/// paint (engine/map.h), and forgets the history. Returns false when a
/// side is 0 or too long.
bool_t vasal_editor_document_create(const uint16_t width,
	const uint16_t height);

/// Replaces the map with a map file's content and forgets the history.
/// Returns the outcome of reading it; unless it is ok, the map is
/// unchanged.
uint32_t vasal_editor_document_open(const uint8_t* const data,
	const size_t size);

/// Starts an operation. Consecutive operations with the same key, other
/// than 0, are merged into one, so dragging a value or an object does not
/// fill the history.
void vasal_editor_document_begin(const uint32_t key);

/// Tells the open operation that something other than tiles changed;
/// an operation that changed nothing is dropped when it ends.
void vasal_editor_document_changed(void);

/// Sets a tile's terrain, within an operation.
void vasal_editor_document_paint(const uint16_t x, const uint16_t y,
	const uint8_t terrain);

/// Ends the operation; it is dropped when it changed nothing.
void vasal_editor_document_end(void);

/// A number that changes whenever the map changes, so views know when to
/// redraw what they keep.
uint32_t vasal_editor_document_revision(void);

/// Undoes or redoes one operation. Returns false when there is none.
bool_t vasal_editor_document_undo(void);
bool_t vasal_editor_document_redo(void);

#endif
