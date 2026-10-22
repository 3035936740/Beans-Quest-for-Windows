# Shared release build for build.bat and build.cmd; explicit tests use a separate executable and saves.
param([switch]$SkipAssets, [switch]$PrepareAssets, [switch]$Test, [switch]$NoPause, [string]$OutputName = 'BeansQuest.exe', [string]$TestArgument = '--self-test')

$ErrorActionPreference = 'Stop'
$buildExitCode = 0
$previousInclude = $env:INCLUDE
$previousLib = $env:LIB
Push-Location -LiteralPath $PSScriptRoot
try {
    if ($OutputName -notmatch '^[A-Za-z0-9_.-]+\.exe$') {
        throw 'OutputName must be an executable filename.'
    }
    if ($TestArgument -notmatch '^--self-test(?:-[a-z0-9-]+)?$') {
        throw 'Invalid isolated test selection.'
    }
    $gameOutput = Join-Path 'bin' $OutputName
    New-Item -ItemType Directory -Force build,bin,bin/saves | Out-Null
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) {
        throw 'Install Visual Studio with the Desktop development with C++ workload and a Windows SDK.'
    }
    $install = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $install) {
        throw 'Install the Visual Studio Desktop development with C++ workload.'
    }
    $toolVersion = (Get-Content -LiteralPath (Join-Path $install 'VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt')).Trim()
    $vc = Join-Path $install "VC/Tools/MSVC/$toolVersion"
    $sdk = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits/10'
    $sdkVersion = (Get-ChildItem -LiteralPath (Join-Path $sdk 'Include') -Directory | Where-Object {
        (Test-Path -LiteralPath (Join-Path $_.FullName 'ucrt/ctype.h')) -and
        (Test-Path -LiteralPath (Join-Path $sdk "Lib/$($_.Name)/um/x86/kernel32.lib"))
    } | Sort-Object Name -Descending | Select-Object -First 1).Name
    if (-not $sdkVersion) {
        throw 'No complete Windows 10 SDK was found. Install one through Visual Studio Installer.'
    }
    $env:INCLUDE = "$vc/include;$sdk/Include/$sdkVersion/shared;$sdk/Include/$sdkVersion/um;$sdk/Include/$sdkVersion/ucrt"
    $env:LIB = "$vc/lib/x86;$sdk/Lib/$sdkVersion/um/x86;$sdk/Lib/$sdkVersion/ucrt/x86"
    $cl = Join-Path $vc 'bin/Hostx64/x86/cl.exe'
    $link = Join-Path $vc 'bin/Hostx64/x86/link.exe'
    $rc = Get-ChildItem -Path (Join-Path $sdk 'bin/*/x64/rc.exe') | Sort-Object FullName -Descending | Select-Object -First 1 -ExpandProperty FullName

    # Preserve saves from the previous output location; never overwrite a bin save.
    if (Test-Path -LiteralPath 'dist/BeanQuest/saves') {
        foreach ($save in Get-ChildItem -LiteralPath 'dist/BeanQuest/saves' -File) {
            $destination = Join-Path 'bin/saves' $save.Name
            if (-not (Test-Path -LiteralPath $destination)) {
                Copy-Item -LiteralPath $save.FullName -Destination $destination
            }
        }
    }
    if ($SkipAssets -and $PrepareAssets) {
        throw 'Use either -SkipAssets or -PrepareAssets.'
    }
    if ($SkipAssets) {
        if (-not (Test-Path -LiteralPath 'bin/data/Media/project.pgame')) {
            if (Test-Path -LiteralPath 'dist/BeanQuest/data/Media/project.pgame') {
                Copy-Item -LiteralPath 'dist/BeanQuest/data' -Destination 'bin/data' -Recurse
            } else {
                throw 'Prepared assets are missing. Run build.cmd without -SkipAssets first.'
            }
        }
    }
    $assetsReady = (Test-Path -LiteralPath 'bin/data/Media/project.pgame') -and
        (Test-Path -LiteralPath 'bin/data/Media/Font/Vera.ttf') -and
        (@(Get-ChildItem -LiteralPath 'bin/data/audio' -Filter '*.wav' -ErrorAction SilentlyContinue).Count -ge 33)
    if ($PrepareAssets -or (-not $SkipAssets -and -not $assetsReady)) {
        if (-not (Get-Command python -ErrorAction SilentlyContinue)) {
            throw 'Python 3 is required for asset preparation. With prepared bin/data, use -SkipAssets.'
        }
        & python tools/prepare.py
        if ($LASTEXITCODE -ne 0) {
            throw 'Asset preparation failed.'
        }
    }
    if ($SkipAssets -and -not $assetsReady) {
        throw 'Complete runtime assets are missing. Run build.cmd -PrepareAssets first.'
    }
    if (Test-Path -LiteralPath 'resources/intro.mp4') {
        & python tools/prepare_intro.py
        if ($LASTEXITCODE -ne 0) {
            throw 'Attract intro preparation failed.'
        }
    }
    # Maker layers are user-editable PNGs; refresh them even when APK extraction is skipped.
    & python tools/prepare_maker_art.py
    if ($LASTEXITCODE -ne 0) {
        throw 'Maker artwork preparation failed.'
    }
    $objects = @('build/main.obj','build/runtime.obj')
    New-Item -ItemType Directory -Force build/box2d220 | Out-Null
    $physicsSources = @(Get-ChildItem -LiteralPath 'third_party/box2d220/Box2D' -Recurse -Filter '*.cpp')
    $physicsHeaders = @(Get-ChildItem -LiteralPath 'third_party/box2d220/Box2D' -Recurse -Filter '*.h')
    $physicsStamp = 'build/box2d220/compiled.stamp'
    # Source restoration can preserve an older timestamp while changing bytes.
    # Hash the complete input set so native comparison builds cannot reuse the
    # other implementation's object files merely because its stamp is newer.
    $physicsHashes = @($physicsSources + $physicsHeaders | Sort-Object FullName | ForEach-Object {
        $_.FullName.Substring($PSScriptRoot.Length) + '=' + (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    })
    $physicsSignature = "Box2D 2.2.0 x86 CRT-free; MSVC $toolVersion; /O2 /Oi /Gy /Gw /Zc:inline /GS- /Gs9999999 /GR- /EHs-c- /Zl /arch:IA32 /fp:fast /DNDEBUG /D_CRT_SECURE_NO_WARNINGS`n" + ($physicsHashes -join "`n")
    $physicsCachedSignature = if (Test-Path -LiteralPath $physicsStamp) {
        (Get-Content -LiteralPath $physicsStamp -Raw).TrimEnd()
    } else {
        ''
    }
    if ($physicsCachedSignature -cne $physicsSignature.TrimEnd()) {
        & $cl /nologo /c /O2 /Oi /Gy /Gw /Zc:inline /GS- /Gs9999999 /GR- /EHs-c- /Zl /arch:IA32 /fp:fast /DNDEBUG /D_CRT_SECURE_NO_WARNINGS /Ithird_party/box2d220 /Fo:build/box2d220/ $physicsSources.FullName
        if ($LASTEXITCODE -ne 0) {
            throw 'Box2D compilation failed.'
        }
        Set-Content -LiteralPath $physicsStamp -Value $physicsSignature -Encoding ASCII
    }
    $objects += @(Get-ChildItem -LiteralPath 'build/box2d220' -Filter '*.obj' | Select-Object -ExpandProperty FullName)
    if (Test-Path -LiteralPath 'build/game.ico') {
        if (-not $rc) {
            throw 'Windows SDK Resource Compiler (rc.exe) was not found.'
        }
        '101 ICON "game.ico"' | Set-Content -LiteralPath 'build/game.rc' -Encoding ascii
        & $rc /nologo /fo build/game.res build/game.rc
        if ($LASTEXITCODE -ne 0) {
            throw 'Resource compilation failed.'
        }
        $objects += 'build/game.res'
    }
    # The distributed program never contains self-tests or runtime diagnostics.
    & $cl /nologo /c /O2 /Oi /Gy /Gw /Zc:inline /GS- /Gs9999999 /GR- /EHs-c- /Zl /W4 /arch:IA32 /fp:fast /DNDEBUG /DBQ_ENABLE_TESTS=0 /D_CRT_SECURE_NO_WARNINGS /Ithird_party/box2d220 /Fo:build/main.obj src/main.cpp
    if ($LASTEXITCODE -ne 0) {
        throw 'C++ compilation failed.'
    }
    & $cl /nologo /c /Od /Oi- /GS- /Zl /Fo:build/runtime.obj src/runtime.cpp
    if ($LASTEXITCODE -ne 0) {
        throw 'Runtime helper compilation failed.'
    }
    # runtime.cpp explicitly calls the .CRT constructor table before game startup.
    & $link /nologo /NODEFAULTLIB /ENTRY:start /SUBSYSTEM:WINDOWS,5.01 /MACHINE:X86 /IGNORE:4210 /DEBUG:NONE /INCREMENTAL:NO /DYNAMICBASE /NXCOMPAT /OPT:REF /OPT:ICF "/OUT:$gameOutput" @objects kernel32.lib user32.lib gdi32.lib opengl32.lib winmm.lib
    if ($LASTEXITCODE -ne 0) {
        throw 'Link failed.'
    }
    $releaseBytes = [System.IO.File]::ReadAllBytes((Join-Path $PSScriptRoot $gameOutput))
    $releasePeOffset = [System.BitConverter]::ToInt32($releaseBytes, 0x3c)
    if ([System.BitConverter]::ToUInt32($releaseBytes, $releasePeOffset + 12) -ne 0 -or [System.BitConverter]::ToUInt32($releaseBytes, $releasePeOffset + 16) -ne 0) {
        throw 'Release contains a COFF symbol table.'
    }
    $releaseText = [System.Text.Encoding]::ASCII.GetString($releaseBytes)
    foreach ($marker in @('test-output','--self-test','--audio-test','live-jump-audio.csv','live-jump-physics.csv')) {
        if ($releaseText.Contains($marker)) {
            throw "Release contains a test/diagnostic marker: $marker"
        }
    }
    # Keep project and third-party license notices beside the runtime.
    Copy-Item -LiteralPath 'LICENSE','THIRD_PARTY_NOTICES.txt' -Destination 'bin'
    # Remove development documents left by previous builds.
    foreach ($document in @('README.md','README_zh.md','NATIVE_REVIEW.md','PORTING_AUDIT.txt')) {
        $documentOutput = Join-Path 'bin' $document
        if (Test-Path -LiteralPath $documentOutput -PathType Leaf) {
            Remove-Item -LiteralPath $documentOutput
        }
    }
    Write-Host "Built: $(Join-Path $PSScriptRoot $gameOutput)" -ForegroundColor Green
    Write-Host 'x86, Windows subsystem 5.01, no CRT dependency. Runtime assets: bin/data; saves: bin/saves.'
    if ($Test) {
        # Explicit tests use their own executable, working directory and saves.
        # Only runtime assets are shared through a directory junction.
        $testFolder = Join-Path $PSScriptRoot 'build/tests'
        New-Item -ItemType Directory -Force -Path $testFolder | Out-Null
        $testAssets = Join-Path $testFolder 'data'
        $assetTarget = (Resolve-Path -LiteralPath 'bin/data').Path
        if (Test-Path -LiteralPath $testAssets) {
            $assetLink = Get-Item -LiteralPath $testAssets
            if ($assetLink.LinkType -ne 'Junction' -or $assetLink.Target -notcontains $assetTarget) {
                throw 'Test assets must be a junction to the prepared bin/data directory.'
            }
        } else {
            New-Item -ItemType Junction -Path $testAssets -Target $assetTarget | Out-Null
        }
        & $cl /nologo /c /O2 /Oi /Gy /Gw /Zc:inline /GS- /Gs9999999 /GR- /EHs-c- /Zl /W4 /arch:IA32 /fp:fast /DNDEBUG /DBQ_ENABLE_TESTS=1 /D_CRT_SECURE_NO_WARNINGS /Ithird_party/box2d220 /Fo:build/tests/main.obj src/main.cpp
        if ($LASTEXITCODE -ne 0) {
            throw 'Test compilation failed.'
        }
        $testObjects = @('build/tests/main.obj') + @($objects | Where-Object {
            $_ -ne 'build/main.obj'
        })
        $testOutput = Join-Path $testFolder 'BeansQuest.tests.exe'
        & $link /nologo /NODEFAULTLIB /ENTRY:start /SUBSYSTEM:WINDOWS,5.01 /MACHINE:X86 /IGNORE:4210 /DEBUG:NONE /INCREMENTAL:NO /DYNAMICBASE /NXCOMPAT /OPT:REF /OPT:ICF "/OUT:$testOutput" @testObjects kernel32.lib user32.lib gdi32.lib opengl32.lib winmm.lib
        if ($LASTEXITCODE -ne 0) {
            throw 'Test link failed.'
        }
        $p = Start-Process -FilePath $testOutput -ArgumentList $TestArgument -WindowStyle Hidden -PassThru -Wait
        $testReport = Join-Path $testFolder 'test-output/results.txt'
        if (-not (Test-Path -LiteralPath $testReport)) {
            throw 'Self-test did not produce a report.'
        }
        Get-Content -LiteralPath $testReport
        if ($p.ExitCode -ne 0) {
            throw "Self-test failed: $($p.ExitCode)"
        }
    }
} catch {
    $buildExitCode = 1
    Write-Host "Build failed: $($_.Exception.Message)" -ForegroundColor Red
} finally {
    $env:INCLUDE = $previousInclude
    $env:LIB = $previousLib
    Pop-Location
    if (-not $NoPause) {
        Read-Host 'Press Enter to close' | Out-Null
    }
}
exit $buildExitCode
