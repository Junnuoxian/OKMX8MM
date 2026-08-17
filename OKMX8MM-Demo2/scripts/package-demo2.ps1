$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$Parent = Split-Path -Parent $Root
$PackageDir = Join-Path $Root 'packages'
$Stage = Join-Path $PackageDir 'OKMX8MM-Demo2'
$ZipPath = Join-Path $PackageDir 'OKMX8MM-Demo2.zip'

New-Item -ItemType Directory -Force -Path $PackageDir | Out-Null

$ResolvedRoot = (Resolve-Path $Root).Path
$ResolvedStageParent = (Resolve-Path $PackageDir).Path
if (-not $ResolvedStageParent.StartsWith($ResolvedRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw 'package output must stay inside Demo2'
}

if (Test-Path $Stage) {
    Remove-Item -LiteralPath $Stage -Recurse -Force
}
if (Test-Path $ZipPath) {
    Remove-Item -LiteralPath $ZipPath -Force
}

New-Item -ItemType Directory -Force -Path $Stage | Out-Null

robocopy $Root $Stage /MIR /XD `
    build runtime-data packages .tools `
    build-aarch64-probe build-aarch64-probe2 build-aarch64-probe3 build-aarch64-probe4 build-aarch64-probe5 build-aarch64-probe6 `
    /XF .gitignore | Out-Null

$RoboExit = $LASTEXITCODE
if ($RoboExit -ge 8) {
    throw "robocopy failed with code $RoboExit"
}

Compress-Archive -Path (Join-Path $Stage '*') -DestinationPath $ZipPath -Force
Write-Host "Package created: $ZipPath"

