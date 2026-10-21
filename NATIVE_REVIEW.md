# libBeansQuest.so 全库核对

**尚未完成完整复刻。** 全库清点、反汇编与逐函数语义核对分开计数，未核对的函数不会自动标记为完成。

原库 SHA256：`43f323946cc9c9d26ae9ca613e5ca6c87c38b447cd48eb172b19e326cb5005f8`

已清点 8743 个定义函数符号、8709 个不同函数地址（含零长度符号推断范围），解码 1,016,382 条指令；函数范围内未解码字节 0。

其中 16 个函数符号没有声明长度：已有同地址函数时保留别名，否则按下一函数或代码节末尾推断范围，未作语义完成判定。

完整产物位于 `build/native-review`：函数与别名、调用和跳转、间接调用、常量池、按模块划分的汇编、代码段外余量、全符号、节、重定位与虚表清单。

`function-review.tsv` 为逐函数台账。`tools/native_review.json` 为人工核对依据；未列出的函数一律 pending。

当前语义核对状态：`implemented_checked` 7, `partial` 536, `pending` 8166。

- `pending`：尚未完成语义核对，不能认定已移植。
- `inspected_missing`：已读到具体行为，当前实现缺失。
- `partial`：已核对部分逻辑，仍有差异。
- `implemented_checked`：该函数对应逻辑已有具体回归验证；不代表整个类已完成。

Windows 的输入、窗口、音频与 OpenGL 后端可以采用平台适配实现，但仍需逐项证明上层可观察行为一致；不能将整个依赖模块直接豁免。

## 模块台账

