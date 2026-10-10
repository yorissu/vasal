
# check.cmake: the check target, part of the Linux tree. It runs:
#	check_cppcheck    cppcheck: its warning, style, performance and
#	                  portability checks and its MISRA C:2012 add-on, once
#	                  per program; only the deviations listed in
#	                  .cmake/cppcheck_suppressions.txt are accepted
#	check_clang_tidy  clang-tidy with the checks in .clang-tidy, once per
#	                  program; the client and editor as compiled for the
#	                  browser
#	check_spec        tools/spec.py: the specification, the references to
#	                  it, the tests the current milestone needs, and the
#	                  file rules of every file
#	check_engine      .cmake/check_engine.cmake: the engine includes nothing
#	                  but the C standard library and itself
# Tests and third-party code are not analysed.

find_program(vasal_cppcheck cppcheck REQUIRED)
find_program(vasal_clang_tidy clang-tidy REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(vasal_root ${PROJECT_SOURCE_DIR})
file(GLOB vasal_check_engine CONFIGURE_DEPENDS ${vasal_root}/engine/*.c)
file(GLOB vasal_check_server CONFIGURE_DEPENDS ${vasal_root}/server/*.c)
file(GLOB vasal_check_client CONFIGURE_DEPENDS ${vasal_root}/client/*.c)
file(GLOB vasal_check_editor CONFIGURE_DEPENDS ${vasal_root}/editor/*.c)
set(vasal_check_sysroot $ENV{EMSDK}/upstream/emscripten/cache/sysroot)

set(vasal_cppcheck_options
	--enable=warning,style,performance,portability --addon=misra
	--std=c11 --quiet --inline-suppr --error-exitcode=1
	--check-level=exhaustive --library=posix --library=gnu
	--suppressions-list=${vasal_root}/.cmake/cppcheck_suppressions.txt
	-I${vasal_root})
set(vasal_cppcheck_web -D__EMSCRIPTEN__ -I${vasal_raylib_include}
	-I${vasal_raygui_include} --suppress=*:${vasal_raylib_include}/*
	--suppress=*:${vasal_raygui_include}/*)
add_custom_target(check_cppcheck
	COMMAND ${vasal_cppcheck} ${vasal_cppcheck_options} ${vasal_check_engine}
	COMMAND ${vasal_cppcheck} ${vasal_cppcheck_options}
		-D_POSIX_C_SOURCE=200809L ${vasal_check_server}
	COMMAND ${vasal_cppcheck} ${vasal_cppcheck_options} ${vasal_cppcheck_web}
		${vasal_check_client}
	COMMAND ${vasal_cppcheck} ${vasal_cppcheck_options} ${vasal_cppcheck_web}
		${vasal_check_editor}
	COMMENT "cppcheck with MISRA C:2012"
	VERBATIM)

set(vasal_clang_tidy_web -std=c11 --target=wasm32-unknown-emscripten
	--sysroot=${vasal_check_sysroot} -D__EMSCRIPTEN__ -I${vasal_root}
	-isystem ${vasal_raylib_include} -isystem ${vasal_raygui_include})
add_custom_target(check_clang_tidy
	COMMAND ${vasal_clang_tidy} --quiet ${vasal_check_engine}
		-- -std=c11 -I${vasal_root}
	COMMAND ${vasal_clang_tidy} --quiet ${vasal_check_server}
		-- -std=c11 -D_POSIX_C_SOURCE=200809L -I${vasal_root}
	COMMAND ${vasal_clang_tidy} --quiet ${vasal_check_client}
		-- ${vasal_clang_tidy_web}
	COMMAND ${vasal_clang_tidy} --quiet ${vasal_check_editor}
		-- ${vasal_clang_tidy_web}
	COMMENT "clang-tidy"
	VERBATIM)

# Every text file of the project, for the file rules and the references.
set(vasal_check_files "")
foreach(folder engine server client editor tools tests .cmake docs scripts
		.docker .github)
	file(GLOB_RECURSE found CONFIGURE_DEPENDS ${vasal_root}/${folder}/*)
	list(APPEND vasal_check_files ${found})
endforeach()
foreach(file CMakeLists.txt CMakePresets.json readme.md .clang-tidy
		.editorconfig .gitattributes .gitignore)
	if(EXISTS ${vasal_root}/${file})
		list(APPEND vasal_check_files ${vasal_root}/${file})
	endif()
endforeach()
list(REMOVE_ITEM vasal_check_files ${vasal_root}/docs/spec.txt)
list(FILTER vasal_check_files EXCLUDE REGEX "/__pycache__/")
add_custom_target(check_spec
	COMMAND ${Python3_EXECUTABLE} ${vasal_root}/tools/spec.py
		--spec ${vasal_root}/docs/spec.txt --milestone ${vasal_milestone}
		${vasal_check_files}
	COMMENT "specification and file rules"
	VERBATIM)

add_custom_target(check_engine
	COMMAND ${CMAKE_COMMAND} -Droot=${vasal_root}
		-P ${vasal_root}/.cmake/check_engine.cmake
	COMMENT "engine includes"
	VERBATIM)

add_custom_target(check)
add_dependencies(check check_cppcheck check_clang_tidy check_spec
	check_engine)
