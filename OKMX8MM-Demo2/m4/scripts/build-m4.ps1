param(
    [string]$SdkRoot = 'D:\Codex_AI\YY_Demo\sdk\SDK_2_16_000_EVK-MIMX8MM',
    [string]$ArmToolchainRoot = '',
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$m4Source = Join-Path $repoRoot 'demo1\m4\armgcc'
$buildRoot = Join-Path $repoRoot 'build\m4'

function Resolve-RequiredCommand([string]$Name) {
    $bundled = $null
    if ($Name -eq 'cmake.exe') {
        $bundled = Join-Path $repoRoot '.tools\cmake\bin\cmake.exe'
    } elseif ($Name -eq 'ninja.exe') {
        $bundled = Join-Path $repoRoot '.tools\ninja\ninja.exe'
    }
    if ($bundled -and (Test-Path -LiteralPath $bundled)) {
        return (Resolve-Path -LiteralPath $bundled).Path
    }

    $command = Get-Command $Name -ErrorAction SilentlyContinue
    if (-not $command) {
        throw "Missing required command: $Name"
    }
    return $command.Source
}

function Resolve-ArmCompiler([string]$ToolchainRoot) {
    if ($ToolchainRoot) {
        $candidate = Join-Path $ToolchainRoot 'bin\arm-none-eabi-gcc.exe'
        if (Test-Path -LiteralPath $candidate) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
        throw "ARM toolchain compiler not found: $candidate"
    }

    $command = Get-Command arm-none-eabi-gcc -ErrorAction SilentlyContinue
    if (-not $command) {
        throw 'Missing arm-none-eabi-gcc. Install Arm GNU Toolchain or pass -ArmToolchainRoot.'
    }
    return $command.Source
}

if (-not (Test-Path -LiteralPath $m4Source)) {
    throw "M4 source directory not found: $m4Source"
}
if (-not (Test-Path -LiteralPath (Join-Path $SdkRoot 'devices\MIMX8MM6\all_lib_device.cmake'))) {
    throw "NXP SDK not found or incomplete: $SdkRoot"
}

$cmake = Resolve-RequiredCommand 'cmake.exe'
$ninja = Resolve-RequiredCommand 'ninja.exe'
$compiler = Resolve-ArmCompiler $ArmToolchainRoot

if ($Clean -and (Test-Path -LiteralPath $buildRoot)) {
    $resolvedBuild = (Resolve-Path -LiteralPath $buildRoot).Path
    $resolvedRepo = (Resolve-Path -LiteralPath $repoRoot).Path
    if (-not $resolvedBuild.StartsWith($resolvedRepo, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to clean a path outside the repository: $resolvedBuild"
    }
    Remove-Item -LiteralPath $resolvedBuild -Recurse -Force
}

$configureArgs = @(
    '-S', $m4Source,
    '-B', $buildRoot,
    '-G', 'Ninja',
    "-DCMAKE_MAKE_PROGRAM=$ninja",
    "-DCMAKE_C_COMPILER=$compiler",
    "-DCMAKE_ASM_COMPILER=$compiler",
    '-DCMAKE_SYSTEM_NAME=Generic',
    '-DCMAKE_SYSTEM_PROCESSOR=arm',
    '-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY',
    "-DSdkRootDirPath=$SdkRoot",
    '-DCMAKE_BUILD_TYPE=Debug'
)

& $cmake @configureArgs
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& $cmake --build $buildRoot
exit $LASTEXITCODE