| 模块 | 函数地址数 | pending | 部分核对/缺失 | 已实现并检查 |
| --- | ---: | ---: | ---: | ---: |
| App.Achievements | 13 | 13 | 0 | 0 |
| App.Actlet | 10 | 10 | 0 | 0 |
| App.ActletMatrix | 37 | 29 | 8 | 0 |
| App.ActletVertex | 2 | 2 | 0 | 0 |
| App.Actor | 35 | 23 | 12 | 0 |
| App.AlternatorBlockBehaviour | 11 | 8 | 3 | 0 |
| App.AlternatorSwitchBehaviour | 13 | 6 | 7 | 0 |
| App.AlternatorSwitchIconBehaviour | 7 | 7 | 0 | 0 |
| App.AlternatorTimerSwitchBehaviour | 14 | 14 | 0 | 0 |
| App.AlternatorTouchBlockBehaviour | 14 | 5 | 9 | 0 |
| App.Animation | 9 | 8 | 1 | 0 |
| App.AnimationEffectBehaviour | 11 | 11 | 0 | 0 |
| App.AnimationInstance | 24 | 18 | 6 | 0 |
| App.AnimationSet | 10 | 10 | 0 | 0 |
| App.AnimationSetCache | 5 | 4 | 1 | 0 |
| App.AnimationToggleBehaviour | 10 | 10 | 0 | 0 |
| App.AutoVariable | 2 | 2 | 0 | 0 |
| App.BackgroundBehaviour | 10 | 8 | 2 | 0 |
| App.BasePhysicsBehaviour | 16 | 12 | 4 | 0 |
| App.BeanDustBehaviour | 9 | 9 | 0 | 0 |
| App.BeanGameHelper | 4 | 3 | 1 | 0 |
| App.BeanLevelStats | 33 | 27 | 6 | 0 |
| App.Behaviour | 91 | 91 | 0 | 0 |
| App.BehaviourData | 7 | 7 | 0 | 0 |
| App.BehaviourFactory | 270 | 264 | 6 | 0 |
| App.BossActivateBehaviour | 11 | 11 | 0 | 0 |
| App.Box | 22 | 19 | 3 | 0 |
| App.BoxGroupPhysicsBehaviour | 7 | 6 | 1 | 0 |
| App.BoxPhysicsBehaviour | 9 | 7 | 2 | 0 |
| App.BreakableBehaviour | 15 | 9 | 6 | 0 |
| App.BugBaddieBehaviour | 16 | 16 | 0 | 0 |
| App.BulletSource | 14 | 4 | 10 | 0 |
| App.ButtonBaseBehaviour | 27 | 27 | 0 | 0 |
| App.ButtonBehaviour | 11 | 11 | 0 | 0 |
| App.Camera | 20 | 13 | 6 | 1 |
| App.CameraOffsetBehaviour | 8 | 8 | 0 | 0 |
| App.CheckpointBehaviour | 11 | 8 | 3 | 0 |
| App.CirclePhysicsBehaviour | 9 | 7 | 2 | 0 |
| App.CoinBehaviour | 6 | 6 | 0 | 0 |
| App.CoinGroupBehaviour | 7 | 6 | 1 | 0 |
| App.CollectableObjectBehaviour | 9 | 5 | 4 | 0 |
| App.CollectableObjectGroupBehaviour | 11 | 8 | 3 | 0 |
| App.CollectableSalamanderBehaviour | 6 | 5 | 1 | 0 |
| App.CollisionWorld | 16 | 10 | 6 | 0 |
| App.ConfigOptions | 13 | 13 | 0 | 0 |
| App.ControlSchemeButton | 8 | 8 | 0 | 0 |
| App.ControlsKeyButton | 8 | 8 | 0 | 0 |
| App.CrumblingPlatformBehaviour | 19 | 8 | 11 | 0 |
| App.CutsceneEndingController | 7 | 4 | 3 | 0 |
| App.CutsceneMovieController | 8 | 5 | 3 | 0 |
| App.CutsceneOpeningController | 12 | 11 | 1 | 0 |
| App.DeadlyBehaviour | 15 | 13 | 2 | 0 |
| App.DestructibleBlockBehaviour | 9 | 9 | 0 | 0 |
| App.DeveloperCommentsController | 6 | 6 | 0 | 0 |
| App.DeveloperCommentsToggleBehaviour | 8 | 8 | 0 | 0 |
| App.DiagonalBehaviour | 11 | 0 | 10 | 1 |
| App.EndOfGameController | 7 | 7 | 0 | 0 |
| App.EndOfGameCutsceneController | 8 | 5 | 3 | 0 |
| App.EndlessController | 14 | 14 | 0 | 0 |
| App.EnemyBulletBehaviour | 16 | 8 | 8 | 0 |
| App.EnemyJumperBehaviour | 17 | 8 | 9 | 0 |
| App.ExplosionBehaviour | 8 | 8 | 0 | 0 |
| App.FadeTask | 35 | 35 | 0 | 0 |
| App.FinalBossBehaviour | 20 | 0 | 19 | 1 |
| App.FinalBossBehaviourArm | 5 | 0 | 5 | 0 |
| App.FinalBossBehaviourEnemy | 6 | 0 | 6 | 0 |
| App.FinalBossBehaviourEnemySpawnable_std | 1 | 0 | 1 | 0 |
| App.FinalBossBehaviourHead | 4 | 0 | 4 | 0 |
| App.FinalBossBehaviourObstacle | 4 | 0 | 4 | 0 |
| App.FinalBossBulletBehaviour | 17 | 9 | 8 | 0 |
| App.FixedUpdateSmoother | 13 | 13 | 0 | 0 |
| App.FollowCameraBehaviour | 6 | 6 | 0 | 0 |
| App.FollowObjectBehaviour | 11 | 5 | 6 | 0 |
| App.ForceAreaBehaviour | 10 | 0 | 10 | 0 |
| App.FrogSpecialBallBehaviour | 13 | 6 | 7 | 0 |
| App.Game | 9 | 9 | 0 | 0 |
| App.GfxApplyButton | 6 | 6 | 0 | 0 |
| App.GfxResolutionButton | 10 | 10 | 0 | 0 |
| App.GfxWindowModeButton | 10 | 10 | 0 | 0 |
| App.HUDController | 23 | 18 | 5 | 0 |
| App.HUDMenuBehaviour | 9 | 9 | 0 | 0 |
| App.HelperBirdBehaviour | 19 | 7 | 11 | 1 |
| App.HelperBirdPerchBehaviour | 8 | 5 | 3 | 0 |
| App.IActletMatrixChangeListener | 2 | 2 | 0 | 0 |
| App.IActorChangeListener | 2 | 2 | 0 | 0 |
| App.IAlternationTarget | 2 | 2 | 0 | 0 |
| App.IBehaviour | 2 | 2 | 0 | 0 |
| App.IBehaviourFactory | 2 | 2 | 0 | 0 |
| App.IBullet | 5 | 5 | 0 | 0 |
| App.IExplodable | 2 | 2 | 0 | 0 |
| App.IInputVector | 2 | 2 | 0 | 0 |
| App.IKillableEnemyAffector | 2 | 2 | 0 | 0 |
| App.IObject | 2 | 2 | 0 | 0 |
| App.IPhysicsObject | 2 | 2 | 0 | 0 |
| App.InputButton | 17 | 17 | 0 | 0 |
| App.InputHelper | 20 | 20 | 0 | 0 |
| App.JumpPotionBehaviour | 6 | 6 | 0 | 0 |
| App.KillableEnemyBehaviour | 16 | 7 | 9 | 0 |
| App.LevelBackdrop | 10 | 9 | 1 | 0 |
| App.LevelButtonBehaviour | 11 | 11 | 0 | 0 |
| App.LevelChangeTimerController | 11 | 11 | 0 | 0 |
| App.LevelCompleteContinueButton | 6 | 5 | 1 | 0 |
| App.LevelCompleteController | 9 | 3 | 6 | 0 |
| App.LevelCompleteRestartButton | 6 | 5 | 1 | 0 |
| App.LevelEndBehaviour | 10 | 7 | 3 | 0 |
| App.LevelGeometry | 12 | 11 | 1 | 0 |
| App.LevelLayer | 24 | 22 | 2 | 0 |
| App.LevelSelectButton | 6 | 6 | 0 | 0 |
| App.LevelSelectController | 8 | 6 | 2 | 0 |
| App.LevelSelectMinimizeButton | 7 | 7 | 0 | 0 |
| App.LevelStartController | 10 | 4 | 6 | 0 |
| App.LevelTask | 55 | 42 | 13 | 0 |
| App.LevelWarpBehaviour | 8 | 8 | 0 | 0 |
| App.MagnetBehaviour | 9 | 7 | 2 | 0 |
| App.Menu | 9 | 9 | 0 | 0 |
| App.MusicToggleBehaviour | 10 | 10 | 0 | 0 |
| App.Object | 32 | 22 | 10 | 0 |
| App.ObjectData | 14 | 13 | 1 | 0 |
| App.OneShotAnimationBehaviour | 9 | 9 | 0 | 0 |
| App.OneWayPlatform | 8 | 4 | 4 | 0 |
| App.OrbitBehaviour | 13 | 7 | 6 | 0 |
| App.OrbitingObjectBehaviour | 12 | 8 | 4 | 0 |
| App.OscillatingBehaviour | 15 | 0 | 15 | 0 |
| App.PanBehaviour | 7 | 7 | 0 | 0 |
| App.Panel | 10 | 10 | 0 | 0 |
| App.PanelBackground | 11 | 11 | 0 | 0 |
| App.PanelButton | 15 | 15 | 0 | 0 |
| App.PanelHeading | 5 | 5 | 0 | 0 |
| App.ParticleDef | 72 | 63 | 8 | 1 |
| App.ParticleDefSettings | 5 | 5 | 0 | 0 |
| App.ParticleEffect | 32 | 24 | 8 | 0 |
| App.ParticleInstance | 9 | 7 | 2 | 0 |
| App.ParticleSystemDef | 10 | 10 | 0 | 0 |
| App.ParticleVertex | 2 | 2 | 0 | 0 |
| App.PauseMenuController | 14 | 5 | 9 | 0 |
| App.PauseRestartBehaviour | 9 | 9 | 0 | 0 |
| App.PauseResumeBehaviour | 9 | 9 | 0 | 0 |
| App.PersistentData | 27 | 27 | 0 | 0 |
| App.PhysicsDebugDrawer | 11 | 11 | 0 | 0 |
| App.PhysicsObject | 27 | 20 | 7 | 0 |
| App.PhysicsObjectEventListener | 13 | 13 | 0 | 0 |
| App.PinballBouncerBehaviour | 11 | 6 | 5 | 0 |
| App.PlatformBehaviour | 9 | 8 | 1 | 0 |
| App.PlayerBehaviour | 8 | 8 | 0 | 0 |
| App.Project | 36 | 36 | 0 | 0 |
| App.ProximityActivateBehaviour | 10 | 5 | 5 | 0 |
| App.QuitButtonBehaviour | 7 | 7 | 0 | 0 |
| App.RandomAnimationBehaviour | 8 | 8 | 0 | 0 |
| App.ResetButtonBehaviour | 8 | 8 | 0 | 0 |
| App.RotatingBehaviour | 6 | 5 | 1 | 0 |
| App.SFXToggleBehaviour | 10 | 10 | 0 | 0 |
| App.SafeBallEndBehaviour | 6 | 6 | 0 | 0 |
| App.SaveStateNode | 4 | 4 | 0 | 0 |
| App.SaveStateNode_App | 2 | 2 | 0 | 0 |
| App.SaveStateStore | 7 | 7 | 0 | 0 |
| App.ScreenHoppingBehaviour | 10 | 10 | 0 | 0 |
| App.SecretRevealerBehaviour | 8 | 5 | 3 | 0 |
| App.SecretRevealerController | 8 | 5 | 3 | 0 |
| App.SeesawBehaviour | 7 | 5 | 2 | 0 |
| App.SpeedPotionBehaviour | 6 | 6 | 0 | 0 |
| App.SpikeBehaviour | 14 | 14 | 0 | 0 |
| App.SpikeGroupBehaviour | 11 | 7 | 4 | 0 |
| App.SpringBehaviour | 11 | 6 | 5 | 0 |
| App.TextObject | 23 | 23 | 0 | 0 |
| App.TimeBombBehaviour | 13 | 13 | 0 | 0 |
| App.TitleScreenController | 8 | 5 | 3 | 0 |
| App.ToggleableButtonBehaviour | 11 | 11 | 0 | 0 |
| App.TomPlayerBehaviour | 37 | 15 | 20 | 2 |
| App.TrackingDetector | 10 | 7 | 3 | 0 |
| App.TrampolineBehaviour | 8 | 7 | 1 | 0 |
| App.TransitionAnimation | 35 | 35 | 0 | 0 |
| App.TurboToggleBehaviour | 8 | 8 | 0 | 0 |
| App.UIOutline | 11 | 11 | 0 | 0 |
| App.UITask | 20 | 20 | 0 | 0 |
| App.UnbeanPotionBehaviour | 6 | 4 | 2 | 0 |
| App.WalkBehaviour | 16 | 4 | 12 | 0 |
| App.WaterAreaBehaviour | 12 | 4 | 8 | 0 |
| App.WebLinkButtonBehaviour | 7 | 7 | 0 | 0 |
| App.WorldButtonBehaviour | 12 | 12 | 0 | 0 |
| App.WorldLevelButtonBehaviour | 12 | 12 | 0 | 0 |
| App.WorldSelectController | 10 | 7 | 3 | 0 |
| App.WreckingBallControllerBehaviour | 11 | 5 | 6 | 0 |
| App.WreckingBallEndBehaviour | 6 | 6 | 0 | 0 |
| Box2D | 433 | 425 | 8 | 0 |
| CxxRuntime | 2410 | 2386 | 24 | 0 |
| PlatformAndOther | 3113 | 3079 | 34 | 0 |
| ZAudio | 44 | 42 | 2 | 0 |
| ZRenderer | 74 | 73 | 1 | 0 |
| ZUtil | 121 | 115 | 6 | 0 |

