
# Vasal code style

How Vasal's code is written: C for the engine, server, client and editor,
Python for the tools. The `check` target enforces what can be checked.

## Every file

- LF line endings only, ASCII only.
- Every file starts with one empty line and ends with exactly one newline.
  A file with no content stays completely empty.
- Indentation with tabs, 4 columns wide. YAML is the exception: it forbids
  tabs.
- Lines are at most 80 columns, with very rare exceptions.
- No trailing spaces or tabs.

## C

### Language and rules

- C11, no C++.
- No dynamic memory (`malloc`, `free`, variable-length arrays). Memory lives
  in pools sized by hard limits.
- Target: MISRA C:2012. Deviations are listed at the end of this file, each
  with a reason.

### Files

- File names are lowercase; words are separated by `_` (`network.c`).
- A wrapped line that aligns with something above it uses tabs as far as
  they reach, then spaces. When that doesn't fit in 80 columns, the
  arguments go on their own lines, one tab deeper than the statement.
- Alignment after code on the same line (aligned columns, macro
  continuation backslashes) uses spaces.
- Header guards are classic guards (no `#pragma once`), named after the
  file's path with `__` between the parts:

```c
#ifndef vasal__server__http_h
#define vasal__server__http_h

#endif
```

### Names

- Everything is lowercase: functions, types, macros, constants, enum
  values. Words and namespaces are separated by `_`.
- Every function, type and macro starts with the prefix `vasal`, then its
  folders as namespaces, then the module (file) name, then its own name:
  `server/http.c` reading a request: `vasal_server_http_parse`.
- Names are written out in full: `network`, not `net`; `initialize`, not
  `init`. The only short forms are standard acronyms (`http`, `tcp`, `id`,
  `ipv4`), unit symbols (`ms`, `hz`), mathematical symbols in mathematical
  code (`x`, `y`, `dt`) and loop counters (`i`, `j`).
- Folder names are the namespaces, so a path, its guard and its names
  always match: `server/http.h`, `vasal__server__http_h`,
  `vasal_server_http_parse`.
- Private names, those of one `.c` file only (`static` functions and
  variables, and the types and macros defined in the `.c` file), start
  with an underscore and otherwise follow the same rule, module included:
  `_vasal_server_network_accept`, `_vasal_client_main_update`. A function
  in a header is public by definition, so a private one is always
  `static` and only in its `.c` file.
- Type suffixes: `_s` struct, `_e` enum, `_f` function pointer, `_u` union,
  `_t` anything else.
- Standard C types (`uint8_t`, `size_t`) are used as they are. The
  project-wide standard typedefs carry no prefix: `bool_t` (in
  `engine/types.h`).
- Tests are named `vasal_tests_<folder>_<module>_...`.

### Code

- `const` on everything that is not modified: value parameters, pointers
  and what they point to, locals.
- Pointers are written with the `*` on the type: `uint8_t* const data`.
- Braces always, also around a single statement.
- K&R braces: the opening brace stays on the line of the statement or
  function it opens.
- Comparisons with a constant put the constant first: `NULL == pointer`.
- Every return value that is not used is cast to `(void)`.
- No project-wide error type. A function returns `bool_t`, or an enum of
  its own module that names the outcomes.
- Early returns are allowed.
- `goto` is allowed only forward, to a label in the same function.
- `sizeof` takes parentheses: `sizeof(buffer)`.
- Includes come in three groups, separated by one empty line: our own
  headers (in quotes, from the project's root: `"server/http.h"`), then
  other libraries' headers (POSIX and Linux system headers, Emscripten,
  raylib, cairo), then the C standard library. A `.c` file includes its
  own header first.

### Comments

- API documentation of types, functions and macros, in headers, uses `///`:
  normal sentences saying what the function does, how, and why. The
  signature already says what it takes.
- Every other comment uses `//`, also in normal sentences.
- Every file opens, after its empty first line, with a `///` block saying
  what the file is for.
- No Doxygen tags.
- Code that implements a story or criterion of the specification names it
  in a comment above it: `// spec: server.web#1`. Every test does the same
  for the criteria it checks.

### Layers

- `engine/`: the code every program shares. It includes only the C
  standard library and itself (checked by `.cmake/check_engine.cmake`).
- `server/`, `client/`, `editor/`: one program each. They may use the
  engine, never each other.
- `tests/`: unit tests, one folder per program.

## Python

Python is used for the tools in `tools/` and their tests.

- Indentation with tabs, like every other file.
- Names follow PEP 8: `lowercase_with_underscores` for functions and
  variables, `CapWords` for classes, `UPPERCASE` for module constants.
- Documentation is written as `#` comments, never docstrings. Every file
  opens, after its empty first line, with a header block framed by lone `#`
  lines: the file's name, what it does, what it is for.
- Non-obvious functions get a comment block directly above the `def`.
- Only the standard library.
- Tests use `unittest` and name the criteria they check like C tests:
  `# spec: tools.check#1` above the test method.

## Checks

The `check` target runs, and all must pass:

- cppcheck: its warning, style, performance and portability checks and its
  MISRA C:2012 add-on, once per program. Only the deviations below are
  accepted (`.cmake/cppcheck_suppressions.txt`).
- clang-tidy: the bugprone, CERT, clang-analyzer, misc, performance,
  portability and readability checks, except the few that contradict this
  style (`.clang-tidy` says which and why).
- `tools/spec.py`: the specification, the references to it, the tests the
  current milestone needs, and the file rules above.
- The engine's include rule.

## MISRA C:2012 deviations

Advisory rules, everywhere:

- 2.3, 2.4, 2.5: unused types, tags and macros. API headers define complete
  sets.
- 8.7: external linkage for functions one other file uses. Module
  interfaces stay interfaces as the code grows.
- 8.9: tables and buffers at file scope, where they are easier to find.
- 15.1: `goto`, forward only. One exit point keeps cleanup in one place.
- 15.5: early returns. Guard clauses read more clearly than nesting.

Required rules, only where named:

- 21.6 in `server/log.c`: stdio. The log writes formatted lines to
  standard output.
- 21.6 in `server/files.c`: `rename` from stdio. An upload replaces the
  map or rules file atomically: written beside it, then renamed over it.
- 21.5 in `server/signals.c`: `signal.h`. The server stops cleanly on
  SIGINT and SIGTERM, through signalfd.
- 11.3 in `server/network.c`: casts to `struct sockaddr*`. The sockets API
  takes them.
- 21.10 in `server/network.c`: `time.h`, for the monotonic clock
  (`clock_gettime`) only; no date or time of day.
- 21.1, 21.2 everywhere: private names start with an underscore. The C
  standard reserves such names at file scope; private names keep the
  project's prefix, so they can't meet the C library's.
- 11.5 (advisory) in `client/main.c`: Emscripten hands downloaded data to
  its callback as a `void*`.
- 22.10 in `server/network.c`: `errno` tests after POSIX calls. They set
  errno; the add-on only knows the C library's errno-setting functions.
