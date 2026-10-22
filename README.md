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
- Pillow (`python -m pip install Pillow`) to prepare the editable Maker chapter images and generate the game icon from your APK.

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

## Course maker

Defeating the Wizard (stage 6-1) adds a hidden **MAKER** card before Grasslands on the World Select screen. Typing `SKELETONKEY` on the title screen unlocks it as well. The card opens a list of 60 course slots; courses are stored in `bin/saves/maker/` and can be copied between installations.

The chapter's artwork layers are `resources/maker-edit/MAKER-scene.png` and `MAKER-title.png`. Keep their 832 x 1024 transparent canvases when editing; building prepares runtime textures from both layers, including with `-SkipAssets`.

- Bricks from every world can be mixed in one course: whole, half and quarter pieces with 32/16/8/4/2/1-unit grid snapping, on back, main and front layers. Each brick is solid, a one-way platform, a slope or pure decoration. The default comes from the original stages, so pillar tops and planks keep their one-way platforms.
- `T` paints only a half or a quarter of any brick. The **Fills** tab holds the colour strips the original stages stretch over large areas, such as black cave fill, lava, water tints and shadows; Shift+drag spans one fill over a whole rectangle.
- Water, wind and other areas are see-through while editing: bricks and fish can be placed inside them, and an area is moved by its frame or name tag.
- Objects include gems, the axolotl, checkpoints, goal springs, enemies, cannons, pendulums, moving platforms, switches, water and wind. The Wizard and the princess cannot be placed; Bean appears at the start marker. Double-click an object to edit its settings. Only the settings an object supports are listed, for example:
  - Enemies: walk, hop and swim speeds; whether they are always active, wait for Bean or are active only near Bean; a switch group flipped on defeat; stomp bounce.
  - Cannons: direction, fire interval, bullet speed, spread shots and aim speed.
  - Moving platforms and patrollers: travel, speed, start point and first direction. Pendulums: swing time, swing angle, start angle and first swing direction. Orbits: radius, lap time and start angle.
  - Breakable blocks: hits to break and which sides break them. Spikes: retract and extend with a switch group. Crumbling clouds: respawn time.
  - Wind: direction, strength, speed cap and how much strength is left at the far end. Water: buoyancy and drag. Physics blocks and balls: weight, bounciness, friction, gravity and rotation. Springs, trampolines, seesaws, spinners, the helper bird, checkpoints and particle areas have their own settings too.
  - A mover wall joins the enemy and Bean walls: it is invisible and only turns back moving objects.
- Course settings include name, theme, background, music, weather, length and height, gold-shoe and par jump counts, the number of gems needed, Bean's run speed and jump height (50% to 200%), the weather amount, and the camera (follow both axes, horizontal only or vertical only, with a look-ahead offset like the authored stages).
- Enter or F5 starts a test from the start, and Shift+Enter starts one from the cursor (a cursor inside bricks is lifted onto them). F5 or Tab returns to the editor. Clearing a test from the start marks the course as verified and reports the jumps used.
- F1 in the editor lists every control: undo/redo, copy/paste, eyedropper, rectangle fill, zoom and the hitbox view. Hovering a toolbar button shows its shortcut, and the mouse wheel steps the value under the cursor in the settings and object panels.

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
