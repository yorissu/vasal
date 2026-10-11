
# Vasal

A multiplayer medieval strategy game in the browser. Players are vasals:
they join an alliance, command a centaine of five vintaines and a
centenar, and fight for the settlements of a large map.

- `docs/spec.txt`: the specification, the single source of truth for what
  the game does.
- `docs/code_style.md`: how the code is written and checked.

## Building

Everything builds in one Docker image; nothing else needs installing. Run
the scripts from the `scripts` folder (`.ps1` for PowerShell, `.sh` for
bash):

- `build_image`: builds the image, once and after `.docker/dockerfile`
  changes.
- `build <target> [debug|release]`: builds a target into `build/`, in
  release unless debug is given. The targets: `server`, `tests` (builds
  and runs the unit tests), `check` (static analysis, the specification
  and the file rules), `client` and `editor`.
- `run_server [debug|release] [arguments]`: runs the server, which serves
  the client at http://localhost:8080. Every argument after the
  configuration goes to the server.
- `run_editor [debug|release]`: opens the editor in the default
  browser. It is one standalone file
  (`build/web-<config>/editor/editor.html`) and needs no server. It makes
  maps (terrain, settlements, spawn areas, bandit camps, decorations) and
  the rules (unit types, starting army, bandit camp, settlement types,
  combat), keeps drafts in the browser, and uploads both to a server
  started with an admin key, for example:
  `run_server --map /vasal/content/game.vasalmap --rules
  /vasal/content/game.vasalrules --admin-key mykey`. Players get the new
  map and rules when they join or reload. F1 in the editor lists its keys.

Debug builds of the server and tests run with AddressSanitizer and
UndefinedBehaviorSanitizer.
