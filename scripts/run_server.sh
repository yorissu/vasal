
# Runs the server in the Docker image, serving the client and editor of the
# same configuration at http://localhost:8080 (the editor at /editor).
# Build them first: bash build.sh server; bash build.sh client;
# bash build.sh editor. Every argument after the configuration is passed on
# to the server. Run it from the scripts folder.
#
#	bash run_server.sh [debug|release] [arguments...]
#	                   release by default

cd .. || exit 1

# Git Bash would otherwise turn /vasal into a Windows path.
export MSYS_NO_PATHCONV=1

config=release
if [ "$1" = debug ] || [ "$1" = release ]; then
	config=$1
	shift
fi
exec docker run --rm --volume "$PWD:/vasal" --publish 8080:8080 \
	vasal-build "build/linux-$config/server/vasal_server" \
	--address 0.0.0.0 --web "build/web-$config/web" "$@"