## 已确认的后续工作

- FollowObject 的跟随轴、持续死区、视口偏移和镜头快照已接入；仍需核对插值、运行时换目标和其他镜头控制器。
- 选关最小化已接入原缓动和图层位移，游玩固定镜头；特殊收集完成行为与标题最小化仍待核对。
- HelperBird / Perch 的外部伤害与多鸟边界、FrogSpecialBall 随机自转与中断边界、完整 Boss 状态机与片尾变身。
- WaterArea 的原运行轨迹、随机水声与完整过滤掩码；选关背景已接入独立 Box2D 和同一水区方程。
- 其余函数须按台账逐项核对配置读取、状态更新、接触、动画、音频、存档与调用关系。
- 最后仍需 50 关按原解法对照、原版画面/音频对照，以及 XP 运行验证。

## 重建核对产物

```powershell
python tools/native_inventory.py
python tools/native_review.py
python tools/check_native_inventory.py
python tools/native_review.py --check-complete
```

`--check-complete` 在仍有未闭合函数或通关/画面音频/XP 验证时返回非零；它与生成清单成功、普通自检通过是不同的检查。

这些工具需要本机 Python、pyelftools、capstone 和 GNU c++filt；不属于游戏运行依赖，也不写入个人存档。


Helper checkpoint/audio bounded follow-up: build/helper-audio-findings.md. Snapshot28 primary layout retained; portable one-way traversal supplement and explicit legacy compatibility added. User-requested enemy side-impact chirp is an intentional addition to the native silent branch. Wizard death per-event fade restored from static native evidence. See final frozen validation reports; full semantic/native-runtime audit remains open.


