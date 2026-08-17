param(
    [switch]$SkipTests,
    [switch]$SkipM4Tests
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot

$Required = @(
    'README.md',
    'm4\demo1\README.md',
    'm4\demo1\src\demo1_modbus.c',
    'm4\demo1\src\demo1_wheel_board_source.c',
    'a53\README.md',
    'a53\src\pipeline.c',
    'a53\src\heartbeat_writer.c',
    'a53\scripts\check-ota-readiness.sh',
    'docs\stm32-to-okmx8mm.md',
    'docs\newcomer-runbook.txt',
    'docs\acceptance-checklist.txt'
)

foreach ($Item in $Required) {
    $Full = Join-Path $Root $Item
    if (-not (Test-Path $Full)) {
        throw "missing required file: $Item"
    }
}

$ForbiddenHits = Select-String -Path `
    (Join-Path $Root 'README.md'), `
    (Join-Path $Root 'docs\*.md'), `
    (Join-Path $Root 'docs\*.txt') `
    -Pattern '地址' -SimpleMatch -ErrorAction SilentlyContinue

if ($ForbiddenHits) {
    $ForbiddenHits | ForEach-Object { Write-Error "$($_.Path):$($_.LineNumber): forbidden word found" }
    throw 'forbidden word check failed'
}

if (-not $SkipTests) {
    Push-Location (Join-Path $Root 'a53')
    try {
        & .\scripts\test-windows.ps1
    } finally {
        Pop-Location
    }

    if (-not $SkipM4Tests) {
        Push-Location (Join-Path $Root 'm4')
        try {
            & .\scripts\build-windows.ps1 -Test
        } finally {
            Pop-Location
        }
    }
}

Write-Host 'Demo2 self-check passed.'
