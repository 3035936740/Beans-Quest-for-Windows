"""Merge explicit human review notes into the whole-library function ledger.

Unlisted functions always remain pending. Even implemented_checked is a local
claim about one function, never completion of its class or the game.
"""

from collections import Counter, defaultdict
from pathlib import Path
import csv
import argparse
import hashlib
import json

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "build/native-review"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check-complete",
        action="store_true",
        help="Require complete function decisions and original-vs-port/XP validation",
    )
    args = parser.parse_args()
    notes = json.loads((ROOT / "tools/native_review.json").read_text(encoding="utf-8"))
    digest = hashlib.sha256((ROOT / "original/libBeansQuest.so").read_bytes()).hexdigest()
    assert notes["library_sha256"] == digest, "Review notes belong to a different native library"
    functions = json.loads((OUTPUT / "functions.json").read_text(encoding="utf-8"))
    groups = defaultdict(list)
    known = {f["address"] for f in functions}
    assert set(notes["functions"]) <= known, "A reviewed address does not exist in this library"
    for f in functions:
        review = notes["functions"].get(
            f["address"],
            dict(
                status="pending",
                port="",
                evidence="",
                remaining="Semantic review and port correspondence required.",
            ),
        )
        f["semantic_review"] = review
        groups[f["group"]].append(f)
    with (OUTPUT / "function-review.tsv").open("w", encoding="utf-8", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t")
        writer.writerow(
            ("address", "size", "group", "function", "status", "port", "evidence", "remaining")
        )
        for f in functions:
            r = f["semantic_review"]
            writer.writerow(
                (
                    f["address"],
                    f["size"],
                    f["group"],
                    f["name"],
                    r["status"],
                    r["port"],
                    r["evidence"],
                    r["remaining"],
                )
            )
    totals = Counter(f["semantic_review"]["status"] for f in functions)
    accepted = {
        "implemented_checked",
        "platform_replaced_checked",
        "dependency_replaced_checked",
        "unused_proven",
    }
    unresolved = sum(f["semantic_review"]["status"] not in accepted for f in functions)
    required = (
        "all_50_original_solution_playthroughs",
        "original_frame_and_audio_comparison",
        "xp_runtime_verified",
    )
    missing = [key for key in required if notes.get("validation", {}).get(key) is not True]
    complete = unresolved == 0 and not missing
    summary = json.loads((OUTPUT / "summary.json").read_text(encoding="utf-8"))
    report = [
        "# libBeansQuest.so 全库核对",
        "",
        "**尚未完成完整复刻。** 全库清点、反汇编与逐函数语义核对分开计数，未核对的函数不会自动标记为完成。",
        "",
        f"原库 SHA256：`{digest}`",
        "",
        f"已清点 {summary['function_symbols']} 个定义函数符号、{len(functions)} 个不同函数地址（含零长度符号推断范围），解码 {summary['instruction_count']:,} 条指令；函数范围内未解码字节 {summary['undecoded_bytes']}。",
        "",
        f"其中 {summary.get('zero_size_function_symbols',0)} 个函数符号没有声明长度：已有同地址函数时保留别名，否则按下一函数或代码节末尾推断范围，未作语义完成判定。",
        "",
        "完整产物位于 `build/native-review`：函数与别名、调用和跳转、间接调用、常量池、按模块划分的汇编、代码段外余量、全符号、节、重定位与虚表清单。",
        "",
        "`function-review.tsv` 为逐函数台账。`tools/native_review.json` 为人工核对依据；未列出的函数一律 pending。",
        "",
        "当前语义核对状态：" + ", ".join(f"`{k}` {n}" for k, n in sorted(totals.items())) + "。",
        "",
        "- `pending`：尚未完成语义核对，不能认定已移植。",
        "- `inspected_missing`：已读到具体行为，当前实现缺失。",
        "- `partial`：已核对部分逻辑，仍有差异。",
        "- `implemented_checked`：该函数对应逻辑已有具体回归验证；不代表整个类已完成。",
        "",
        "Windows 的输入、窗口、音频与 OpenGL 后端可以采用平台适配实现，但仍需逐项证明上层可观察行为一致；不能将整个依赖模块直接豁免。",
        "",
        "## 模块台账",
        "",
        "| 模块 | 函数地址数 | pending | 部分核对/缺失 | 已实现并检查 |",
        "| --- | ---: | ---: | ---: | ---: |",
    ]
    for name, fs in sorted(groups.items()):
        counts = Counter(f["semantic_review"]["status"] for f in fs)
        report.append(
            f"| {name} | {len(fs)} | {counts['pending']} | {counts['partial']+counts['inspected_missing']} | {counts['implemented_checked']} |"
        )
    report += [
        "",
        "## 已确认的后续工作",
        "",
        "- FollowObject 的跟随轴、持续死区、视口偏移和镜头快照已接入；仍需核对插值、运行时换目标和其他镜头控制器。",
        "- 选关最小化已接入原缓动和图层位移，游玩固定镜头；特殊收集完成行为与标题最小化仍待核对。",
        "- HelperBird / Perch 的外部伤害与多鸟边界、FrogSpecialBall 随机自转与中断边界、完整 Boss 状态机与片尾变身。",
        "- WaterArea 的原运行轨迹、随机水声与完整过滤掩码；选关背景已接入独立 Box2D 和同一水区方程。",
        "- 其余函数须按台账逐项核对配置读取、状态更新、接触、动画、音频、存档与调用关系。",
        "- 最后仍需 50 关按原解法对照、原版画面/音频对照，以及 XP 运行验证。",
        "",
        "## 重建核对产物",
        "",
        "```powershell",
        "python tools/native_inventory.py",
        "python tools/native_review.py",
        "python tools/check_native_inventory.py",
        "python tools/native_review.py --check-complete",
        "```",
        "",
        "`--check-complete` 在仍有未闭合函数或通关/画面音频/XP 验证时返回非零；它与生成清单成功、普通自检通过是不同的检查。",
        "",
        "这些工具需要本机 Python、pyelftools、capstone 和 GNU c++filt；不属于游戏运行依赖，也不写入个人存档。",
        "",
    ]
    (ROOT / "NATIVE_REVIEW.md").write_text("\n".join(report), encoding="utf-8")
    (OUTPUT / "semantic-summary.json").write_text(
        json.dumps(dict(totals), indent=2), encoding="utf-8"
    )
    print("Whole-library semantic review status:", dict(totals))
    gate = dict(complete=complete, unresolved_functions=unresolved, missing_validation=missing)
    (OUTPUT / "completion-gate.json").write_text(json.dumps(gate, indent=2), encoding="utf-8")
    print(
        "Full reconstruction is complete." if complete else "Full reconstruction is NOT complete."
    )
    if args.check_complete and not complete:
        raise SystemExit(2)


if __name__ == "__main__":
    main()
