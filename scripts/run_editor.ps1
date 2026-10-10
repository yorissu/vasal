
# Opens the map editor in the default browser, straight from disk; it
# needs no server. Build it first: .\build.ps1 editor. Run it from the
# scripts folder.
#
#	.\run_editor.ps1 [debug|release]
#	                 release by default

param([Parameter(Position = 0)][ValidateSet("debug", "release")]
	[string]$Config = "release")

Start-Process (Resolve-Path "..\build\web-$Config\editor\editor.html")
