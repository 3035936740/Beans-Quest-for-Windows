"""Verify the pinned 2.2.0 source and record the explicit XP portability diff."""

from pathlib import Path
import argparse
import difflib
import hashlib
import json
import zipfile

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / "third_party/box2d220"
ARCHIVE_SHA = "1b298d30d8eb562fbe2a16cf16a6ec0ffc85688b13d4c0a490850c0eac6b9f31"
URL = "https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/box2d/Box2D_v2.2.0.zip"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", action="store_true")
    args = parser.parse_args()
    if args.record:
        archive = ROOT / "build/Box2D_v2.2.0.zip"
        assert hashlib.sha256(archive.read_bytes()).hexdigest() == ARCHIVE_SHA
        files = []
        diff = []
        with zipfile.ZipFile(archive) as z:
            for p in sorted((BASE / "Box2D").rglob("*")):
                if not p.is_file():
                    continue
                relative = p.relative_to(BASE).as_posix()
                upstream = z.read("Box2D_v2.2.0/Box2D/" + relative)
                local = p.read_bytes()
                changed = upstream.replace(b"\r\n", b"\n") != local.replace(b"\r\n", b"\n")
                files.append(
                    dict(
                        file=relative,
                        upstream_sha256=hashlib.sha256(upstream).hexdigest(),
                        local_sha256=hashlib.sha256(local).hexdigest(),
                        modified=changed,
                    )
                )
                if changed:
                    diff.extend(
                        difflib.unified_diff(
                            upstream.decode().splitlines(),
                            local.decode().splitlines(),
                            fromfile="upstream/" + relative,
                            tofile="local/" + relative,
                            lineterm="",
                        )
                    )
        record = dict(
            version="2.2.0",
            native_version_evidence="libBeansQuest.so: b2_version = 2,2,0; WorldToPhysicsScale = 1/64",
            url=URL,
            archive_sha256=ARCHIVE_SHA,
            files=files,
        )
        (BASE / "SOURCE.json").write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
        (BASE / "PORTING.diff").write_text("\n".join(diff) + "\n", encoding="utf-8")
    record = json.loads((BASE / "SOURCE.json").read_text(encoding="utf-8"))
    for entry in record["files"]:
        assert (
            hashlib.sha256((BASE / entry["file"]).read_bytes()).hexdigest() == entry["local_sha256"]
        ), entry["file"]
    assert {p.relative_to(BASE).as_posix() for p in (BASE / "Box2D").rglob("*") if p.is_file()} == {
        f["file"] for f in record["files"]
    }
    print(
        "PASS pinned Box2D 2.2.0:",
        len(record["files"]),
        "source files;",
        sum(f["modified"] for f in record["files"]),
        "explicit portability changes; all hashes match.",
    )


if __name__ == "__main__":
    main()
