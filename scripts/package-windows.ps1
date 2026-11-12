[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Push-Location $root
try {
 $bundle = 'build-windows/CHiMetricMatchPro_artefacts/Release/VST3/CHi Metric Match Pro.vst3'
 $relativeBinary = 'CHi Metric Match Pro.vst3/Contents/x86_64-win/CHi Metric Match Pro.vst3'
 $binary = Join-Path (Split-Path $bundle) $relativeBinary
 if (!(Test-Path $binary)) { throw 'Windows VST3 binary missing' }
 $bytes = [IO.File]::ReadAllBytes((Resolve-Path $binary))
 if ($bytes.Length -lt 64 -or $bytes[0] -ne 0x4d -or $bytes[1] -ne 0x5a) { throw 'Not a Windows PE binary' }
 $pe = [BitConverter]::ToInt32($bytes, 0x3c)
 if ($pe -lt 0 -or $pe + 6 -gt $bytes.Length -or [BitConverter]::ToUInt32($bytes, $pe) -ne 0x4550 -or [BitConverter]::ToUInt16($bytes, $pe + 4) -ne 0x8664) { throw 'Not PE x64' }
 $inventory = & './build-windows/CHiTests_artefacts/Release/CHiTests.exe' --inventory
 if ($LASTEXITCODE -ne 0) { throw 'Parameter inventory failed' }
 $parameters = @(($inventory -join [Environment]::NewLine) | ConvertFrom-Json)
 if ($parameters.Count -ne 1 -or $parameters[0].id -ne 'bypass') { throw 'Parameter inventory differs from foundation audit' }
 $commit = (git rev-parse HEAD).Trim()
 if ($LASTEXITCODE -ne 0) { throw 'Cannot identify source commit' }
 New-Item -ItemType Directory -Force dist/staging | Out-Null
 Copy-Item $bundle dist/staging/ -Recurse
 Copy-Item README.md,VALIDATION.md,docs/AUDIT.md dist/staging/
 [ordered]@{
  sourceCommit = $commit
  platform = 'Windows x64'
  configuration = 'Release'
  productStage = 'Foundation only: no reference player, analysis or EQ Match'
  parameters = $parameters
  binarySha256 = (Get-FileHash $binary).Hash.ToLower()
  automatedTests = @('foundation', 'parameter_mapping', 'lifecycle')
  pluginval = '1.0.4, strictness 5; see workflow logs'
 } | ConvertTo-Json -Depth 8 | Set-Content dist/staging/BUILD-INFO.json -Encoding utf8
 $zip = 'dist/CHi-Metric-Match-Pro-Windows-x64.zip'
 Compress-Archive -Path 'dist/staging/*' -DestinationPath $zip
 Expand-Archive -LiteralPath $zip -DestinationPath dist/verify
 if ((Get-FileHash $binary).Hash -ne (Get-FileHash (Join-Path 'dist/verify' $relativeBinary)).Hash) { throw 'ZIP round-trip failed' }
 $hash = (Get-FileHash $zip -Algorithm SHA256).Hash.ToLower()
 "$hash  CHi-Metric-Match-Pro-Windows-x64.zip" | Set-Content dist/SHA256SUMS.txt -Encoding ascii
 Copy-Item dist/staging/BUILD-INFO.json dist/
 Write-Host "Verified Windows x64 ZIP SHA256: $hash"
} finally { Pop-Location }
