
# check_engine.cmake: the engine must run the same everywhere, so it
# includes nothing but the C standard library and its own headers: no
# sockets, no Emscripten, no raylib, no other part of the project.
#
#	cmake -Droot=<project folder> -P .cmake/check_engine.cmake

# spec: simulation.engine#1

set(standard_headers assert.h ctype.h errno.h float.h inttypes.h iso646.h
	limits.h stdalign.h stdarg.h stdbool.h stddef.h stdint.h stdnoreturn.h
	string.h)

file(GLOB files ${root}/engine/*.c ${root}/engine/*.h)
set(failed FALSE)
foreach(file ${files})
	file(STRINGS ${file} includes REGEX "^[ \t]*#[ \t]*include")
	foreach(include ${includes})
		string(REGEX MATCH "[<\"]([^>\"]+)[>\"]" found "${include}")
		set(header ${CMAKE_MATCH_1})
		if(include MATCHES "\"engine/")
			continue()
		endif()
		list(FIND standard_headers "${header}" index)
		if(index EQUAL -1 OR include MATCHES "\"")
			message(SEND_ERROR "${file}: the engine may not include ${header}")
			set(failed TRUE)
		endif()
	endforeach()
endforeach()
if(failed)
	message(FATAL_ERROR "the engine includes headers it may not")
endif()
