
# The match server, Linux only. Everything but main.c is a library, so the
# tests can use the modules.

add_library(vasal_server_modules STATIC
	files.c
	http.c
	log.c
	network.c
	options.c
	response.c
	signals.c)
target_link_libraries(vasal_server_modules PUBLIC vasal_engine)
target_compile_definitions(vasal_server_modules PUBLIC
	_POSIX_C_SOURCE=200809L)
vasal_settings(vasal_server_modules)

add_executable(server main.c)
set_target_properties(server PROPERTIES OUTPUT_NAME vasal_server)
target_link_libraries(server PRIVATE vasal_server_modules)
vasal_settings(server)
