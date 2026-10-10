
# Runs the server in the Docker image, serving the client and editor of the
# same configuration at http://localhost:8080 (the editor at /editor).
# Build them first: .\build.ps1 server; .\build.ps1 client;
# .\build.ps1 editor. Every argument after the configuration is passed on
# to the server. Run it from the scripts folder.
#
#	.\run_server.ps1 [debug|release] [arguments...]
#	                 release by default

$config = "release"
$rest = $args
if (($args.Count -gt 0) -and ($args[0] -in "debug", "release")) {
	$config = $args[0]
	$rest = @($args | Select-Object -Skip 1)
}

Push-Location ..
try {
	docker run --rm --volume "${PWD}:/vasal" --publish 8080:8080 `
		vasal-build "build/linux-$config/server/vasal_server" `
		--address 0.0.0.0 --web "build/web-$config/web" @rest
	$code = $LASTEXITCODE
} finally {
	Pop-Location
}
exit $code
