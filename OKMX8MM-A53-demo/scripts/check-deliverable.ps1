[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'

$demoRoot = Split-Path -Parent $PSScriptRoot
$errors = [System.Collections.Generic.List[string]]::new()

function Add-CheckError {
    param([Parameter(Mandatory = $true)][string]$Message)
    $errors.Add($Message)
}

function Read-RequiredFile {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    $path = Join-Path $demoRoot $RelativePath
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        Add-CheckError "Missing file: $RelativePath"
        return $null
    }

    $content = Get-Content -LiteralPath $path -Raw -Encoding UTF8
    if ([string]::IsNullOrWhiteSpace($content)) {
        Add-CheckError "Empty file: $RelativePath"
        return $null
    }
    return $content
}

function Require-Literal {
    param(
        [AllowNull()][string]$Content,
        [Parameter(Mandatory = $true)][string]$Literal,
        [Parameter(Mandatory = $true)][string]$RelativePath
    )

    if ($null -ne $Content -and $Content.IndexOf($Literal, [System.StringComparison]::Ordinal) -lt 0) {
        Add-CheckError "Missing required text '$Literal' in $RelativePath"
    }
}

$requiredFiles = @(
    'CMakeLists.txt',
    'README.md',
    'REQUIREMENTS.md',
    'BOARD_RUN.md',
    'include/a53_demo.h',
    'src/main.c',
    'src/pipeline.c',
    'src/storage_writer.c',
    'src/mqtt_outbox.c',
    'src/can_trace.c',
    'src/m4_file_source.c',
    'src/m4_replay_source.c',
    'scripts/build-windows.ps1',
    'scripts/test-windows.ps1',
    'scripts/build-linux.sh',
    'scripts/check-board-env.sh',
    'scripts/package-board.ps1',
    'scripts/check-deliverable.ps1',
    'scripts/publish-mqtt-outbox.sh',
    'scripts/send-can-trace.sh',
    'config/mqtt.env.example',
    'config/can.env.example',
    'docs/reference-migration-map.md',
    'docs/protocol-guide.md',
    'docs/acceptance-checklist.md',
    'examples/m4-input.csv'
)

$contents = @{}
foreach ($relativePath in $requiredFiles) {
    $contents[$relativePath] = Read-RequiredFile $relativePath
}

Require-Literal $contents['README.md'] 'REQUIREMENTS.md' 'README.md'
Require-Literal $contents['REQUIREMENTS.md'] 'M 核：只负责采集' 'REQUIREMENTS.md'
Require-Literal $contents['REQUIREMENTS.md'] 'A53：负责接收数据、保存到 SD 卡、上传 MQTT、发送 CAN' 'REQUIREMENTS.md'
Require-Literal $contents['BOARD_RUN.md'] 'sh scripts/check-board-env.sh' 'BOARD_RUN.md'
Require-Literal $contents['BOARD_RUN.md'] 'sh scripts/publish-mqtt-outbox.sh --env config/mqtt.env' 'BOARD_RUN.md'
Require-Literal $contents['BOARD_RUN.md'] 'sh scripts/send-can-trace.sh --env config/can.env' 'BOARD_RUN.md'
Require-Literal $contents['docs/reference-migration-map.md'] 'modules/storage' 'docs/reference-migration-map.md'
Require-Literal $contents['docs/reference-migration-map.md'] 'modules/mqtt' 'docs/reference-migration-map.md'
Require-Literal $contents['docs/reference-migration-map.md'] 'modules/can' 'docs/reference-migration-map.md'
Require-Literal $contents['docs/protocol-guide.md'] 'sequence,ai0,ai1,ai2,ai3,ai4,ai5,ai6,ai7,ai8,ai9,di_bits,speed_pulse_delta,speed_period_us' 'docs/protocol-guide.md'
Require-Literal $contents['docs/protocol-guide.md'] '存储游标' 'docs/protocol-guide.md'
Require-Literal $contents['docs/acceptance-checklist.md'] 'Windows 模拟验收' 'docs/acceptance-checklist.md'
Require-Literal $contents['docs/acceptance-checklist.md'] '开发板基础验收' 'docs/acceptance-checklist.md'

$zipPath = Join-Path $demoRoot 'packages/OKMX8MM-A53-demo-board.zip'
if (-not (Test-Path -LiteralPath $zipPath -PathType Leaf)) {
    Add-CheckError 'Missing package: packages/OKMX8MM-A53-demo-board.zip'
} else {
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip = [IO.Compression.ZipFile]::OpenRead($zipPath)
    try {
        $entryNames = @($zip.Entries | ForEach-Object { $_.FullName })
        foreach ($expectedEntry in @(
            'OKMX8MM-A53-demo/REQUIREMENTS.md',
            'OKMX8MM-A53-demo/docs/reference-migration-map.md',
            'OKMX8MM-A53-demo/docs/protocol-guide.md',
            'OKMX8MM-A53-demo/docs/acceptance-checklist.md',
            'OKMX8MM-A53-demo/scripts/check-deliverable.ps1',
            'OKMX8MM-A53-demo/scripts/publish-mqtt-outbox.sh',
            'OKMX8MM-A53-demo/scripts/send-can-trace.sh',
            'OKMX8MM-A53-demo/config/mqtt.env.example',
            'OKMX8MM-A53-demo/config/can.env.example'
        )) {
            if ($entryNames -notcontains $expectedEntry) {
                Add-CheckError "Missing package entry: $expectedEntry"
            }
        }

        foreach ($entryName in $entryNames) {
            if ($entryName -match '/(build|runtime-data|packages|_package)/') {
                Add-CheckError "Package contains blocked folder: $entryName"
            }
            if ($entryName -eq 'OKMX8MM-A53-demo/config/mqtt.env' -or
                $entryName -eq 'OKMX8MM-A53-demo/config/can.env') {
                Add-CheckError "Package contains private config: $entryName"
            }
        }
    } finally {
        $zip.Dispose()
    }
}

if ($errors.Count -gt 0) {
    foreach ($checkError in $errors) {
        Write-Error $checkError -ErrorAction Continue
    }
    exit 1
}

Write-Output 'Deliverable check passed: A53 demo files, docs, scripts, config examples, and package are complete.'
