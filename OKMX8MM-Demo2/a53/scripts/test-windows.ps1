$ErrorActionPreference = 'Stop'

$demoRoot = Split-Path -Parent $PSScriptRoot
$repoRoot = Split-Path -Parent $demoRoot
$workspaceRoot = Split-Path -Parent $repoRoot
$toolCandidates = @(
    (Join-Path $repoRoot 'OKMX8MM-M4-demo1\.tools'),
    (Join-Path $workspaceRoot 'OKMX8MM-M4-demo1\.tools'),
    (Join-Path (Split-Path -Parent $workspaceRoot) 'OKMX8MM-M4-demo1\.tools')
)
$toolRoot = $toolCandidates | Where-Object {
    Test-Path -LiteralPath (Join-Path $_ 'cmake\bin\ctest.exe')
} | Select-Object -First 1
if (-not $toolRoot) {
    throw 'M4 test tool package was not found.'
}
$ctest = Join-Path $toolRoot 'cmake\bin\ctest.exe'
$buildScript = Join-Path $PSScriptRoot 'build-windows.ps1'
$buildRoot = Join-Path $demoRoot 'build'

& $buildScript
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& $ctest --test-dir $buildRoot --output-on-failure
exit $LASTEXITCODE
