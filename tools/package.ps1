# Assembles a runnable folder: tools\package.ps1 [-Out N:\Projects\HexicHD\dist\HexicHD]
# Game data is never included; the player drops their own XBLA package into game\.
param([string]$Out = 'N:\Projects\HexicHD\dist\HexicHD')

$bin = 'N:\Projects\HexicHD\port\out\build\win-amd64-release'
if (!(Test-Path "$bin\hexic.exe")) { throw "missing $bin\hexic.exe - build first" }

# Replace what we ship; keep the player's game files, settings and logs.
if (Test-Path $Out) { Get-ChildItem $Out -Exclude game, hexic.toml, logs | Remove-Item -Recurse -Force }
New-Item -ItemType Directory -Force "$Out\game" | Out-Null
Copy-Item "$bin\hexic.exe", "$bin\*.dll" $Out
Copy-Item "$bin\shader_cache" $Out -Recurse

@'
Hexic HD Recompiled - Hexic HD (Xbox Live Arcade) for PC

SETUP
  Copy your own Hexic HD package from your Xbox 360 into the "game" folder. It is the
  file named 584107D1... (about 58 MB, found under Content\0000000000000000\584107D1\
  000D0000\ on the console drive); the whole 584107D1 folder works too. The first start
  unpacks it in place. An already unpacked game (default.xex and the .swf/.wma files)
  works as well.

PLAY
  hexic.exe. Controller: A / X rotate, left stick or d-pad moves, Start pauses.

LEGAL
  A fan project, not affiliated with Microsoft. No game data is included.
'@ | Set-Content "$Out\README.txt" -Encoding utf8

$size = (Get-ChildItem $Out -Recurse -File | ? { $_.FullName -notlike "$Out\game\*" } | Measure-Object Length -Sum).Sum
"packaged $Out ($([int]($size / 1MB)) MB)"
