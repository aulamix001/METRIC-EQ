[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Push-Location $root
try {
 $bundle = 'build-windows/CHiMetricMatchPro_artefacts/Release/VST3/CHi Metric Match Pro.vst3'
 $binary = Join-Path $bundle 'Contents/x86_64-win/CHi Metric Match Pro.vst3'
 $legacy = 'build-windows/CHiMetricMatchPro_artefacts/Release/VST2/CHi Metric Match Pro.dll'
 function Assert-Pe64([string]$path) {
  $bytes = [IO.File]::ReadAllBytes((Resolve-Path $path))
  if ($bytes.Length -lt 64 -or $bytes[0] -ne 0x4d -or $bytes[1] -ne 0x5a) { throw "Not a Windows PE binary: $path" }
  $pe = [BitConverter]::ToInt32($bytes, 0x3c)
  if ($pe -lt 0 -or $pe + 6 -gt $bytes.Length -or [BitConverter]::ToUInt32($bytes, $pe) -ne 0x4550 -or [BitConverter]::ToUInt16($bytes, $pe + 4) -ne 0x8664) { throw "Not PE x64: $path" }
 }
 Assert-Pe64 $binary
 Assert-Pe64 $legacy
 $inventory = & './build-windows/CHiTests_artefacts/Release/CHiTests.exe' --inventory
 if ($LASTEXITCODE -ne 0) { throw 'Parameter inventory failed' }
 $parameters = @(($inventory -join [Environment]::NewLine) | ConvertFrom-Json)
 if ($parameters.Count -ne 37 -or !($parameters | Where-Object id -eq 'ab') -or !($parameters | Where-Object id -eq 'gainA')) { throw 'Unexpected parameter inventory' }
 $commit = (git rev-parse HEAD).Trim()
 if ($LASTEXITCODE -ne 0) { throw 'Cannot identify source commit' }
 New-Item -ItemType Directory -Force dist/staging/VST2 | Out-Null
 Copy-Item $bundle dist/staging/ -Recurse
 Copy-Item $legacy dist/staging/VST2/
 Copy-Item README.md,VALIDATION.md dist/staging/
 Copy-Item docs dist/staging/ -Recurse
 Copy-Item ThirdParty dist/staging/ -Recurse
 Copy-Item ThirdParty/LegacyABI/LICENSE dist/staging/LegacyABI-LICENSE.txt
 [ordered]@{
  version = '0.2.0'
  sourceCommit = $commit
  platform = 'Windows x64'
  configuration = 'Release'
  productStage = 'Reference comparison: real editor, A/B RAM player, FFT, filters, monitors; advanced mastering features pending'
  parameters = $parameters
  binarySha256 = (Get-FileHash $binary).Hash.ToLower()
  vst2Sha256 = (Get-FileHash $legacy).Hash.ToLower()
  automatedTests = @('foundation','parameter_mapping','lifecycle','reference_playback','monitor_filters','spectrum_calibration')
  pluginval = '1.0.4 strictness 5, VST3 and VST2-compatible adapter'
 } | ConvertTo-Json -Depth 8 | Set-Content dist/staging/BUILD-INFO.json -Encoding utf8
 $combined = 'dist/CHi-Metric-Match-Pro-Windows-x64.zip'
 Compress-Archive -Path 'dist/staging/*' -DestinationPath $combined
 Compress-Archive -Path "dist/staging/CHi Metric Match Pro.vst3",dist/staging/docs,dist/staging/ThirdParty,dist/staging/README.md,dist/staging/BUILD-INFO.json -DestinationPath dist/CHi-Metric-Match-Pro-VST3-Windows-x64.zip
 Compress-Archive -Path "dist/staging/VST2/CHi Metric Match Pro.dll",dist/staging/docs,dist/staging/ThirdParty,dist/staging/README.md,dist/staging/LegacyABI-LICENSE.txt,dist/staging/BUILD-INFO.json -DestinationPath dist/CHi-Metric-Match-Pro-VST2-Windows-x64.zip
 Expand-Archive $combined dist/verify
 if ((Get-FileHash $binary).Hash -ne (Get-FileHash 'dist/verify/CHi Metric Match Pro.vst3/Contents/x86_64-win/CHi Metric Match Pro.vst3').Hash) { throw 'VST3 ZIP round-trip failed' }
 if ((Get-FileHash $legacy).Hash -ne (Get-FileHash 'dist/verify/VST2/CHi Metric Match Pro.dll').Hash) { throw 'VST2 ZIP round-trip failed' }
 $lines = foreach($file in Get-ChildItem dist/*.zip) {"$((Get-FileHash $file).Hash.ToLower())  $($file.Name)"}
 $lines | Set-Content dist/SHA256SUMS.txt -Encoding ascii
 Copy-Item dist/staging/BUILD-INFO.json dist/
} finally { Pop-Location }