Helper collision scope follow-up: build/helper-collision-scope-findings.md. Original class filters, all represented physical collider classes, actual projectile pools and one-way pass-through are checked in owned campaign/UI worlds. Spike affector impact audio is a user-requested addition; nonfollowing helper contact opt-out ordering corrected. Whole native semantic audit remains open.


Helper ending pickup presentation follow-up: build/helper-pickup-presentation-findings.md. Original ending helper now shares domain-owned pickup particles/sound with campaign and follows its own Bean facing; native StopFollowing silence is preserved. The existing Windows ending mute is retained here and still requires comparison with original scene/object mute dispatch. Controlled native-resource contact checks do not establish full original runtime equivalence.


Ending object-mute follow-up: build/ending-object-mute-findings.md. Earlier ending-mute uncertainty is resolved by OnActivate virtual dispatch and Object/Actor/ParticleEffect vtables. Preserve native silence for new scene requests while removing the Windows global user/mixer toggle that froze an accepted voice in deterministic real-contact interleaving. Existing playback, user preference and headphone silence compatibility remain active; full generic object mute and native runtime equivalence remain partial.


Touch cooldown LoadState follow-up: build/touch-cooldown-load-findings.md. Same-object checkpoint/reset load retains current unsaved touch guard and solved latch, confirmed through native Touch/Box/BasePhysics and LevelTask/Actor/Object paths. Remove blanket0.1 physics resets; fresh construction still initializes0.1. New real-contact/fade/disk/fresh-process checks and corrected older oracle preserve Snapshot28. Whole native semantic audit remains open.


