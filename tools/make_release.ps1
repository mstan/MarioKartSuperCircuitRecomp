<#
Build the MarioKartSuperCircuitRecomp Windows x64 release archive.

The archive contains the stripped executable, recomp-ui assets, built-in mod
catalog, MinGW/SDL runtime DLLs, the toolchain-less overlay compiler, and a
short player README. It never contains a ROM, retail GBA BIOS, save data,
generated source, or developer configuration.

Usage:
  pwsh tools\make_release.ps1 -Version 0.0.2
#>
param(
    [Parameter(Mandatory = $true)][string]$Version,
    [string]$BuildDir = 'build-release',
    [string]$GbarecompRoot = '',
    [ValidateRange(1, 32)][int]$Jobs = 4
)

$ErrorActionPreference = 'Stop'

if ($Version -notmatch '^\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?$') {
    throw "Version must look like 0.0.1 (received '$Version')."
}

$mingwBin = 'C:\msys64\mingw64\bin'
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = (Resolve-Path (Join-Path $scriptDir '..')).Path
if (-not $GbarecompRoot) { $GbarecompRoot = Join-Path $root 'gbarecomp' }
$engine = (Resolve-Path $GbarecompRoot).Path
$build = Join-Path $root $BuildDir
$out = Join-Path $root 'release-stage'
$target = 'MarioKartSuperCircuitRecomp'
$stageName = "$target-windows-x64-v$Version"
$stage = Join-Path $out $stageName
$zip = Join-Path $out "$stageName.zip"
$expectedSha1 = '9d327c030c3e2d9007990518594f70c3340ac56f'
$dlls = @('SDL2.dll', 'libgcc_s_seh-1.dll', 'libstdc++-6.dll', 'libwinpthread-1.dll')

foreach ($required in @(
    (Join-Path $mingwBin 'cc.exe'),
    (Join-Path $mingwBin 'c++.exe'),
    (Join-Path $mingwBin 'ninja.exe'),
    (Join-Path $mingwBin 'strip.exe'),
    (Join-Path $engine 'CMakeLists.txt'),
    (Join-Path $engine 'tools\fetch_tcc.ps1'),
    (Join-Path $engine 'src\runtime\generated_bios\bios_recompiled.cpp'),
    (Join-Path $engine 'src\runtime\generated_bios\bios_recompiled.h'),
    (Join-Path $root 'recomp\launcher\boxart.tga')
)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required release dependency is missing: $required"
    }
}

