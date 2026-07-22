@echo off
chcp 65001 >nul
setlocal EnableExtensions
cd /d "%~dp0"

rem =============================================================================
rem  QtExeTool.bat  -  ONE file, no other scripts required
rem  For ANY Qt + Visual Studio Windows exe (Debug-only / Release-only / both)
rem
rem  What it does:
rem    1 FIX  - copy correct Qt runtime next to the exe (double-click works)
rem    2 PACK - clean portable folder + zip under QtExePack\ (next to this bat)
rem    3 ALL  - FIX then PACK
rem
rem  How to use:
rem    - Double-click this bat, then enter/drag the exe path
rem    - Or drag YourApp.exe onto this bat
rem    - Or: QtExeTool.bat "C:\path\YourApp.exe" all
rem    - Optional 3rd arg: nopause   (for CI / MSBuild PostBuild)
rem
rem  Debug/Release is detected from the EXE import table (not folder name).
rem  PACK output:  <batDir>\QtExePack\<App>_<debug|release>\  and .zip
rem =============================================================================

set "P1=%~1"
set "P2=%~2"
set "P3=%~3"
set "P4=%~f0"
set "TMPPS=%TEMP%\QtExeTool_embed.ps1"

powershell -NoProfile -ExecutionPolicy Bypass -Command "& { $ErrorActionPreference='Stop'; $b='%~f0'; $r=[IO.File]::ReadAllText($b); $m='___EMBEDDED_POWERSHELL_V1___'; $i=$r.LastIndexOf($m); if ($i -lt 0) { throw 'embedded marker missing' }; [IO.File]::WriteAllText($env:TEMP + '\QtExeTool_embed.ps1', $r.Substring($i + $m.Length), (New-Object System.Text.UTF8Encoding $false)) }"
if errorlevel 1 (
  echo [FAILED] extract embedded script
  if /I not "%P3%"=="nopause" if /I not "%P2%"=="nopause" pause
  exit /b 1
)

powershell -NoProfile -ExecutionPolicy Bypass -File "%TMPPS%" -ExePath "%P1%" -Mode "%P2%" -NoPause "%P3%" -BatPath "%P4%"
set "ERR=%ERRORLEVEL%"
del "%TMPPS%" >nul 2>&1
echo.
if /I not "%P3%"=="nopause" if /I not "%P2%"=="nopause" (
  if not "%ERR%"=="0" echo [FAILED] exit=%ERR%
  pause
)
exit /b %ERR%

___EMBEDDED_POWERSHELL_V1___
param(
    [string]$ExePath = "",
    [string]$Mode = "",
    [string]$NoPause = "",
    [string]$BatPath = ""
)

$ErrorActionPreference = "Stop"
# Pack root folder name (next to this bat)
$script:PackFolderName = "QtExePack"

function Write-Step([string]$m) { Write-Host ("[QtExeTool] " + $m) }

function Test-QtBin([string]$bin) {
    if ([string]::IsNullOrWhiteSpace($bin)) { return $false }
    $wdOk = (Test-Path (Join-Path $bin "windeployqt.exe")) -or (Test-Path (Join-Path $bin "windeployqt6.exe"))
    $qmOk = (Test-Path (Join-Path $bin "qmake.exe")) -or (Test-Path (Join-Path $bin "qmake6.exe"))
    return ($wdOk -and $qmOk)
}