Saved mechanism event follow-up: build/mechanism-event-checkpoint-findings.md. Ordinary switches and breakables restore their SAVED pending flag/damage, distinct from touch-block current unsaved fields. Finite tagged unused aux slots preserve Snapshot28 and legacy no-event defaults; actual full-tick, disk and fresh-process controls cover both original static classes. General native store/resource/runtime equivalence remains partial.


Legacy switch idle follow-up: build/switch-legacy-idle-findings.md. Remove the port-only positive-aux delayed toggle, reproduced after real ON-switch legacy restore. Two-second actual disk/full-tick controls strengthen the previous two-update OFF-only legacy oracle. Saved pending-event ownership and Snapshot28 stay unchanged; whole native semantic audit remains open.


4-2 helper-flight camera follow-up: build/helper-camera-restore-findings.md. Actual continuing ascent exposed a camera-only wait after generic dead-zone recentering. Following-helper checkpoints now retain their established tracking phase across viewport sizes; nonflight/native resource-order and Boss camera paths remain separate. Original helper-specific native load scheduling is not established by this Windows continuity repair; whole original audit remains open.

Creature carrying and cloud-seam follow-up: build/creature-support-findings.md. Preserve real one-way top support under mixed collision impulses; new underside entry remains one-way. Per explicit user request, driven kinematic compression kills dynamic KillableEnemy creatures through the normal death queue. Finite-mass stacking, free carrying, authored immunity, jump settings and Snapshot28 remain unchanged. Full original-device equivalence remains partial.