$generated = @(Get-ChildItem -LiteralPath (Join-Path $root 'generated') `
    -Filter 'recompiled_*.cpp' -File -ErrorAction SilentlyContinue)
if ($generated.Count -eq 0) {
    throw 'Generated guest shards are missing. Supply the verified ROM and run tools\regen.ps1 first.'
}

$env:PATH = "$mingwBin;$env:PATH"
New-Item -ItemType Directory -Force -Path $out | Out-Null

& cmake -S $root -B $build -G Ninja `
    -DCMAKE_C_COMPILER="$mingwBin/cc.exe" `
    -DCMAKE_CXX_COMPILER="$mingwBin/c++.exe" `
    -DCMAKE_MAKE_PROGRAM="$mingwBin/ninja.exe" `
    -DCMAKE_BUILD_TYPE=Release `
    "-DCMAKE_CXX_FLAGS_RELEASE=-O1 -DNDEBUG" `
    -DGBARECOMP_ROOT="$engine" `
    -DGBARECOMP_BUILD_ORACLE=OFF `
    -DGBARECOMP_MINGW_PREFIX_UNIX='/c/msys64/mingw64' `
    -DSDL2_INCLUDE_DIR='C:/msys64/mingw64/include/SDL2' `
    -DSDL2_LIBRARY='C:/msys64/mingw64/lib/libSDL2.dll.a'
if ($LASTEXITCODE -ne 0) { throw "Release configure failed ($LASTEXITCODE)." }

& cmake --build $build --target $target --parallel $Jobs
if ($LASTEXITCODE -ne 0) { throw "Release build failed ($LASTEXITCODE)." }

$exe = Join-Path $build "$target.exe"
if (-not (Test-Path -LiteralPath $exe)) { throw "Expected executable is missing: $exe" }
& (Join-Path $mingwBin 'strip.exe') $exe
if ($LASTEXITCODE -ne 0) { throw "strip failed ($LASTEXITCODE)." }

if (Test-Path -LiteralPath $stage) {
    $resolvedStage = (Resolve-Path -LiteralPath $stage).Path
    $resolvedOut = (Resolve-Path -LiteralPath $out).Path
    if (-not $resolvedStage.StartsWith("$resolvedOut\", [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove stage outside release output: $resolvedStage"
    }
    Remove-Item -LiteralPath $resolvedStage -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $stage | Out-Null
Copy-Item -LiteralPath $exe -Destination $stage
foreach ($dll in $dlls) {
    $source = Join-Path $mingwBin $dll
    if (-not (Test-Path -LiteralPath $source)) { throw "Runtime DLL is missing: $source" }
    Copy-Item -LiteralPath $source -Destination $stage
}

$assets = Join-Path $build 'assets'
$stagedBoxart = Join-Path $assets 'img\boxart.tga'
if (-not (Test-Path -LiteralPath $stagedBoxart)) {
    throw "recomp-ui launcher box art is missing: $stagedBoxart"
}
Copy-Item -LiteralPath $assets -Destination $stage -Recurse

$mods = Join-Path $build 'mods'
if (-not (Test-Path -LiteralPath (Join-Path $mods 'packages'))) {
    throw "Preloaded mod catalog is missing: $mods"
}
Copy-Item -LiteralPath $mods -Destination $stage -Recurse

& (Join-Path $engine 'tools\fetch_tcc.ps1') `
    -Toolchain (Join-Path $stage 'overlay_toolchain') -EngineRoot $engine
if ($LASTEXITCODE -ne 0) { throw "Overlay toolchain staging failed ($LASTEXITCODE)." }

foreach ($requiredStaged in @(
    (Join-Path $stage 'overlay_toolchain\tcc\tcc.exe'),
    (Join-Path $stage 'overlay_toolchain\tcc\libtcc.dll'),
    (Join-Path $stage 'overlay_toolchain\include\overlay_runtime_arm.h'),
    (Join-Path $stage 'overlay_toolchain\include\overlay_abi.h'),
    (Join-Path $stage 'overlay_toolchain\include\runtime_arm_types.h')
)) {
    if (-not (Test-Path -LiteralPath $requiredStaged)) {
        throw "Overlay toolchain staging is incomplete: $requiredStaged"
    }
}

@"
# Mario Kart: Super Circuit - GBA static recompilation (Windows x64)

This v$Version preview runs the USA retail game through gbarecomp with a
recomp-ui launcher and optional enhancement mods.

This experimental build is a byproduct of developing and testing gbarecomp.
It has no affiliation with or endorsement from Nintendo.

## How to run

1. Extract this entire folder and keep its DLLs, assets, mods, and
   overlay_toolchain beside $target.exe.
2. Run $target.exe.
3. Select your legally obtained Mario Kart: Super Circuit (USA) ROM.
   Expected SHA-1: $expectedSha1
4. Select your legally obtained retail gba_bios.bin under Settings > System.
5. Open Mods to opt into Adaptive Widescreen or 60 FPS, then select PLAY.

Display, audio, input, and enhancement settings persist locally. Keyboard
defaults: arrows = D-pad, Z = A, X = B, A = L, S = R, Enter = Start,
Right Shift = Select, and Tab = fast-forward. Shift+F1-F9 saves a state;
F1-F9 loads one.

The ROM and retail GBA BIOS are not included. Supply your own dumps.
Interactive play has no implicit runtime step limit; only an explicit
``--steps`` option applies a deterministic upper bound.

Project: https://github.com/mstan/MarioKartSuperCircuitRecomp
Engine: https://github.com/mstan/gbarecomp
"@ | Out-File -LiteralPath (Join-Path $stage 'README.md') -Encoding utf8

$forbidden = Get-ChildItem -LiteralPath $stage -Recurse -File | Where-Object {
    $_.Extension -in @('.gba', '.sav', '.srm') -or
    $_.Name -ieq 'gba_bios.bin' -or
    $_.Name -in @(
        'game.toml',
        'rom.cfg',
        'bios.cfg',
        'config.ini',
        'keybinds.ini',
        'mods.ini',
        'state.toml'
    )
}
if ($forbidden) {
    throw "Forbidden release content detected: $($forbidden.FullName -join ', ')"
}

if (Test-Path -LiteralPath $zip) { Remove-Item -LiteralPath $zip -Force }
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

# ZIP entry names must always use '/', regardless of the host OS.
# Compress-Archive preserves Windows backslashes, which POSIX extractors
# treat as literal filename characters rather than directory separators -- so
# a Linux / Steam Deck / Proton user gets files literally named
# "assets\fonts\LatoLatin-Regular.ttf" (and "mods\packages\..." where the
# game ships a mod catalog), the nested trees are never created, and the
# ImGui launcher finds neither its fonts nor its mods. Write portably here,
# then verify before anyone can publish it.
# (Convention ported from snesrecomp/SuperMarioWorldRecomp.)
$rzStageFull = [IO.Path]::GetFullPath((Resolve-Path -LiteralPath $stage).Path)
$rzZipFull = [IO.Path]::GetFullPath($zip)
$rzPrefix = $rzStageFull.TrimEnd('\') + '\'
$rzFiles = @(Get-ChildItem -LiteralPath $stage -File -Recurse |
    Sort-Object FullName)
$rzArchive = [IO.Compression.ZipFile]::Open(
    $rzZipFull, [IO.Compression.ZipArchiveMode]::Create)
try {
    foreach ($rzFile in $rzFiles) {
        $rzFull = [IO.Path]::GetFullPath($rzFile.FullName)
        if (-not $rzFull.StartsWith(
                $rzPrefix, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing to archive a file outside the release stage: $rzFull"
        }
        $rzName = $rzFull.Substring($rzPrefix.Length).Replace('\', '/')
        if ($rzName.StartsWith('/') -or $rzName -match '(^|/)..(/|$)') {
            throw "Unsafe ZIP entry name: $rzName"
        }
        [IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
            $rzArchive, $rzFull, $rzName,
            [IO.Compression.CompressionLevel]::Optimal) | Out-Null
    }
} finally {
    $rzArchive.Dispose()
}

# Read the archive back and reject non-portable entry names outright, so a
# regression in the writer cannot ship a Windows-only zip again.
$rzArchive = [IO.Compression.ZipFile]::OpenRead($rzZipFull)
try {
    $rzBad = @($rzArchive.Entries | Where-Object {
        $_.FullName.Contains('\') -or
        $_.FullName.StartsWith('/') -or
        $_.FullName -match '(^|/)..(/|$)'
    })
    if ($rzBad.Count -ne 0) {
        throw "ZIP contains non-portable entry names: $(
            ($rzBad | ForEach-Object FullName) -join ', ')"
    }
    if ($rzArchive.Entries.Count -ne $rzFiles.Count) {
        throw "ZIP entry count mismatch: expected $($rzFiles.Count), got $(
            $rzArchive.Entries.Count)"
    }
} finally {
    $rzArchive.Dispose()
}
Write-Host "--- $stageName ---"
Get-ChildItem -LiteralPath $stage | Select-Object Name, Length | Out-Host
Get-Item -LiteralPath $zip | Select-Object Name, Length | Out-Host
Get-FileHash -LiteralPath $zip -Algorithm SHA256 |
    Select-Object Algorithm, Hash, Path | Out-Host
