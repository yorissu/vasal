
# settings.cmake: the compiler settings of Vasal's own code.
#
# vasal_settings(<target>) gives a target the project's warnings (all of
# them errors) and, in debug builds for Linux, AddressSanitizer and
# UndefinedBehaviorSanitizer. Third-party code (raylib) keeps its own
# settings.

set(vasal_warnings_common
	-Wall -Wextra -Werror -Wpedantic
	-Wshadow -Wundef -Wfloat-equal -Wdouble-promotion -Wformat=2
	-Wconversion -Wsign-conversion -Wcast-qual
	-Wstrict-prototypes -Wold-style-definition -Wmissing-prototypes
	-Wwrite-strings -Wswitch-enum -Wswitch-default
	-Wnull-dereference -Winit-self -Wvla)

# Only gcc knows these; Emscripten's compiler is clang.
set(vasal_warnings_gcc
	-Wcast-align=strict -Wjump-misses-init -Wduplicated-cond
	-Wduplicated-branches -Wlogical-op -Walloca)

set(vasal_sanitizers -fsanitize=address,undefined -fno-omit-frame-pointer
	-fno-sanitize-recover=all)

function(vasal_settings target)
	target_compile_options(${target} PRIVATE ${vasal_warnings_common}
		$<$<C_COMPILER_ID:GNU>:${vasal_warnings_gcc}>)
	if(NOT EMSCRIPTEN)
		target_compile_options(${target} PRIVATE
			$<$<CONFIG:Debug>:${vasal_sanitizers}>)
		target_link_options(${target} PRIVATE
			$<$<CONFIG:Debug>:${vasal_sanitizers}>)
	endif()
endfunction()
