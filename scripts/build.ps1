
# Builds a target in the Docker image: cmake --workflow --preset <target>,
# with the configuration in vasal_config. The project folder is mounted
# into the container, so the output appears in build/. Run it from the
# scripts folder.
#
#	.\build.ps1 <target> [debug|release]
#	                     server, tests, check, client or editor; release
#	                     by default
#	.\build.ps1          lists the targets

param(
	[Parameter(Position = 0)][string]$Target = "",
	[Parameter(Position = 1)][ValidateSet("debug", "release")]
	[string]$Config = "release"
)

Push-Location ..
try {
	if ($Target -eq "") {
		docker run --rm --volume "${PWD}:/vasal" vasal-build `
			cmake --workflow --list-presets
	} else {
		docker run --rm --volume "${PWD}:/vasal" `
			--env "vasal_config=$Config" vasal-build `
			cmake --workflow --preset $Target
	}
	$code = $LASTEXITCODE
} finally {
	Pop-Location
}
exit $code
