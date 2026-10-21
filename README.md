# Bean's Quest for Windows

English | [简体中文](README_zh.md)

An unofficial C++ reconstruction of Bean's Quest for Windows, intended for learning and technical reference. The original game was created by Kumobius. This project is not an official Kumobius release.

## Copyright and license

Original contributions that BingAlpha is entitled to license are available under the [MIT License](LICENSE), with the notice `Copyright (c) 2026 BingAlpha`.

This license does not cover the original game program, APK, artwork, characters, animation, levels, music, sound effects, or other original resources, including extracted or converted copies. Rights in Bean's Quest remain with Kumobius (the game's creators) and the respective rightsholders. Reconstructed material is not automatically cleared for redistribution by adding this license. Third-party components retain their own licenses; see [THIRD_PARTY_NOTICES.txt](THIRD_PARTY_NOTICES.txt).

Being free of charge, non-commercial, labeled "for learning only", or crediting the original authors does not automatically grant permission to publish. The learning/reference purpose does not add a non-commercial restriction to the MIT-licensed contributions.

## Supply your own APK

**No APK is provided.** Obtain your own lawful copy of the original Android game. Rename it exactly **`Bean's Quest.apk`** and place it beside `build.cmd` in the project root:

```text
Bean's Quest/
  Bean's Quest.apk
  build.cmd
  build.ps1
  src/
  tools/
  third_party/
```

Use the straight apostrophe (`'`) and check that the filename does not end in `.apk.apk` when Windows hides file extensions. The preparation script expects the original `assets/Media/` layout; modified or incompatible APKs may not work. APKs and generated resource directories are ignored by `.gitignore`; that exclusion is not permission to redistribute them.

## Intro video source

`resources/intro.mp4` comes from [this YouTube video](https://www.youtube.com/watch?v=EJ_j8Lq6bR4), uploaded by [Kumobius](https://www.youtube.com/@Kumobius).

## Build requirements

- A 64-bit Windows build environment.
- Visual Studio 2022 or its Build Tools, with **Desktop development with C++**, MSVC x86/x64 tools, and a complete **Windows 10 SDK**.
- **Python 3.10 or later**, available as `python` on `PATH`.
- **FFmpeg**, available as `ffmpeg` on `PATH`, for initial audio conversion.
- Optional: Pillow (`python -m pip install Pillow`) to generate the game icon from your APK.

Box2D 2.2.0 is included in `third_party/box2d220`. No Android SDK is needed.

## Build and run

Open PowerShell in the project root and run:

```powershell
.\build.cmd -NoPause
.\bin\BeansQuest.exe
```

The script locates the MSVC toolchain automatically, prepares resources from your APK when needed, and builds a **32-bit Windows executable**. Python and FFmpeg are build tools, not runtime dependencies.

For later builds with prepared resources:

```powershell
.\build.cmd -SkipAssets -NoPause
```

To run asset preparation again:

```powershell
.\build.cmd -PrepareAssets -NoPause
```

`-SkipAssets` and `-PrepareAssets` cannot be combined. Keep Python installed: an optional `resources/intro.mp4` is handled by a Python preparation step even when APK extraction is skipped. Close the game normally before replacing its executable.

## Output and saves

```text
bin/
  BeansQuest.exe
  LICENSE
  THIRD_PARTY_NOTICES.txt
  data/                    Prepared runtime resources
  saves/                   Personal progress
build/                     Compiler intermediates and test output
```

Keep `data/` beside `BeansQuest.exe`; the executable alone is not a complete runnable copy. Back up `bin/saves/` before replacing or moving your local installation.

## Tests and local packaging

With resources already prepared, build and run the isolated regression executable:

```powershell
.\build.cmd -SkipAssets -Test -NoPause
```

Tests use `build/tests/BeansQuest.tests.exe` and `build/tests/test-output/`, separate from the game's personal saves.

To package a local runnable copy:

```powershell
python tools/package.py
```

This creates `bin/BeanQuest-Windows-x86.zip`, containing the executable, runtime resources, MIT license, third-party notices, and an empty save directory. Personal saves and development documents are excluded. **The package contains original game resources; successful packaging does not grant permission to publish it.** Any redistribution of MIT-covered contributions must retain the MIT copyright and permission notice, alongside applicable third-party notices.
