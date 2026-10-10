
# The map editor, standalone: one build/<preset>/editor/editor.html with
# everything embedded, opened straight from disk, with no server. It is not
# served to players. Its page shell holds the file picker and the download
# that the program calls; the functions the page calls back are exported.

file(GLOB vasal_editor_sources CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/*.c)
add_executable(editor ${vasal_editor_sources})
target_link_libraries(editor PRIVATE vasal_engine raylib raygui)
vasal_settings(editor)
vasal_web_program(editor editor editor ON
	${CMAKE_CURRENT_SOURCE_DIR}/shell.html)
set(vasal_editor_exports _main _vasal_editor_files_buffer
	_vasal_editor_files_capacity _vasal_editor_files_size
	_vasal_editor_files_opened)
list(JOIN vasal_editor_exports "," vasal_editor_exports)
target_link_options(editor PRIVATE
	-sEXPORTED_FUNCTIONS=${vasal_editor_exports}
	-sEXPORTED_RUNTIME_METHODS=ccall,HEAPU8)
