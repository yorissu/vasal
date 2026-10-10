
# The engine: the code every program shares (simulation, map format,
# protocol). It depends on the C standard library only, so it runs the same
# on the server, in the browser and in tests.

file(GLOB vasal_engine_sources CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/*.c)
add_library(vasal_engine STATIC ${vasal_engine_sources})
target_include_directories(vasal_engine PUBLIC ${PROJECT_SOURCE_DIR})
vasal_settings(vasal_engine)
