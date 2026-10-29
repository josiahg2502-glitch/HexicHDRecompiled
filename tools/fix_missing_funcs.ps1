# Repeats tools/find_missing_funcs.py (merging into config/hexic.toml) + codegen until nothing new turns up.
# -Image: default.xex's decrypted, decompressed PE image (the basefile the recompiler works from).
param([string]$Image = 'N:\Recomps\HexicHD\image.bin')
$port = 'N:\Recomps\HexicHD\port'
$image = $Image
for ($pass = 1; $pass -le 8; $pass++) {
  python -I N:\Recomps\HexicHD\tools\find_missing_funcs.py $image "$port\generated\default\hexic_init.cpp" "$port\config\hexic.toml"
  $changed = $LASTEXITCODE -eq 2
  Push-Location $port
  cmd /c "N:\Recomps\HexicHD\tools\env.cmd rexglue codegen hexic_manifest.toml" > N:\Recomps\HexicHD\codegen.log 2>&1
  Pop-Location
  Select-String -Path N:\Recomps\HexicHD\codegen.log -Pattern 'Unresolved|Failed|error|Overlapping' | % { "  $($_.Line)" }
  if (!$changed) { "pass ${pass}: converged"; break }
}
