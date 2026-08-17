param(
    [switch]$Clean,
    [switch]$Test,
    [string]$ToolRoot
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$build = Join-Path $root 'build\windows'
$demo2Root = Split-Path -Parent $root
$repoRoot = Split-Path -Parent $demo2Root
$workspaceRoot = Split-Path -Parent $repoRoot

if ($Clean -and (Test-Path -LiteralPath $build)) {
    $resolvedBuild = (Resolve-Path -LiteralPath $build).Path
    $resolvedRoot = (Resolve-Path -LiteralPath $root).Path
    if (-not $resolvedBuild.StartsWith($resolvedRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean a path outside the repository: $resolvedBuild"
    }
    Remove-Item -LiteralPath $resolvedBuild -Recurse -Force
}

function Resolve-Tool([string]$BundledPath, [string]$CommandName) {
    if ($BundledPath -and (Test-Path -LiteralPath $BundledPath)) {
        return (Resolve-Path -LiteralPath $BundledPath).Path
    }
    $command = Get-Command $CommandName -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }
    return $null
}

    $toolBundleRoot = $ToolRoot
    if (-not $toolBundleRoot) {
        $toolCandidates = @(
            (Join-Path $root '.tools'),
            (Join-Path $repoRoot 'OKMX8MM-M4-demo1\.tools'),
            (Join-Path $workspaceRoot 'OKMX8MM-M4-demo1\.tools')
        )
        $toolBundleRoot = $toolCandidates | Where-Object {
            Test-Path -LiteralPath (Join-Path $_ 'cmake\bin\cmake.exe')
        } | Select-Object -First 1
        if ($toolBundleRoot) {
            $toolBundleRoot = (Resolve-Path -LiteralPath $toolBundleRoot).Path
        }
    }

    $cmakeBundledPath = $null
    $ninjaBundledPath = $null
    $compilerBundledPath = $null
    if ($toolBundleRoot) {
        $cmakeBundledPath = Join-Path $toolBundleRoot 'cmake\bin\cmake.exe'
        $ninjaBundledPath = Join-Path $toolBundleRoot 'ninja\ninja.exe'
        $compilerBundledPath = Join-Path $toolBundleRoot 'bin\zig-cc.cmd'
    }

$cmake = Resolve-Tool $cmakeBundledPath 'cmake.exe'
$ninja = Resolve-Tool $ninjaBundledPath 'ninja.exe'
    $compiler = Resolve-Tool $compilerBundledPath 'zig-cc.cmd'

if (-not $cmake) {
    throw 'CMake not found. Install CMake or pass -ToolRoot pointing to a tools bundle.'
}
if (-not $ninja) {
    throw 'Ninja not found. Install Ninja or pass -ToolRoot pointing to a tools bundle.'
}

$env:ZIG_GLOBAL_CACHE_DIR = Join-Path $build 'zig-global-cache'
$env:ZIG_LOCAL_CACHE_DIR = Join-Path $build 'zig-local-cache'
New-Item -ItemType Directory -Force -Path $env:ZIG_GLOBAL_CACHE_DIR, $env:ZIG_LOCAL_CACHE_DIR | Out-Null

$configureArgs = @(
    '-S', $root,
    '-B', $build,
    '-G', 'Ninja',
    "-DCMAKE_MAKE_PROGRAM=$ninja",
    '-DCMAKE_BUILD_TYPE=Debug'
)
if ($compiler) {
    $configureArgs += "-DCMAKE_C_COMPILER=$compiler"
}

& $cmake @configureArgs
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& $cmake --build $build
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

if ($Test) {
    & $cmake --build $build --target test
    exit $LASTEXITCODE
}
