
# Builds a target in the Docker image: cmake --workflow --preset <target>,
# with the configuration in vasal_config. The project folder is mounted
# into the container, so the output appears in build/. Run it from the
# scripts folder.
#
#	bash build.sh <target> [debug|release]
#	                       server, tests, check, client or editor;
#	                       release by default
#	bash build.sh          lists the targets

cd .. || exit 1

# Git Bash would otherwise turn /vasal into a Windows path.
export MSYS_NO_PATHCONV=1

if [ -z "$1" ]; then
	exec docker run --rm --volume "$PWD:/vasal" vasal-build \
		cmake --workflow --list-presets
fi
exec docker run --rm --volume "$PWD:/vasal" \
	--env "vasal_config=${2:-release}" vasal-build \
	cmake --workflow --preset "$1"
