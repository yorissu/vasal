
/// Opening and saving map files through the browser, with no server: the
/// page's file picker copies a chosen file into this module's buffer, and
/// saving writes the map into the buffer and lets the page download it.
/// The functions the page calls are exported from the program (see the
/// editor's CMakeLists.txt and shell.html).

#ifndef vasal__editor__files_h
#define vasal__editor__files_h

#include "engine/map.h"

/// The buffer the page copies an opened file into, and reads a saved file
/// from. Called by the page.
uint8_t* vasal_editor_files_buffer(void);

/// The buffer's size in bytes. Called by the page.
uint32_t vasal_editor_files_capacity(void);

/// The size of the file to save. Called by the page.
uint32_t vasal_editor_files_size(void);

/// The page copied a chosen file of size bytes into the buffer; 0 when it
/// was too large. Called by the page.
void vasal_editor_files_opened(const uint32_t size);

/// Opens the browser's file picker. The file arrives later, see
/// vasal_editor_files_take.
void vasal_editor_files_choose(void);

/// Takes the file the page delivered since the last call, if any: returns
/// true and its size (0 when it was too large to open).
bool_t vasal_editor_files_take(size_t* const size);

/// Writes the map and lets the browser download it, named after the map.
/// Returns false when the map does not fit into a map file.
bool_t vasal_editor_files_save(const vasal_engine_map_s* const map);

#endif
