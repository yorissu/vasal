
# web.cmake: settings of the programs built for the browser.
#
# vasal_web_program(<target> <name> <folder> <single> <shell>) makes
# Emscripten write the program, in the page shell given, into
# build/<preset>/<folder>:
#	single OFF  <name>.html, <name>.js and <name>.wasm, as a web server
#	            serves them (the client, from the server)
#	single ON   one <name>.html with everything embedded, which opens
#	            straight from disk with no server (the editor)
#
# The link happens in a folder inside the container, not in the project
# folder: on Docker Desktop for Windows, the linker and llvm-objcopy fail to
# write large files into a folder mounted from Windows (they map the file
# into memory and rename it), while plain copies work. Elsewhere it is just
# one copy more.

function(vasal_web_program target name folder single shell)
	set(link_directory /tmp/vasal_link/${CMAKE_BUILD_TYPE}/${target})
	set(output_directory ${CMAKE_BINARY_DIR}/${folder})

	set_target_properties(${target} PROPERTIES
		OUTPUT_NAME ${name}
		SUFFIX .html
		RUNTIME_OUTPUT_DIRECTORY ${link_directory}
		LINK_DEPENDS ${shell})
	# The map, the world and the path search buffers are static: about
	# 30 MB, more than Emscripten's default start.
	target_link_options(${target} PRIVATE --shell-file ${shell}
		-sINITIAL_MEMORY=67108864)
	set(outputs ${link_directory}/${name}.html)
	if(single)
		target_link_options(${target} PRIVATE -sSINGLE_FILE=1)
	else()
		list(APPEND outputs ${link_directory}/${name}.js
			${link_directory}/${name}.wasm)
	endif()
	add_custom_command(TARGET ${target} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory ${output_directory}
		COMMAND ${CMAKE_COMMAND} -E copy ${outputs} ${output_directory}
		VERBATIM)
endfunction()