function Resolve-QtBin {
    $candidates = New-Object System.Collections.Generic.List[string]
    foreach ($key in @("QTDIR", "QT_DIR", "Qt5_DIR", "Qt6_DIR", "CMAKE_PREFIX_PATH")) {
        $v = [Environment]::GetEnvironmentVariable($key)
        if ([string]::IsNullOrWhiteSpace($v)) { continue }
        foreach ($part in ($v -split ";")) {
            $p = $part.Trim().TrimEnd('\', '/')
            if (-not $p) { continue }
            [void]$candidates.Add((Join-Path $p "bin"))
            [void]$candidates.Add($p)
            $parent = Split-Path -Parent $p
            if ($parent) { [void]$candidates.Add((Join-Path $parent "bin")) }
        }
    }
    foreach ($name in @("qmake.exe", "qmake6.exe", "windeployqt.exe", "windeployqt6.exe")) {
        $cmd = Get-Command $name -ErrorAction SilentlyContinue
        if ($cmd -and $cmd.Source) { [void]$candidates.Add((Split-Path -Parent $cmd.Source)) }
    }
    $kits = @("msvc2022_64", "msvc2019_64", "msvc2017_64", "mingw_64")
    foreach ($drive in @([IO.DriveInfo]::GetDrives() | Where-Object { $_.IsReady -and $_.DriveType -eq "Fixed" } | ForEach-Object { $_.Name.TrimEnd('\') })) {
        $qtRoot = Join-Path $drive "Qt"
        if (-not (Test-Path -LiteralPath $qtRoot)) { continue }
        Get-ChildItem -LiteralPath $qtRoot -Directory -ErrorAction SilentlyContinue | ForEach-Object {
            foreach ($k in $kits) {
                [void]$candidates.Add((Join-Path $_.FullName (Join-Path $k "bin")))
            }
        }
    }
    foreach ($c in $candidates) { if (Test-QtBin $c) { return $c } }
    foreach ($c in $candidates) {
        foreach ($qmName in @("qmake.exe", "qmake6.exe")) {
            $qmake = Join-Path $c $qmName
            if (-not (Test-Path -LiteralPath $qmake)) { continue }
            try {
                $bins = (& $qmake -query QT_INSTALL_BINS 2>$null)
                if ($bins) {
                    $b = $bins.ToString().Trim()
                    if (Test-QtBin $b) { return $b }
                }
            } catch {}
        }
    }
    return $null
}

# Read PE import DLL names from a native exe (no dumpbin required).
function Get-PeImportNames([string]$path) {
    $fs = [IO.File]::OpenRead($path)
    $br = New-Object IO.BinaryReader($fs)
    try {
        $fs.Seek(0x3C, "Begin") | Out-Null
        $pe = $br.ReadInt32()
        $fs.Seek($pe, "Begin") | Out-Null
        if ($br.ReadUInt32() -ne 0x4550) { throw "not a PE file" }
        $fs.Seek($pe + 4, "Begin") | Out-Null
        $null = $br.ReadUInt16()
        $numSections = $br.ReadUInt16()
        $fs.Seek(12, "Current") | Out-Null
        $sizeOpt = $br.ReadUInt16()
        $null = $br.ReadUInt16()
        $optStart = $fs.Position
        $magic = $br.ReadUInt16()
        $isPE32Plus = ($magic -eq 0x20B)
        if (-not $isPE32Plus -and $magic -ne 0x10B) { throw "unsupported PE optional header" }
        $ddOff = if ($isPE32Plus) { 112 } else { 96 }
        $fs.Seek($optStart + $ddOff + 8, "Begin") | Out-Null
        $importRva = $br.ReadUInt32()
        $null = $br.ReadUInt32()
        if ($importRva -eq 0) { return @() }

        $sectStart = $optStart + $sizeOpt
        $sections = @()
        for ($i = 0; $i -lt $numSections; $i++) {
            $fs.Seek($sectStart + $i * 40, "Begin") | Out-Null
            $null = $br.ReadBytes(8)
            $vs = $br.ReadUInt32(); $va = $br.ReadUInt32(); $rs = $br.ReadUInt32(); $ptr = $br.ReadUInt32()
            $sections += [pscustomobject]@{ VA = $va; VS = $vs; Ptr = $ptr; RS = $rs }
        }
        function Convert-RvaToOffset([uint32]$rva) {
            foreach ($s in $sections) {
                $span = [Math]::Max($s.VS, $s.RS)
                if ($rva -ge $s.VA -and $rva -lt ($s.VA + $span)) {
                    return [int64]($s.Ptr + ($rva - $s.VA))
                }
            }
            return -1
        }

        $names = New-Object System.Collections.Generic.List[string]
        $off = Convert-RvaToOffset $importRva
        if ($off -lt 0) { return @() }
        while ($true) {
            $fs.Seek($off, "Begin") | Out-Null
            $ilt = $br.ReadUInt32(); $null = $br.ReadUInt32(); $null = $br.ReadUInt32()
            $nameRva = $br.ReadUInt32(); $ft = $br.ReadUInt32()
            if ($ilt -eq 0 -and $nameRva -eq 0 -and $ft -eq 0) { break }
            $no = Convert-RvaToOffset $nameRva
            if ($no -ge 0) {
                $fs.Seek($no, "Begin") | Out-Null
                $bytes = New-Object System.Collections.Generic.List[byte]
                while (($b = $br.ReadByte()) -ne 0) { [void]$bytes.Add($b) }
                [void]$names.Add([Text.Encoding]::ASCII.GetString($bytes.ToArray()))
            }
            $off += 20
        }
        return $names.ToArray()
    } finally {
        $br.Close()
        $fs.Close()
    }
}

function Get-QtImports([string]$exe) {
    return @(Get-PeImportNames $exe | Where-Object { $_ -match '(?i)^Qt[56].+\.dll$' })
}

# Decide windeployqt mode from EXE imports (authoritative), not from folder name.
function Get-WindeployModeFromExe([string]$exe) {
    $qt = @(Get-QtImports $exe)
    if ($qt.Count -eq 0) {
        Write-Step "WARNING: no Qt DLL in exe imports; defaulting to release"
        return "release"
    }
    $debugHits = @($qt | Where-Object { $_ -match '(?i)d\.dll$' })
    $releaseHits = @($qt | Where-Object { $_ -notmatch '(?i)d\.dll$' })
    Write-Step ("exe Qt imports: " + ($qt -join ", "))
    if ($debugHits.Count -gt 0 -and $releaseHits.Count -gt 0) {
        Write-Step "WARNING: mixed Qt debug/release imports; preferring debug"
        return "debug"
    }
    if ($debugHits.Count -gt 0) { return "debug" }
    return "release"
}

function Test-NeedsGuiPlatform([string]$exe) {
    $qt = @(Get-QtImports $exe)
    if ($qt.Count -eq 0) { return $true }
    return [bool]($qt | Where-Object { $_ -match '(?i)Gui|Widgets' })
}

function Remove-ConflictingQtRuntime([string]$exeDir, [string]$mode) {
    $wantDebug = ($mode -eq "debug")
    Get-ChildItem -LiteralPath $exeDir -File -ErrorAction SilentlyContinue | ForEach-Object {
        $n = $_.Name
        if ($n -notmatch '(?i)^Qt[56].+\.dll$' -and $n -notmatch '(?i)^(libEGL|libGLESv2)d?\.dll$') { return }
        $isDebugDll = ($n -match '(?i)d\.dll$')
        if ($wantDebug -and -not $isDebugDll) {
            Remove-Item -LiteralPath $_.FullName -Force
            Write-Step ("removed wrong release DLL: " + $n)
        }
        if (-not $wantDebug -and $isDebugDll) {
            Remove-Item -LiteralPath $_.FullName -Force
            Write-Step ("removed wrong debug DLL: " + $n)
        }
    }

    $pluginDirs = @("platforms", "styles", "imageformats", "iconengines", "bearer", "tls", "networkinformation", "sqldrivers", "generic")
    foreach ($pd in $pluginDirs) {
        $dir = Join-Path $exeDir $pd
        if (-not (Test-Path -LiteralPath $dir)) { continue }
        Get-ChildItem -LiteralPath $dir -File -Filter "*.dll" -ErrorAction SilentlyContinue | ForEach-Object {
            $n = $_.Name
            $isDebugPlugin = ($n -match '(?i)d\.dll$')
            if ($wantDebug -and -not $isDebugPlugin) {
                Remove-Item -LiteralPath $_.FullName -Force
                Write-Step ("removed wrong release plugin: " + $pd + "\" + $n)
            }
            if (-not $wantDebug -and $isDebugPlugin) {
                Remove-Item -LiteralPath $_.FullName -Force
                Write-Step ("removed wrong debug plugin: " + $pd + "\" + $n)
            }
        }
    }
}

function Get-RequiredQtDlls([string]$exe, [string]$mode) {
    $imports = @(Get-QtImports $exe)
    if ($imports.Count -gt 0) { return $imports }
    if ($mode -eq "debug") {
        return @("Qt5Cored.dll", "Qt5Guid.dll", "Qt5Widgetsd.dll")
    }
    return @("Qt5Core.dll", "Qt5Gui.dll", "Qt5Widgets.dll")
}

function Assert-KitHasMode([string]$qtBin, [string]$mode) {
    if ($mode -eq "debug") {
        $probe = @("Qt5Cored.dll", "Qt6Cored.dll") | ForEach-Object { Join-Path $qtBin $_ }
        if (-not ($probe | Where-Object { Test-Path $_ })) {
            throw ("Qt kit has no Debug DLLs (*d.dll) under: " + $qtBin + "  Install Qt debug libraries, or build/use a Release exe.")
        }
    } else {
        $probe = @("Qt5Core.dll", "Qt6Core.dll") | ForEach-Object { Join-Path $qtBin $_ }
        if (-not ($probe | Where-Object { Test-Path $_ })) {
            throw ("Qt kit has no Release DLLs under: " + $qtBin)
        }
    }
}

function Assert-DeployOk([string]$exe, [string]$mode) {
    $exeDir = Split-Path -Parent $exe
    $need = @(Get-RequiredQtDlls $exe $mode)
    $missing = New-Object System.Collections.Generic.List[string]
    foreach ($n in $need) {
        if (-not (Test-Path -LiteralPath (Join-Path $exeDir $n))) { [void]$missing.Add($n) }
    }
    if (Test-NeedsGuiPlatform $exe) {
        $plat = Join-Path $exeDir "platforms"
        $qwin = @(Get-ChildItem -LiteralPath $plat -Filter "qwindows*.dll" -ErrorAction SilentlyContinue)
        if ($qwin.Count -eq 0) { [void]$missing.Add("platforms\qwindows*.dll") }
    }
    if ($missing.Count -gt 0) {
        throw ("FIX incomplete, missing: " + ($missing -join ", "))
    }
    Write-Step ("VERIFY FIX OK (" + $need.Count + " Qt DLL imports present)")
}

function Invoke-Deploy([string]$exe) {
    $exe = (Resolve-Path -LiteralPath $exe).Path
    $exeDir = Split-Path -Parent $exe
    Write-Step ("FIX: deploy Qt runtime next to exe")
    Write-Step ("exe = " + $exe)

    $qtBin = Resolve-QtBin
    if (-not $qtBin) {
        throw "Qt not found on this PC. Install Qt, or add windeployqt to PATH, or set QTDIR. (Needed only for FIX on a build machine.)"
    }
    Write-Step ("Qt bin = " + $qtBin)

    $mode = Get-WindeployModeFromExe $exe
    Write-Step ("windeploy mode = " + $mode + " (from exe imports)")
    Assert-KitHasMode $qtBin $mode
    Remove-ConflictingQtRuntime $exeDir $mode

    $windeploy = Join-Path $qtBin "windeployqt.exe"
    if (-not (Test-Path -LiteralPath $windeploy)) { $windeploy = Join-Path $qtBin "windeployqt6.exe" }

    & $windeploy "--$mode" --no-translations --no-system-d3d-compiler --compiler-runtime $exe
    if ($LASTEXITCODE -ne 0) {
        Write-Step ("WARNING: windeployqt exit=" + $LASTEXITCODE)
    }

    foreach ($n in @(Get-RequiredQtDlls $exe $mode)) {
        $dst = Join-Path $exeDir $n
        $src = Join-Path $qtBin $n
        if (-not (Test-Path $dst) -and (Test-Path $src)) {
            Copy-Item $src $dst -Force
            Write-Step ("fallback copy " + $n)
        }
    }

    Assert-DeployOk $exe $mode
    Write-Step "FIX done. You can double-click the exe now."
}

function Test-ExcludedFile([string]$name) {
    $n = $name.ToLowerInvariant()
    if ($n -match '\.(obj|ilk|iobj|ipdb|tlog|log|recipe|user|lastbuildstate|pdb)$') { return $true }
    if ($n -match 'filelistabsolute\.txt$' -or $n -match 'build\.cppclean') { return $true }
    if ($n -match '\.zip$') { return $true }
    if ($n -eq "qt.natvis") { return $true }
    return $false
}

function Test-ExcludedDir([string]$name) {
    $n = $name.ToLowerInvariant()
    if ($n.EndsWith("_portable")) { return $true }
    return @("moc", "uic", "rcc", "qmake", "obj", "ipch", ".vs", "qtexepack") -contains $n -or $n.EndsWith(".tlog")
}

function Get-PackRoot([string]$batDir) {
    if (-not $batDir -or -not (Test-Path -LiteralPath $batDir)) {
        throw "bat directory unknown; cannot create QtExePack"
    }
    $root = Join-Path $batDir $script:PackFolderName
    if (-not (Test-Path -LiteralPath $root)) {
        New-Item -ItemType Directory -Force -Path $root | Out-Null
        Write-Step ("created pack root: " + $root)
    }
    return $root
}

function Get-PackLeafName([string]$exe, [string]$base, [string]$qtMode) {
    # Prefer VS output folder name (Debug/Release) so both configs coexist
    # even when both link the same Qt variant (e.g. CA Debug uses Release Qt).
    $dirName = Split-Path -Leaf (Split-Path -Parent $exe)
    if ($dirName -match '^(?i)(Debug|Release)$') {
        return ($base + "_" + $dirName)
    }
    return ($base + "_" + $qtMode)
}

function Invoke-Pack([string]$exe, [string]$batDir) {
    $exe = (Resolve-Path -LiteralPath $exe).Path
    $exeDir = Split-Path -Parent $exe
    $exeName = Split-Path -Leaf $exe
    $base = [IO.Path]::GetFileNameWithoutExtension($exeName)
    $mode = Get-WindeployModeFromExe $exe
    $leaf = Get-PackLeafName $exe $base $mode

    $packRoot = Get-PackRoot $batDir
    $outDir = Join-Path $packRoot $leaf
    if (Test-Path $outDir) { Remove-Item $outDir -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $outDir | Out-Null
    Write-Step ("PACK output folder = " + $outDir + " (qt=" + $mode + ")")

    Copy-Item $exe (Join-Path $outDir $exeName) -Force
    Get-ChildItem -LiteralPath $exeDir -File | ForEach-Object {
        if ($_.Name -ieq $exeName) { return }
        if (Test-ExcludedFile $_.Name) { return }
        Copy-Item $_.FullName (Join-Path $outDir $_.Name) -Force
    }

    $plugins = @("platforms", "styles", "imageformats", "iconengines", "bearer", "translations", "tls", "networkinformation", "sqldrivers", "generic")
    Get-ChildItem -LiteralPath $exeDir -Directory -ErrorAction SilentlyContinue | ForEach-Object {
        if (Test-ExcludedDir $_.Name) { return }
        $ok = $false
        foreach ($p in $plugins) { if ($_.Name -ieq $p) { $ok = $true; break } }
        if (-not $ok) { return }
        Copy-Item $_.FullName (Join-Path $outDir $_.Name) -Recurse -Force
    }

    $need = @(Get-RequiredQtDlls $exe $mode)
    $missing = @($need | Where-Object { -not (Test-Path (Join-Path $outDir $_)) })
    if (Test-NeedsGuiPlatform $exe) {
        $platOk = @(Get-ChildItem -LiteralPath (Join-Path $outDir "platforms") -Filter "qwindows*.dll" -ErrorAction SilentlyContinue)
        if ($platOk.Count -eq 0) { $missing += "platforms\qwindows*.dll" }
    }
    if ($missing.Count -gt 0) {
        throw ("PACK refused: runtime incomplete. Run FIX/ALL first. missing=" + ($missing -join ", "))
    }

    $readme = @(
        ("# " + $base + " (" + $leaf + ") - portable package"),
        "",
        "1. Extract this WHOLE folder.",
        ("2. Double-click " + $exeName),
        "3. No need to install Qt if DLLs are included here.",
        "4. If Windows asks for VCRUNTIME140.dll, install VC++ 2015-2022 x64 redistributable.",
        "5. Requires 64-bit Windows.",
        ("6. Qt windeploy mode: " + $mode),
        "",
        ("Packed: " + (Get-Date -Format "yyyy-MM-dd HH:mm:ss"))
    ) -join "`r`n"
    Set-Content -LiteralPath (Join-Path $outDir "README.txt") -Value $readme -Encoding UTF8

    $zip = Join-Path $packRoot ($leaf + ".zip")
    if (Test-Path $zip) { Remove-Item $zip -Force }
    Compress-Archive -Path (Join-Path $outDir "*") -DestinationPath $zip -Force
    $cnt = (Get-ChildItem $outDir -Recurse -File).Count
    Write-Step ("PACK done: " + $cnt + " files")
    Write-Step ("Folder: " + $outDir)
    Write-Step ("Zip (send this to others): " + $zip)
}

function Read-ExePath([string]$hint) {
    $hint = ($hint + "").Trim().Trim('"')
    if ($hint -and (Test-Path -LiteralPath $hint) -and ($hint -match '\.exe$')) {
        return (Resolve-Path -LiteralPath $hint).Path
    }
    Write-Host ""
    Write-Host "Enter full path of the .exe (drag file into this window, then Enter):"
    $p = (Read-Host "Exe").Trim().Trim('"')
    if (-not $p -or -not (Test-Path -LiteralPath $p)) { throw ("exe not found: " + $p) }
    if ($p -notmatch '\.exe$') { throw ("not an exe: " + $p) }
    return (Resolve-Path -LiteralPath $p).Path
}

function Read-Mode([string]$hint) {
    $h = ($hint + "").Trim().ToLowerInvariant()
    if ($h -eq "nopause") { $h = "" }
    if ($h -eq "1" -or $h -eq "fix") { return "fix" }
    if ($h -eq "2" -or $h -eq "pack") { return "pack" }
    if ($h -eq "3" -or $h -eq "all") { return "all" }

    Write-Host ""
    Write-Host "Select:"
    Write-Host "  1  FIX   - make this exe double-clickable (copy Qt runtime beside it)"
    Write-Host "  2  PACK  - clean portable folder + zip under QtExePack\ (for sending)"
    Write-Host "  3  ALL   - FIX then PACK (recommended)"
    Write-Host ""
    $c = (Read-Host "Enter 1 / 2 / 3").Trim()
    if ($c -eq "1") { return "fix" }
    if ($c -eq "2") { return "pack" }
    if ($c -eq "3") { return "all" }
    throw ("invalid choice: " + $c)
}

Write-Host "========================================"
Write-Host " QtExeTool  (single BAT, shareable)"
Write-Host " Debug-only / Release-only / both OK"
Write-Host "========================================"

$modeIn = ($Mode + "").Trim()
if ($modeIn.ToLowerInvariant() -eq "nopause") {
    $modeIn = ""
}

$batDir = ""
if ($BatPath) {
    $batDir = Split-Path -Parent ([IO.Path]::GetFullPath($BatPath.Trim().Trim('"')))
}

$exe = Read-ExePath $ExePath
$op = Read-Mode $modeIn
Write-Step ("Target = " + $exe)
Write-Step ("Action = " + $op)
if ($batDir) { Write-Step ("Bat dir = " + $batDir) }

switch ($op) {
    "fix" { Invoke-Deploy $exe }
    "pack" { Invoke-Pack $exe $batDir }
    "all"  { Invoke-Deploy $exe; Invoke-Pack $exe $batDir }
}

Write-Step "All done"
