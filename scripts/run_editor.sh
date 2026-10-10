
# Opens the map editor in the default browser, straight from disk; it
# needs no server. Build it first: bash build.sh editor. Run it from the
# scripts folder.
#
#	bash run_editor.sh [debug|release]
#	                   release by default

cd .. || exit 1
exec xdg-open "build/web-${1:-release}/editor/editor.html"
