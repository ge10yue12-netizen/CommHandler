# Pre-build: ensure ui/MainWindow.ui still contains bound objectNames.
param(
    [string]$UiPath = "",
    [string]$ContractPath = ""
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
if (-not $UiPath) { $UiPath = Join-Path $root "ui\MainWindow.ui" }
if (-not $ContractPath) { $ContractPath = Join-Path $PSScriptRoot "ui_contract.txt" }

if (-not (Test-Path -LiteralPath $UiPath)) {
    Write-Error "UI contract check failed: missing $UiPath"
}
if (-not (Test-Path -LiteralPath $ContractPath)) {
    Write-Error "UI contract check failed: missing $ContractPath"
}

$uiText = [System.IO.File]::ReadAllText($UiPath)
$required = Get-Content -LiteralPath $ContractPath -Encoding UTF8 | ForEach-Object { $_.Trim() } |
    Where-Object { $_ -and -not $_.StartsWith("#") }

$missing = New-Object System.Collections.Generic.List[string]
foreach ($name in $required) {
    $pat = 'name="' + [regex]::Escape($name) + '"'
    if (-not [regex]::IsMatch($uiText, $pat)) {
        [void]$missing.Add($name)
    }
}

$colspanHits = [regex]::Matches($uiText, 'colspan\s*=')
$rowspanHits = [regex]::Matches($uiText, 'rowspan\s*=')
if ($colspanHits.Count -gt 0 -or $rowspanHits.Count -gt 0) {
    Write-Host "UI contract FAILED: MainWindow.ui contains colspan/rowspan (unsafe for Qt Designer). Use QVBoxLayout + standard 2-column QFormLayout instead."
    exit 1
}

if ($missing.Count -gt 0) {
    Write-Host "UI contract FAILED: these objectNames are missing from .ui but still required by C++. Restore same objectName in Designer, or update MainWindow.cpp and tools/ui_contract.txt:"
    foreach ($m in $missing) { Write-Host ("  - " + $m) }
    exit 1
}

Write-Host ("UI contract OK: {0} bound names present; no colspan/rowspan." -f $required.Count)
exit 0
