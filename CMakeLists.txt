
# Vasal: a multiplayer medieval strategy game in the browser.
#
# The targets:
#	server  the match server, for Linux
#	tests   the unit tests, for Linux; run them with ctest
#	check   static analysis (cppcheck with MISRA C:2012, clang-tidy), the
#	        specification check and the file rules
#	client  the game, for the browser
#	editor  the map editor, for the browser
# A build tree uses one toolchain, so the first three are built in a Linux
# tree with gcc and the last two in a web tree with Emscripten. Each target
# has a workflow preset (CMakePresets.json) that picks its tree:
#	cmake --workflow --preset <target>
# The environment variable vasal_config chooses debug or release, and the
# tree's folder: build/linux-<config> or build/web-<config>. The scripts in
# scripts/ set it and run the build in the Docker image.
#
# engine/ holds the code every program shares; server/, client/ and
# editor/ one program each; tools/ the project's Python tools; tests/ the
# unit tests; .cmake/ the build's helpers; docs/ the specification and the
# code style. Names of our own CMake variables are lowercase.

cmake_minimum_required(VERSION 3.28)
project(vasal VERSION 0.1.0 LANGUAGES C)

# The milestone being implemented (docs/spec.txt). Agreed criteria of this
# milestone and earlier ones must have tests.
set(vasal_milestone 1)

# The build configuration: debug (with sanitizers on Linux) or release.
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE release)
endif()
if(NOT CMAKE_BUILD_TYPE MATCHES "^(debug|release)$")
	message(FATAL_ERROR
		"the configuration must be debug or release, not ${CMAKE_BUILD_TYPE}")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(.cmake/settings.cmake)
include(.cmake/dependencies.cmake)
include(.cmake/web.cmake)

add_subdirectory(engine)
if(EMSCRIPTEN)
	add_subdirectory(client)
	add_subdirectory(editor)
else()
	enable_testing()
	add_subdirectory(server)
	add_subdirectory(tests)
	include(.cmake/check.cmake)
endif()
