[CmdletBinding()]
param([ValidateSet('Debug','Release')][string]$Configuration = 'Release', [switch]$Install)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Push-Location $root
try {
 New-Item -ItemType Directory -Force build-windows | Out-Null
 Start-Transcript -Path "build-windows/build-$Configuration.log" -Force
 try {
  cmake -S . -B build-windows -G 'Visual Studio 17 2022' -A x64 -DBUILD_TESTING=ON
  if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed' }
  cmake --build build-windows --config $Configuration --parallel 2
  if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
  ctest --test-dir build-windows -C $Configuration --output-on-failure
  if ($LASTEXITCODE -ne 0) { throw 'Tests failed' }
  $plugin = Join-Path $root "build-windows/CHiMetricMatchPro_artefacts/$Configuration/VST3/CHi Metric Match Pro.vst3"
  if (!(Test-Path $plugin)) { throw 'VST3 bundle missing' }
  Write-Host "Built: $plugin"
  if ($Install) {
   $target = 'C:\Program Files\Common Files\VST3\CHi Audio\CHi Metric Match Pro.vst3'
   if (Test-Path $target) { throw 'Existing plugin will not be overwritten' }
   if (!(Get-ChildItem $plugin -Recurse -Filter '*.vst3' -File)) { throw 'VST3 binary missing' }
   if ($PSCmdlet.ShouldContinue('Install the tested VST3 in Common Files?', 'Authorize installation')) {
    New-Item -ItemType Directory -Force (Split-Path $target) | Out-Null
    Copy-Item $plugin $target -Recurse
   }
  }
 } finally { Stop-Transcript }
} finally { Pop-Location }
