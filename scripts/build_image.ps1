
# Builds the Docker image every build runs in (vasal-build), from
# .docker/dockerfile. Run it once, and again after the dockerfile changes.
# Run it from the scripts folder.
#
#	.\build_image.ps1

Push-Location ..
try {
	Get-Content -Raw .docker/dockerfile | docker build --tag vasal-build -
	$code = $LASTEXITCODE
} finally {
	Pop-Location
}
exit $code
