$ErrorActionPreference = 'Stop'

$demoRoot = Split-Path -Parent $PSScriptRoot
$packageRoot = Join-Path $demoRoot '_package'
$packageName = 'OKMX8MM-A53-demo'
$stagingRoot = Join-Path $packageRoot $packageName
$outputRoot = Join-Path $demoRoot 'packages'
$zipPath = Join-Path $outputRoot "$packageName-board.zip"

function Remove-SafeFolder([string]$Target, [string]$AllowedRoot) {
    if (-not (Test-Path -LiteralPath $Target)) {
        return
    }

    $resolvedTarget = (Resolve-Path -LiteralPath $Target).Path
    $resolvedAllowed = (Resolve-Path -LiteralPath $AllowedRoot).Path
    if (-not $resolvedTarget.StartsWith($resolvedAllowed, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove unexpected folder: $resolvedTarget"
    }

    Remove-Item -LiteralPath $resolvedTarget -Recurse -Force
}

Remove-SafeFolder $packageRoot $demoRoot
New-Item -ItemType Directory -Force $stagingRoot, $outputRoot | Out-Null

$items = @(
    'CMakeLists.txt',
    'README.md',
    'REQUIREMENTS.md',
    'BOARD_RUN.md',
    '.gitignore',
    'include',
    'src',
    'scripts',
    'config',
    'docs',
    'examples',
    'tests'
)

foreach ($item in $items) {
    $source = Join-Path $demoRoot $item
    $target = Join-Path $stagingRoot $item
    if (-not (Test-Path -LiteralPath $source)) {
        throw "Missing package item: $item"
    }
    Copy-Item -LiteralPath $source -Destination $target -Recurse -Force
}

if (Test-Path -LiteralPath $zipPath) {
    Remove-Item -LiteralPath $zipPath -Force
}

Compress-Archive -LiteralPath $stagingRoot -DestinationPath $zipPath -Force
Remove-SafeFolder $packageRoot $demoRoot

Write-Output "Created: $zipPath"