Potion frame and selector audio correction: each original potion frame is copied byte-for-byte to matching local texture coordinates in the GL cache to prevent fractional-scale atlas sampling from moving its silhouette. Entity geometry, collection, animation timing and shipped atlas remain unchanged. Minimized level-select ordinary terrain bounces now play jump audio; expanded menus remain quiet and the SFX toggle is honored. Five selector scenes, transition state, real physics launches and mixed audio samples verified. Prior full suite:3112 passes; current targeted reports are recorded separately, not a new full-suite claim.

Responsive backdrop follow-up: all74 authored scenes audited;101 geometry-guarded backdrop policies across62 scenes, including31 scenery bands. Extend nonzero parallax-layer mountains/woods, constant fog strips and flipped ceiling scenery. Reuse shared atlas motifs and reflect the one nonperiodic single landscape; preserve isolated palms/columns/logos. Beyond ordinary widths, widen distant silhouettes to limit repetition to about three full large image periods plus clipped edges. Sparse clouds retain size and move across wider spacing; the existing four-piece bank remains continuous without extra actors. Pixel checks cover651 width/camera cases, tall/ceiling edges, original ordinary-width pixels and foreground identity. Current targeted108 checks pass; this is Windows viewport adaptation, not a new claim of full Android equivalence.

Backdrop region/sparse-pattern follow-up: displayed4-10 Brutal Forces uses an eight-column/five-row occupancy motif rather than a solid rectangular grid. All208 authored brick cells and32 openings are preserved. Sparse motifs repeat outside their original area; ultra-wide uniform scaling controls full motif repetition without stretching brick aspect ratio. Staged story scenes with EndOfGameCutsceneController retain local parallax artwork bounds, preventing the later courtyard skyline from wrapping into the Boss room. Authored later-area artwork and story activation remain untouched. Current targeted140 checks pass, including630 horizon cases,15 sparse-pattern viewport/camera cases with840 sampled cells, Boss/local-courtyard pixel identity and ending collectable/transformation checks. Whole original-device equivalence remains partial.

Boss courtyard spatial boundary correction: the previous region release only removed background extension and did not resolve shared-layer parallax displacement. The three exact skyline panels x4384..6208 share .75 horizontal parallax with arena pillars. Their total1824-unit width equals the final world-space sky region x6176..8000. Rendering now anchors only these UV/geometry-guarded panels to that region via +1792 world units and the foreground transform, leaving other artwork and physics unchanged. Actual OpenGL tests cover40 viewport/camera cases across five Boss arenas and later courtyard, widths512..16384; all skyline pixels stay in the final region. The reference courtyard camera7168 retains exact original pixels. Original asset files and tile vertex bytes are unchanged. Current142 targeted checks pass; this is not a current full-suite or whole-original-device equivalence claim.

Cloud support handoff follow-up: 3117 complete regression checks pass with zero failures on345 frozen source inputs. Previous coarse ordinary traversal/mixed-wave diagnostics did not reproduce the new screenshots. Two controlled actual-Box2D contact regressions do reproduce implementation defects: moving the supported actor into0.2px top penetration at a newly reached neighboring fixture permanently refused that fixture; with10px of collider overlap remaining the feet fell toY62.626 below theY80 surface. Corrected neighboring stationary coplanar support continuity keeps the same case atY80.927 or higher. A retained top-support pair also incorrectly enabled an underside manifold and stopped a60px/s ascent; the signed top normal now rejects that underside, preserving60px/s. These are Windows continuity corrections, not new branches attributed to the original ARM library. The handoff requires existing solved support, connected horizontal tops, feet within2 Box2D linear slops, stationary surfaces and the same owning world. Real gaps, unequal heights, moving platforms and underside entry remain native-gated. Forty-eight physical cases cover all4Walk fixture classes in both directions and six step profiles, with192 negative controls;288 repeated traversal/impulse cases and12 actual solo/wave cloud-endpoint contexts pass. Normal first-cloud-to-spring descent is retained. Shared campaign and UI/credits callbacks use the correction. Authored fixtures, animation, speed, gravity, save layouts and original assets are unchanged. This controlled reproduction is not a recording/replay of the precise user session; whole original-device equivalence remains partial.
