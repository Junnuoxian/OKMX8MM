param(
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'

$demoRoot = Split-Path -Parent $PSScriptRoot
$repoRoot = Split-Path -Parent $demoRoot
$toolCandidates = @(
    (Join-Path $repoRoot 'OKMX8MM-M4-demo1\.tools'),
    (Join-Path (Split-Path -Parent $repoRoot) 'OKMX8MM-M4-demo1\.tools')
)
$toolRoot = $toolCandidates | Where-Object {
    Test-Path -LiteralPath (Join-Path $_ 'cmake\bin\cmake.exe')
} | Select-Object -First 1
if (-not $toolRoot) {
    throw 'M4 tool package was not found.'
}
$cmake = Join-Path $toolRoot 'cmake\bin\cmake.exe'
$ninja = Join-Path $toolRoot 'ninja\ninja.exe'
$compiler = Join-Path $toolRoot 'bin\zig-cc.cmd'
$buildRoot = Join-Path $demoRoot 'build'

foreach ($tool in @($cmake, $ninja, $compiler)) {
    if (-not (Test-Path -LiteralPath $tool)) {
        throw "Missing tool: $tool"
    }
}

if ($Clean -and (Test-Path -LiteralPath $buildRoot)) {
    $resolvedBuild = (Resolve-Path -LiteralPath $buildRoot).Path
    $resolvedDemo = (Resolve-Path -LiteralPath $demoRoot).Path
    if (-not $resolvedBuild.StartsWith($resolvedDemo, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean unexpected folder: $resolvedBuild"
    }
    Remove-Item -LiteralPath $resolvedBuild -Recurse -Force
}

& $cmake -S $demoRoot -B $buildRoot -G Ninja `
    "-DCMAKE_MAKE_PROGRAM=$ninja" `
    "-DCMAKE_C_COMPILER=$compiler"
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& $cmake --build $buildRoot
exit $LASTEXITCODE
