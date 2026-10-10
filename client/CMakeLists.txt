
# The game client, for the browser: client.html, client.js and client.wasm
# in build/<preset>/web, where the server serves them from.

file(GLOB vasal_client_sources CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/*.c)
add_executable(client ${vasal_client_sources})
target_link_libraries(client PRIVATE vasal_engine raylib)
vasal_settings(client)
vasal_web_program(client client web OFF
	${CMAKE_CURRENT_SOURCE_DIR}/shell.html)
# The art goes inside the client, so the server serves nothing more.
set(vasal_client_art ${PROJECT_SOURCE_DIR}/art/structures.png)
target_link_options(client PRIVATE
	"SHELL:--embed-file ${vasal_client_art}@/art/structures.png")
set_property(TARGET client APPEND PROPERTY LINK_DEPENDS ${vasal_client_art})
