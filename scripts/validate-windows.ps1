[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Push-Location $root
try {
 $url = 'https://github.com/Tracktion/pluginval/releases/download/v1.0.4/pluginval_Windows.zip'
 Invoke-WebRequest $url -OutFile pluginval.zip
 if ((Get-FileHash pluginval.zip).Hash.ToLower() -ne 'c08e61ce3b96db41636f8ec7e76f4c7e2c13ebdac7fa1b5a1f52b4f32ec715ab') { throw 'pluginval checksum mismatch' }
 Expand-Archive pluginval.zip pluginval -Force
 New-Item -ItemType Directory -Force build-windows/pluginval-logs | Out-Null
 $plugin = (Resolve-Path 'build-windows/CHiMetricMatchPro_artefacts/Release/VST3/CHi Metric Match Pro.vst3').Path
 $logs = (Resolve-Path 'build-windows/pluginval-logs').Path
 # pluginval is a Windows GUI executable. PowerShell's direct invocation may
 # return before it exits; never read a stale/null LASTEXITCODE for this app.
 $info = [Diagnostics.ProcessStartInfo]::new()
 $info.FileName = (Resolve-Path 'pluginval/pluginval.exe').Path
 $info.UseShellExecute = $false
 $info.RedirectStandardOutput = $true
 $info.RedirectStandardError = $true
 $info.CreateNoWindow = $true
 foreach ($argument in @('--strictness-level', '5', '--random-seed', '12345', '--sample-rates', '44100,48000,88200,96000,192000', '--block-sizes', '32,2048', '--output-dir', $logs, '--validate', $plugin)) {
  $info.ArgumentList.Add($argument)
 }
 $process = [Diagnostics.Process]::new()
 $process.StartInfo = $info
 try {
  if (!$process.Start()) { throw 'Cannot start pluginval' }
  $stdout = $process.StandardOutput.ReadToEndAsync()
  $stderr = $process.StandardError.ReadToEndAsync()
  $process.WaitForExit()
  $outputText = $stdout.GetAwaiter().GetResult()
  $errorText = $stderr.GetAwaiter().GetResult()
  $outputText | Set-Content (Join-Path $logs 'stdout.log')
  $errorText | Set-Content (Join-Path $logs 'stderr.log')
  Write-Host $outputText
  if ($errorText) { Write-Host $errorText }
  Write-Host "pluginval process exit code: $($process.ExitCode)"
  if ($process.ExitCode -ne 0) { throw 'pluginval failed; publication blocked' }
  if ($outputText -notmatch '(?m)^SUCCESS\s*$') { throw 'pluginval did not confirm completed validation' }
 } finally { $process.Dispose() }
} finally { Pop-Location }
