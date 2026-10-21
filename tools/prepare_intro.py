"""Copy the supplied original MP4 without transcoding or changing its bytes."""

from pathlib import Path
import hashlib, json, shutil

root = Path(__file__).resolve().parents[1]
target = root / "bin/data/Media/Movies/intro.mp4"
candidates = [root / "resources/intro.mp4", root / "data/Media/Movies/intro.mp4", target]
source = next((p for p in candidates if p.is_file()), None)
if source is None:
    raise SystemExit("No intro.mp4 supplied; optional title demo remains unavailable.")
target.parent.mkdir(parents=True, exist_ok=True)
digest = hashlib.sha256(source.read_bytes()).hexdigest()
if source != target and (
    not target.is_file() or hashlib.sha256(target.read_bytes()).hexdigest() != digest
):
    shutil.copy2(source, target)
assert hashlib.sha256(target.read_bytes()).hexdigest() == digest
for old, expected in [
    (
        root / "bin/data/Media/Movies/attract-intro.avi",
        "e13fbc82a7f5d990e4581857cde6affb07e47b79998120cccc112b7d34b3769b",
    ),
    (root / "bin/resources/intro.mp4", digest),
]:
    if old.is_file():
        assert (
            hashlib.sha256(old.read_bytes()).hexdigest() == expected
        ), "Unexpected contents; preserve " + str(old)
        old.unlink()
record = {
    "source": str(source.relative_to(root)),
    "source_sha256": digest,
    "runtime_mp4": "bin/data/Media/Movies/intro.mp4",
    "runtime_sha256": digest,
    "bytes": target.stat().st_size,
    "transcoded": False,
    "resized": False,
    "ffmpeg_runtime_required": False,
}
(root / "build/intro-media-manifest.json").write_text(
    json.dumps(record, indent=2) + "\n", encoding="utf-8"
)
print(json.dumps(record, indent=2))
