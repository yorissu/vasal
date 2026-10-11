
# The unit tests: the C tests in one program (tests, written to
# vasal_tests) and the Python tests of the tools. CTest runs both.

file(GLOB vasal_test_sources CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/client/*.c
	${CMAKE_CURRENT_SOURCE_DIR}/editor/*.c
	${CMAKE_CURRENT_SOURCE_DIR}/engine/*.c
	${CMAKE_CURRENT_SOURCE_DIR}/server/*.c)
# The client's and the editor's logic, which uses neither raylib nor
# Emscripten, so it runs here too.
set(vasal_test_web_sources
	${PROJECT_SOURCE_DIR}/client/camera.c
	${PROJECT_SOURCE_DIR}/client/depth.c
	${PROJECT_SOURCE_DIR}/client/game.c
	${PROJECT_SOURCE_DIR}/client/keys.c
	${PROJECT_SOURCE_DIR}/client/layout.c
	${PROJECT_SOURCE_DIR}/client/minimap.c
	${PROJECT_SOURCE_DIR}/client/stress.c
	${PROJECT_SOURCE_DIR}/client/wheel.c
	${PROJECT_SOURCE_DIR}/editor/document.c
	${PROJECT_SOURCE_DIR}/editor/ruleset.c)
add_executable(tests main.c ${vasal_test_sources} ${vasal_test_web_sources})
set_target_properties(tests PROPERTIES OUTPUT_NAME vasal_tests)
target_include_directories(tests SYSTEM PRIVATE ${vasal_cairo_include})
target_link_libraries(tests PRIVATE vasal_server_modules vasal_engine m)
vasal_settings(tests)
add_test(NAME unit COMMAND tests)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME tools
	COMMAND ${Python3_EXECUTABLE} -m unittest discover
		-s ${CMAKE_CURRENT_SOURCE_DIR}/tools)

# The determinism scenario, built natively with gcc and for WebAssembly with
# Emscripten (for Node, in one file); the test runs both and compares what
# they print. Emscripten links in /tmp, since wasm-ld fails writing to the
# mounted project folder on Docker Desktop for Windows.
add_executable(determinism determinism/scenario.c)
set_target_properties(determinism PROPERTIES OUTPUT_NAME vasal_determinism)
target_link_libraries(determinism PRIVATE vasal_engine)
vasal_settings(determinism)

find_program(vasal_emcc emcc REQUIRED)
file(GLOB vasal_node $ENV{EMSDK}/node/*/bin/node)
if(NOT vasal_node)
	message(FATAL_ERROR "Node of the Emscripten SDK not found")
endif()
list(GET vasal_node 0 vasal_node)
file(GLOB vasal_determinism_engine CONFIGURE_DEPENDS
	${PROJECT_SOURCE_DIR}/engine/*.c)
set(vasal_determinism_web ${CMAKE_CURRENT_BINARY_DIR}/vasal_determinism.js)
add_custom_command(OUTPUT ${vasal_determinism_web}
	COMMAND ${CMAKE_COMMAND} -E make_directory /tmp/vasal_determinism
	COMMAND ${vasal_emcc} -std=c11 -O2 -I${PROJECT_SOURCE_DIR}
		-sENVIRONMENT=node -sSINGLE_FILE=1 -sINITIAL_MEMORY=67108864
		${vasal_determinism_engine}
		${CMAKE_CURRENT_SOURCE_DIR}/determinism/scenario.c
		-o /tmp/vasal_determinism/vasal_determinism.js
	COMMAND ${CMAKE_COMMAND} -E copy
		/tmp/vasal_determinism/vasal_determinism.js ${vasal_determinism_web}
	DEPENDS ${vasal_determinism_engine}
		${CMAKE_CURRENT_SOURCE_DIR}/determinism/scenario.c
	COMMENT "determinism scenario for WebAssembly"
	VERBATIM)
add_custom_target(determinism_web ALL DEPENDS ${vasal_determinism_web})
add_test(NAME determinism
	COMMAND ${Python3_EXECUTABLE}
		${CMAKE_CURRENT_SOURCE_DIR}/determinism/compare.py
		--native $<TARGET_FILE:determinism> --node ${vasal_node}
		--web ${vasal_determinism_web})
add_dependencies(tests determinism determinism_web)
