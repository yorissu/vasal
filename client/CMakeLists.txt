
# The game client, for the browser: client.html, client.js and client.wasm
# in build/<preset>/web, where the server serves them from.

file(GLOB vasal_client_sources CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/*.c)
add_executable(client ${vasal_client_sources})
target_link_libraries(client PRIVATE vasal_engine raylib)
vasal_settings(client)
vasal_web_program(client client web OFF
	${CMAKE_CURRENT_SOURCE_DIR}/shell.html)
