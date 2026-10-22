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
- Pillow（`python -m pip install Pillow`），用于准备可编辑的 Maker 章节图片，以及从自备 APK 生成游戏图标。

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

## 关卡编辑器

打败巫师（6-1）后，世界选择界面会在第一章 Grasslands 前面出现隐藏的 **MAKER** 卡片；在标题画面输入 `SKELETONKEY` 也会同时解锁。进入后是 60 个关卡槽位的列表，关卡保存在 `bin/saves/maker/`，可以直接复制给别人。

章节美术分为 `resources/maker-edit/MAKER-scene.png` 和 `MAKER-title.png` 两层。编辑时保留 832 x 1024 的透明画布；构建会从这两个图层准备运行贴图，使用 `-SkipAssets` 也会更新。

- 所有世界的砖块都能混用：整格、半格、1/4 格，网格吸附 32/16/8/4/2/1，分后景、主层、前景三层。每块砖可设为实心、单向平台、斜坡或纯装饰，默认值取自原版关卡（柱顶、木板等会自动带单向板）。
- 按 `T` 可以只画任意砖块的左/右/上/下半块或四分之一块。**Fills** 分类收录了原版关卡里拉伸铺满大片区域的色块（洞穴黑色填充、岩浆、水色、阴影等），按住 Shift 拖动可以用一块填充铺满整个矩形。
- 编辑时水域、风场等区域是"透明"的：可以在里面刷砖、放鱼；移动区域时拖它的边框或左上角的名字标签。
- 物件包括宝石、美西螈、存档灯、终点弹簧、各类敌人、炮台、摆锤、移动平台、开关砖、水域和风场。巫师和公主不可放置，Bean 出现在起点标记处。双击物件可调参数，面板只列出该物件支持的项目，例如：
  - 敌人：行走、跳跃、游动速度；一直活动 / 等 Bean 靠近才启动 / 只在 Bean 附近活动；被打败时翻转某个开关组；被踩时的反弹力度。
  - 炮台：方向、发射间隔、子弹速度、散射、瞄准转向速度。
  - 移动平台和巡逻敌人：行程、速度、起点位置、先往哪边走。摆锤：摆动时间、摆动角度、初始角度、先往哪边摆。绕圈：半径、一圈时间、起始角度。
  - 碎砖：撞几下碎、哪些方向能撞碎。尖刺：跟随开关组伸缩。碎云：重生时间。
  - 风：方向、力度、速度上限、远端剩余力度（渐变风）。水：浮力和阻力。物理方块和球：重量、弹性、摩擦、是否受重力、能否翻滚。弹簧、蹦床、跷跷板、旋转平台、助手鸟、存档灯和粒子区域也各有设置。
  - 隐形墙新增只挡移动物件的一种，与挡敌人、挡 Bean 的两种并列。
- 关卡设置包括名称、主题、背景、音乐、天气、长度/高度、金鞋步数、普通步数、宝石要求数量、Bean 的移动速度和跳跃高度（50%–200%）、天气强度，以及摄像机（横竖都跟随 / 只横向 / 只纵向，可设镜头偏移，和原版关卡一样）。
- Enter/F5 从起点试玩，Shift+Enter 从光标处试玩（光标在砖块里时会自动抬到砖块上方）；试玩中按 F5 或 Tab 返回编辑器。从起点试玩通关后，关卡会标记为"已验证"，并提示这次用了几跳。
- 编辑器内按 F1 可查看全部操作说明，包括撤销/重做、复制/粘贴、吸管、矩形填充、缩放和碰撞显示。鼠标停在工具栏按钮上会显示快捷键；在设置面板和物件参数面板里，对着数值滚动鼠标滚轮即可调整。

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
