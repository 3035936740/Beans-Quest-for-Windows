"""Read-only APK/source inventory for reviewing reconstruction gaps.

Literal references are navigation aids, not proof of a complete implementation.
This does not run the game or modify the executable or the player's saves.
"""

from collections import defaultdict
from pathlib import Path
import gzip
import hashlib
import json
import re
import zipfile

import resources

ROOT = Path(__file__).resolve().parents[1]


def digest(data):
    return hashlib.sha256(data).hexdigest()


def stage_label(props):
    world = int(props.get("bean-world", 0))
    world = {4: 2, 2: 3, 3: 4}.get(world, world)
    return f"{world}-{int(props.get('bean-level', 0)):02d} {props.get('bean-level-name', props['name'])}"


def main():
    sources = {
        p.relative_to(ROOT).as_posix(): p.read_text(encoding="utf-8-sig")
        for p in sorted((ROOT / "src").glob("*"))
        if p.suffix in (".h", ".cpp")
    }
    source = "\n".join(sources.values())
    references = defaultdict(list)
    for path, content in sources.items():
        for number, line in enumerate(content.splitlines(), 1):
            for name in re.findall(r'"([A-Za-z]+(?:Behaviour|Controller|Settings|Button))"', line):
                references[name].append({"file": path, "line": number})

    project = resources.project()
    apk = zipfile.ZipFile(ROOT / "Bean's Quest.apk")
    # Confirm that the parser's extracted inputs still match the supplied APK.
    for file in ["project.pgame"] + ["Levels/" + v["file"] for v in project["levels"].values()]:
        packed = apk.read("assets/Media/" + file)
        unpacked = gzip.decompress(packed) if packed.startswith(b"\x1f\x8b") else packed
        if (ROOT / "original/Media" / file).read_bytes() != unpacked:
            raise RuntimeError("Extracted audit input differs from APK: " + file)

    behaviors = defaultdict(
        lambda: {
            "scene_occurrences": 0,
            "gameplay_occurrences": 0,
            "gameplay_stages": set(),
            "source_literal_references": [],
        }
    )
    scenes = []
    initially_off_switches, initial_blocks, turret_offsets = [], [], []
    for uid, info in project["levels"].items():
        level = resources.level(info["file"])
        game = int(info["props"].get("bean-world", 0)) > 0
        label = stage_label(info["props"]) if game else info["props"]["name"]
        scenes.append({"id": str(uid), "name": label, "file": info["file"], "gameplay": game})
        for obj in level["objects"]:
            gameplay_object = game and not level["layers"][obj["layer"]]["flags"][2]
            for behavior in obj["behaviors"]:
                name, props = behavior["type"], behavior["props"]
                item = behaviors[name]
                item["scene_occurrences"] += 1
                if not gameplay_object:
                    continue
                item["gameplay_occurrences"] += 1
                item["gameplay_stages"].add(label)
                location = {"stage": label, "object_id": str(obj["id"]), "file": info["file"]}
                if name == "AlternatorSwitchBehaviour" and props.get("Alternation") == "0":
                    initially_off_switches.append(location)
                if name == "AlternatorBlockBehaviour":
                    initial_blocks.append(
                        dict(
                            location,
                            initial_alternation=int(props.get("Alternation", "0")),
                            mask=int(props.get("AlternationMask", "0")),
                        )
                    )
                if name == "BulletSource" and props.get("timeOffset"):
                    turret_offsets.append(dict(location, time_offset=props["timeOffset"]))
    for name, item in behaviors.items():
        item["gameplay_stages"] = sorted(item["gameplay_stages"])
        item["source_literal_references"] = references[name]

    # Include conditional literal choices, such as sound(hp ? "hurt" : "death").
    # References remain an inventory aid, not evidence that an event was played.
    called_sounds = {
        name
        for argument in re.findall(r"\bsound\(([^;\n]*)\)", source)
        for name in re.findall(r'"([a-z0-9-]+)"', argument)
    }
    sound_keys = set(project["sounds"])
    matching_resources = 0
    missing_resources, changed_resources = [], []
    for name in apk.namelist():
        if not name.startswith("assets/Media/") or not name.endswith(
            (".pgame", ".plevel", ".panims", ".ptex", ".ttf")
        ):
            continue
        packed = apk.read(name)
        unpacked = gzip.decompress(packed) if packed.startswith(b"\x1f\x8b") else packed
        target = ROOT / "bin/data" / name.removeprefix("assets/")
        if not target.is_file():
            missing_resources.append(name)
        elif target.read_bytes() != unpacked:
            changed_resources.append(name)
        else:
            matching_resources += 1

    packages = []
    executable = (ROOT / "bin/BeansQuest.exe").read_bytes()
    for path in [ROOT / "BeanQuest-Windows-x86.zip", ROOT / "bin/BeanQuest-Windows-x86.zip"]:
        if not path.exists():
            continue
        with zipfile.ZipFile(path) as package:
            name = next((n for n in package.namelist() if n.endswith("/BeansQuest.exe")), None)
            packages.append(
                {
                    "file": path.relative_to(ROOT).as_posix(),
                    "executable_matches_bin": bool(name and package.read(name) == executable),
                }
            )

    report = {
        "method": "APK binary resources, current C++ source, and local package comparison",
        "caveat": "A behavior-name reference or a matching resource does not establish behavioral parity.",
        "apk_sha256": digest((ROOT / "Bean's Quest.apk").read_bytes()),
        "source_sha256": {path: digest(content.encode()) for path, content in sources.items()},
        "scene_count": len(scenes),
        "gameplay_stage_count": sum(s["gameplay"] for s in scenes),
        "scenes": scenes,
        "behaviors": dict(sorted(behaviors.items())),
        "switches_with_initial_alternation_zero": initially_off_switches,
        "regular_alternator_blocks": initial_blocks,
        "turrets_with_authored_phase_offsets": turret_offsets,
        "sounds": {
            "original_count": len(sound_keys),
            "called_original_count": len(called_sounds & sound_keys),
            "unreferenced_original_keys": sorted(sound_keys - called_sounds),
            "calls_without_original_asset": sorted(called_sounds - sound_keys),
        },
        "resources": {
            "matching": matching_resources,
            "missing": missing_resources,
            "changed": changed_resources,
        },
        "standalone_movie_candidates": [
            n
            for n in apk.namelist()
            if n.lower().endswith((".mp4", ".m4v", ".avi", ".mov", ".webm", ".3gp", ".ogv"))
        ],
        "packages": packages,
        "validation_limit": "No original-vs-port playthrough or XP machine validation was performed by this audit.",
    }
    output = ROOT / "build/port-audit.json"
    output.parent.mkdir(exist_ok=True)
    output.write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")
    print(
        f'{len(scenes)} scenes, {report["gameplay_stage_count"]} stages; {matching_resources} matching original resources.'
    )
    print(
        f"{len(initially_off_switches)} switches start with state zero (not a command to turn targets off); {len(turret_offsets)} turrets configure phase offsets."
    )
    print(
        f"{len(sound_keys - called_sounds)} original sound keys have no trigger; {len(called_sounds - sound_keys)} calls lack an original asset."
    )
    print(f"Report: {output}")


if __name__ == "__main__":
    main()
