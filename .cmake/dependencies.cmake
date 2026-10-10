
# dependencies.cmake: third-party code, downloaded at configure time and
# pinned by version and hash.
#
#	raylib 6.0   drawing and input for the client and editor; built as a
#	             static library in the web tree; in the Linux tree only its
#	             headers are used, by the static analysis of the client
#	             and editor
#	raygui 5.0   raylib's widgets, for the editor; one header, built as a
#	             library in the web tree, read by the lint in the Linux
#	             tree
#	cairo 1.3.1  the unit test framework, one header, Linux tree only

include(FetchContent)

set(vasal_raylib_url
	https://github.com/raysan5/raylib/archive/refs/tags/6.0.tar.gz)
set(vasal_raylib_hash
	2b3ee1e2120c7a0796b33062c7e9a694dd8a8caa56a96319ac8c8ecf54a90d0b)
set(vasal_raygui_url
	https://raw.githubusercontent.com/raysan5/raygui/5.0/src/raygui.h)
set(vasal_raygui_hash
	9428525eef081eaec45f8fedbb68199bcbe876f2ce927d5e2a69e811a01ae3a9)
set(vasal_cairo_url
	https://raw.githubusercontent.com/yorissu/cairo/v1.3.1/cairo.h)
set(vasal_cairo_hash
	98e82bf0c2c15fcd3fafaf5dbf292117dfcb117dba1566a92f86b57984a198ad)

if(EMSCRIPTEN)
	set(vasal_raylib_subdirectory .)
else()
	# A source folder that does not exist keeps FetchContent from building
	# raylib; it is only downloaded.
	set(vasal_raylib_subdirectory none)
endif()
FetchContent_Declare(raylib
	URL ${vasal_raylib_url}
	URL_HASH SHA256=${vasal_raylib_hash}
	DOWNLOAD_EXTRACT_TIMESTAMP ON
	SOURCE_SUBDIR ${vasal_raylib_subdirectory}
	SYSTEM)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)
set(vasal_raylib_include ${raylib_SOURCE_DIR}/src)
if(TARGET raylib)
	# raylib's web audio uses Emscripten's EM_ASM, which needs GNU C.
	set_target_properties(raylib PROPERTIES C_EXTENSIONS ON)
	# Its bundled libraries (miniaudio, stb) warn with newer compilers;
	# they are not ours to fix, and the warnings would hide our own.
	target_compile_options(raylib PRIVATE -w)
endif()

set(vasal_raygui_include ${CMAKE_BINARY_DIR}/dependencies/raygui)
file(DOWNLOAD ${vasal_raygui_url} ${vasal_raygui_include}/raygui.h
	EXPECTED_HASH SHA256=${vasal_raygui_hash})
if(EMSCRIPTEN)
	# raygui's implementation, compiled once, with its own settings.
	file(WRITE ${vasal_raygui_include}/raygui.c
		"#define RAYGUI_IMPLEMENTATION
#include \"raygui.h\"
")
	add_library(raygui STATIC ${vasal_raygui_include}/raygui.c)
	target_include_directories(raygui SYSTEM PUBLIC ${vasal_raygui_include})
	target_link_libraries(raygui PUBLIC raylib)
	set_target_properties(raygui PROPERTIES C_EXTENSIONS ON)
	target_compile_options(raygui PRIVATE -w)
endif()

if(NOT EMSCRIPTEN)
	set(vasal_cairo_include ${CMAKE_BINARY_DIR}/dependencies/cairo)
	file(DOWNLOAD ${vasal_cairo_url} ${vasal_cairo_include}/cairo.h
		EXPECTED_HASH SHA256=${vasal_cairo_hash})
endif()
