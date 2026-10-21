# Bean's Quest for Windows

[English](README.md) | 简体中文

Bean's Quest 的非官方 Windows C++ 重建项目，用于学习与技术参考。原游戏由 Kumobius 制作，本项目并非 Kumobius 的官方发行版本。

## 版权与许可

BingAlpha 有权授权的本项目原创贡献采用 [MIT 协议](LICENSE)，版权行如下：

```text
Copyright (c) 2026 BingAlpha
```

该许可不包含原作程序、APK、美术、角色、动画、关卡、音乐、音效及其他原作资源，也不包含它们的提取或转换副本。Bean's Quest 的相关权利归 Kumobius（原游戏制作组）及相应权利人所有。添加 MIT 协议不代表参照原作重建的内容自动获得再发布许可。第三方组件沿用各自的许可，见 [THIRD_PARTY_NOTICES.txt](THIRD_PARTY_NOTICES.txt)。

免费、非商用、写“仅供学习”、保留原作者署名，都不自动获得发布权。

本项目的学习参考用途说明，不构成对 MIT 授权部分额外施加的非商用限制。

## 自备 APK

**本项目不提供 APK。** 请自行合法获取原版 Android 游戏，将文件准确命名为 **`Bean's Quest.apk`**，放在项目根目录，与 `build.cmd` 同级：

```text
Bean's Quest/
  Bean's Quest.apk
  build.cmd
  build.ps1
  src/
  tools/
  third_party/
```

文件名使用英文半角单引号（`'`）。若 Windows 隐藏了扩展名，请确认没有误命名为 `.apk.apk`。资源准备脚本要求 APK 保留原版的 `assets/Media/` 结构，修改版或结构不同的 APK 可能无法使用。`.gitignore` 已排除 APK 和生成的资源目录；忽略文件并不代表获得了这些内容的再发布许可。

## 开场视频来源

`resources/intro.mp4` 来自 [这段 YouTube 视频](https://www.youtube.com/watch?v=EJ_j8Lq6bR4)，发布者为 [Kumobius](https://www.youtube.com/@Kumobius)。

## 编译环境

- 64 位 Windows 编译环境。
- Visual Studio 2022 或其 Build Tools，安装 **“使用 C++ 的桌面开发”**、MSVC x86/x64 工具和完整的 **Windows 10 SDK**。
- **Python 3.10 或更新版本**，确保能通过 `PATH` 执行 `python`。
- **FFmpeg**，确保能通过 `PATH` 执行 `ffmpeg`，用于首次转换音频。
- 可选：Pillow（`python -m pip install Pillow`），用于从自备 APK 生成游戏图标。

项目已包含 `third_party/box2d220` 下的 Box2D 2.2.0，无需安装 Android SDK。

## 编译与运行

在项目根目录打开 PowerShell，执行：

```powershell
.\build.cmd -NoPause
.\bin\BeansQuest.exe
```

脚本会自动定位 MSVC 工具链，按需从自备 APK 准备资源，并生成 **32 位 Windows 程序**。Python 和 FFmpeg 用于构建，运行游戏无需安装它们。

资源已准备完成后，可跳过 APK 提取：

```powershell
.\build.cmd -SkipAssets -NoPause
```

需要重新执行资源准备时：

```powershell
.\build.cmd -PrepareAssets -NoPause
```

`-SkipAssets` 与 `-PrepareAssets` 不能同时使用。请保留 Python：若存在可选的 `resources/intro.mp4`，即使跳过 APK 提取，构建仍会调用 Python 处理视频准备步骤。替换主程序前，请正常退出游戏。

## 输出与存档

```text
bin/
  BeansQuest.exe
  LICENSE
  THIRD_PARTY_NOTICES.txt
  data/                    准备好的运行资源
  saves/                   个人游戏进度
build/                     编译中间文件及测试输出
```

请将 `data/` 保持在 `BeansQuest.exe` 旁边，仅复制 EXE 无法组成完整的可运行副本。替换或移动本地安装目录前，请备份 `bin/saves/`。

## 测试与本地打包

资源已准备完成后，可编译并运行独立的回归测试程序：

```powershell
.\build.cmd -SkipAssets -Test -NoPause
```

测试使用 `build/tests/BeansQuest.tests.exe` 和 `build/tests/test-output/`，与正式游戏的个人存档分开。

生成本地可运行压缩包：

```powershell
python tools/package.py
```

脚本生成 `bin/BeanQuest-Windows-x86.zip`，包含主程序、运行资源、MIT 许可、第三方声明及空存档目录，不打包个人存档和开发文档。**该压缩包包含原作游戏资源，打包成功不代表获得了公开发布许可。** 再发布 MIT 授权部分时，须保留 MIT 的版权及许可声明，并遵守适用的第三方许可。
