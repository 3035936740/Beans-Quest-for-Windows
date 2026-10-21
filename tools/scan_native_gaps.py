"""Whole-library triage, not an assertion that unreviewed code is equivalent.

Join every native function with the explicit semantic ledger and every actual
resource behavior with source references. The resulting priorities direct exact
native reading; name matches alone never close a semantic review item.
"""

from collections import Counter, defaultdict
from pathlib import Path
import hashlib
import json
import re
import resources

ROOT = Path(__file__).resolve().parents[1]


def main():
    native = json.loads((ROOT / "build/native-review/functions.json").read_text(encoding="utf-8"))
    ledger = json.loads((ROOT / "tools/native_review.json").read_text(encoding="utf-8"))
    sha = hashlib.sha256((ROOT / "original/libBeansQuest.so").read_bytes()).hexdigest()
    assert sha == ledger["library_sha256"]
    occurrences = Counter()
    project = resources.project()
    for level in project["levels"].values():
        for obj in resources.level(level["file"])["objects"]:
            occurrences.update(b["type"] for b in obj["behaviors"])
    sources = {
        p.relative_to(ROOT).as_posix(): p.read_text(encoding="utf-8-sig")
        for p in sorted((ROOT / "src").glob("*"))
        if p.suffix in (".h", ".cpp")
    }
    references = defaultdict(list)
    # Include BulletSource, which does not have a Behaviour suffix.
    for path, content in sources.items():
        for number, line in enumerate(content.splitlines(), 1):
            for name in set(re.findall(r'"([A-Za-z][A-Za-z0-9]+)"', line)) & occurrences.keys():
                references[name].append(dict(file=path, line=number))
    groups = defaultdict(list)
    all_functions = []
    for function in native:
        review = ledger["functions"].get(function["address"], {})
        row = dict(
            address=function["address"],
            function=function["name"],
            size=function["size"],
            status=review.get("status", "pending"),
            read_scope=review.get("read_scope", "not_recorded"),
        )
        all_functions.append(row)
        groups[function["group"]].append(row)
    priorities = []
    for group, functions in groups.items():
        behavior = group.removeprefix("App.")
        counts = Counter(f["status"] for f in functions)
        priorities.append(
            dict(
                group=group,
                functions=len(functions),
                semantic_status=dict(counts),
                resource_occurrences=occurrences[behavior],
                source_references=references[behavior],
            )
        )
    priorities.sort(
        key=lambda x: (
            -x["resource_occurrences"],
            -x["semantic_status"].get("pending", 0),
            x["group"],
        )
    )
    report = dict(
        library_sha256=sha,
        function_count=len(native),
        status_counts=dict(Counter(f["status"] for f in all_functions)),
        method="Every function address and resource behavior mechanically joined to explicit review records and C++ references.",
        limitation="This is whole-library triage. Pending functions have NOT thereby been semantically read or implemented; source literals do not prove behavior.",
        source_sha256={p: hashlib.sha256(s.encode()).hexdigest() for p, s in sources.items()},
        priorities=priorities,
        functions=all_functions,
    )
    output = ROOT / "build/native-gap-scan.json"
    output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(
        f"Indexed {len(native)} native functions and {len(occurrences)} configured behavior types."
    )
    print(report["status_counts"])
    for item in priorities:
        if item["resource_occurrences"] and item["semantic_status"].get("pending", 0):
            print(item["group"], item["resource_occurrences"], item["semantic_status"])


if __name__ == "__main__":
    main()
