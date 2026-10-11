
/// Files through the browser: the page opens files from its file picker or
/// dropped on it, downloads saved files, uploads files to the game's
/// server and fetches them from it, and keeps drafts of the map and the
/// rules in the browser's storage, so a closed editor starts where it
/// was. Everything arrives through one buffer the page fills: files one at
/// a time, and the server's answers to requests. The functions the page
/// calls are exported from the program (see the editor's CMakeLists.txt
/// and shell.html).

#ifndef vasal__editor__files_h
#define vasal__editor__files_h

#include "engine/rules.h"

/// Where a file came from.
typedef enum vasal_editor_files_origin_e {
	/// The file picker, or dropped on the page.
	vasal_editor_files_origin_user,
	/// A draft kept in the browser.
	vasal_editor_files_origin_draft,
	/// Fetched from the server.
	vasal_editor_files_origin_server
} vasal_editor_files_origin_e;

/// What a file is.
typedef enum vasal_editor_files_kind_e {
	vasal_editor_files_kind_map,
	vasal_editor_files_kind_rules,
	vasal_editor_files_kind_unknown
} vasal_editor_files_kind_e;

/// The requests to the server.
typedef enum vasal_editor_files_request_e {
	vasal_editor_files_request_upload_map,
	vasal_editor_files_request_upload_rules,
	vasal_editor_files_request_fetch_map,
	vasal_editor_files_request_fetch_rules
} vasal_editor_files_request_e;

/// A file that arrived.
typedef struct vasal_editor_files_arrival_s {
	const uint8_t*              data;
	size_t                      size;
	vasal_editor_files_origin_e origin;
} vasal_editor_files_arrival_s;

/// A server's answer to a request: its HTTP status, 0 when the server
/// could not be reached, and the text it answered.
typedef struct vasal_editor_files_answer_s {
	vasal_editor_files_request_e request;
	uint32_t                     status;
	const char*                  text;
} vasal_editor_files_answer_s;

/// The buffer files pass through, its size, and the size of the file in
/// it to save or upload. Called by the page.
uint8_t* vasal_editor_files_buffer(void);
uint32_t vasal_editor_files_capacity(void);
uint32_t vasal_editor_files_size(void);

/// A buffer of text passing between the page and the editor: the server
/// address and admin key going out, the server's answers coming in. Called
/// by the page.
char* vasal_editor_files_text(void);
uint32_t vasal_editor_files_text_capacity(void);

/// The page copied a file of size bytes into the buffer; 0 when it was too
/// large. origin is a vasal_editor_files_origin_e. Called by the page.
void vasal_editor_files_opened(const uint32_t size, const uint32_t origin);

/// The server answered a request, the text of its answer in the text
/// buffer. Called by the page.
void vasal_editor_files_answered(const uint32_t request,
	const uint32_t status);

/// What a file is, by its magic word.
vasal_editor_files_kind_e vasal_editor_files_kind(const uint8_t* const data,
	const size_t size);

/// Opens the browser's file picker. The file arrives later.
void vasal_editor_files_choose(void);

/// Takes the next file or answer the page has, at most one of each per
/// call: returns whether a file arrived, and whether an answer did. A file
/// of size 0 was too large to open.
bool_t vasal_editor_files_take(vasal_editor_files_arrival_s* const arrival,
	bool_t* const answered, vasal_editor_files_answer_s* const answer);

/// Writes the map, or the rules, and lets the browser download the file,
/// named after it. Returns false when it does not fit into a file.
bool_t vasal_editor_files_save_map(const vasal_engine_map_s* const map);
bool_t vasal_editor_files_save_rules(const vasal_engine_rules_s* const rules);

/// Uploads the map, or the rules, to a server at an address (with or
/// without http://) with an admin key. Uploads go one after the other; the
/// answer arrives later. Returns false when it does not fit into a file.
bool_t vasal_editor_files_upload_map(const vasal_engine_map_s* const map,
	const char* const address, const char* const key);
bool_t vasal_editor_files_upload_rules(
	const vasal_engine_rules_s* const rules, const char* const address,
	const char* const key);

/// Fetches the map, or the rules, a server plays; it arrives later as a
/// file, with an answer.
void vasal_editor_files_fetch(const vasal_editor_files_kind_e kind,
	const char* const address);

/// Keeps the map, or the rules, as a draft in the browser.
void vasal_editor_files_keep_map(const vasal_engine_map_s* const map);
void vasal_editor_files_keep_rules(const vasal_engine_rules_s* const rules);

/// Asks the page for the drafts it keeps; they arrive later as files.
void vasal_editor_files_restore(void);

/// Remembers the server address, and the key when given (NULL forgets
/// it), in the browser.
void vasal_editor_files_remember(const char* const address,
	const char* const key);

/// Reads the remembered server address and key into the buffers given,
/// each of the capacity given. Returns whether a key was remembered.
bool_t vasal_editor_files_recall(char* const address,
	const size_t address_capacity, char* const key,
	const size_t key_capacity);

#endif
