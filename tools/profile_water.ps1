# Build and run the isolated water profiler; measurements and scratch saves belong to diagnostics.
$ErrorActionPreference = 'Stop'
$recheckRoot = Split-Path -Parent $PSScriptRoot
Push-Location -LiteralPath $recheckRoot
$oldInclude = $env:INCLUDE
$oldLib = $env:LIB
try {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $install = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    $version = (Get-Content -LiteralPath (Join-Path $install 'VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt')).Trim()
    $vc = Join-Path $install "VC/Tools/MSVC/$version"
    $sdk = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits/10'
    $sdkVersion = (Get-ChildItem -LiteralPath (Join-Path $sdk 'Include') -Directory | Where-Object {
        (Test-Path -LiteralPath (Join-Path $_.FullName 'ucrt/ctype.h')) -and
        (Test-Path -LiteralPath (Join-Path $sdk "Lib/$($_.Name)/um/x86/kernel32.lib"))
    } | Sort-Object Name -Descending | Select-Object -First 1).Name
    if (-not $sdkVersion) {
        throw 'No complete Windows SDK found'
    }
    if (-not (Test-Path -LiteralPath build/runtime.obj) -or -not (Test-Path -LiteralPath build/box2d220/b2World.obj)) {
        throw 'Build the game once with build.cmd before running the isolated recheck'
    }
    $env:INCLUDE = "$vc/include;$sdk/Include/$sdkVersion/shared;$sdk/Include/$sdkVersion/um;$sdk/Include/$sdkVersion/ucrt"
    $env:LIB = "$vc/lib/x86;$sdk/Lib/$sdkVersion/um/x86;$sdk/Lib/$sdkVersion/ucrt/x86"
    $cl = Join-Path $vc 'bin/Hostx64/x86/cl.exe'
    $link = Join-Path $vc 'bin/Hostx64/x86/link.exe'
    & $cl /nologo /c /O1 /Oi /GS- /Gs9999999 /GR- /EHs-c- /Zl /W4 /arch:IA32 /fp:fast /DNDEBUG /Ithird_party/box2d220 /Fo:build/water-profile.obj tools/water_profile.cpp
    if ($LASTEXITCODE -ne 0) {
        throw 'Water profile compilation failed'
    }
    $objects = @('build/water-profile.obj','build/runtime.obj') + @(Get-ChildItem -LiteralPath build/box2d220 -Filter '*.obj' | Select-Object -ExpandProperty FullName)
    & $link /nologo /NODEFAULTLIB /ENTRY:start /SUBSYSTEM:WINDOWS,5.01 /MACHINE:X86 /IGNORE:4210 /OPT:REF /OPT:ICF /OUT:build/WaterProfile.exe @objects kernel32.lib user32.lib gdi32.lib opengl32.lib winmm.lib
    if ($LASTEXITCODE -ne 0) {
        throw 'Water profile link failed'
    }
    $process = Start-Process -FilePath (Join-Path $recheckRoot 'build/WaterProfile.exe') -ArgumentList '--water-profile' -WindowStyle Hidden -PassThru -Wait
    Get-Content -LiteralPath bin/test-output/water-performance.csv
    if ($process.ExitCode -ne 0) {
        throw "Water profile returned $($process.ExitCode)"
    }
} finally {
    $env:INCLUDE = $oldInclude
    $env:LIB = $oldLib
    Pop-Location
}
