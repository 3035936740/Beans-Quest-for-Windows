"""Exercise full asset preparation in isolated fixtures; never run FFmpeg."""

from pathlib import Path
from unittest.mock import patch
import argparse
import gzip
import hashlib
import importlib.util
import json
import shutil
import sys
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "prepare_assets_under_test", ROOT / "tools/prepare.py"
)
prepare = importlib.util.module_from_spec(spec)
spec.loader.exec_module(prepare)
import resources

parser = argparse.ArgumentParser()
parser.add_argument("--label", required=True)
args = parser.parse_args()
if not args.label.replace("-", "").isalnum():
    raise SystemExit("Label must contain only letters, numbers and hyphens.")
WORK = ROOT / "build/tests" / ("intro-pipeline-" + args.label)
if WORK.exists():
    raise SystemExit("Use a new fixture label; existing evidence is preserved.")
WORK.mkdir(parents=True)
with zipfile.ZipFile(ROOT / "Bean's Quest.apk") as original:
    project_raw = original.read("assets/Media/project.pgame")
    icon = original.read("res/drawable-xhdpi-v4/icon.png")
    project = gzip.decompress(project_raw) if project_raw[:2] == b"\x1f\x8b" else project_raw
    reader = resources.Reader(project)
    reader.take(16)
    reader.d()
    for _ in range(reader.i()):
        reader.q()
        reader.s()
        reader.d()
    sounds = reader.d()
    tracks = {}
    for _ in range(reader.i()):
        uid = reader.q()
        file = reader.s()
        reader.d()
        tracks[str(uid)] = file
    selected = next((key, file) for key, file in sounds.items() if key == "jump")
    sound_member = (
        "assets/Media/" + ("Samples/" if selected[1].endswith(".ogg") else "Music/") + selected[1]
    )
    sound_raw = original.read(sound_member)
MOVIE = ROOT / "resources/intro.mp4"
MOVIE_HASH = hashlib.sha256(MOVIE.read_bytes()).hexdigest()


class PrepareIntroPipeline(unittest.TestCase):
    def fixture(self, name, movie_in_apk=False, missing_audio=False):
        root = WORK / name
        root.mkdir()
        output = root / "bin"
        audio = output / "data/audio"
        audio.mkdir(parents=True)
        for key in set(sounds) | set(tracks):
            if not missing_audio or key != selected[0]:
                shutil.copy2(ROOT / "bin/data/audio" / (key + ".wav"), audio / (key + ".wav"))
        with zipfile.ZipFile(root / "Bean's Quest.apk", "w") as apk:
            apk.writestr("assets/Media/project.pgame", project_raw)
            apk.writestr("res/drawable-xhdpi-v4/icon.png", icon)
            apk.writestr(sound_member, sound_raw)
            if movie_in_apk:
                apk.write(MOVIE, "assets/Media/Movies/intro.mp4")
        movie = output / "data/Media/Movies/intro.mp4"
        if not movie_in_apk:
            movie.parent.mkdir(parents=True)
            shutil.copy2(MOVIE, movie)
        calls = []

        def ffmpeg(command, **kwargs):
            calls.append(command)
            target = Path(command[-1])
            self.assertEqual(
                target.suffix, ".wav", "Movie preparation must never transcode an MP4 to AVI"
            )
            self.assertEqual(command[command.index("-ac") + 1], "2")
            self.assertEqual(command[command.index("-ar") + 1], "22050")
            self.assertEqual(command[command.index("-c:a") + 1], "pcm_s16le")
            shutil.copy2(ROOT / "bin/data/audio" / (selected[0] + ".wav"), target)

        def run(ffmpeg_available=True):
            with (
                patch.object(prepare, "ROOT", root),
                patch.object(prepare, "OUT", output),
                patch.object(
                    prepare.shutil,
                    "which",
                    return_value="isolated-ffmpeg-stub" if ffmpeg_available else None,
                ),
                patch.object(prepare.subprocess, "run", side_effect=ffmpeg),
            ):
                prepare.main()
            self.assertEqual(hashlib.sha256(movie.read_bytes()).hexdigest(), MOVIE_HASH)
            self.assertFalse(movie.with_suffix(".avi").exists())

        return root, movie, calls, run

    def test_cached_original_mp4_is_retained_without_conversion(self):
        _, _, calls, run = self.fixture("cached")
        run()
        self.assertEqual(calls, [])

    def test_original_apk_mp4_is_extracted_without_conversion(self):
        _, _, calls, run = self.fixture("apk-movie", movie_in_apk=True)
        run()
        self.assertEqual(calls, [])

    def test_cached_audio_and_mp4_do_not_require_ffmpeg(self):
        _, _, calls, run = self.fixture("without-ffmpeg")
        run(ffmpeg_available=False)
        self.assertEqual(calls, [])

    def test_repeated_preparation_preserves_video_and_unrelated_avi(self):
        _, movie, calls, run = self.fixture("repeat")
        unrelated = movie.parent / "user-supplied.avi"
        unrelated.write_bytes(b"Unrelated user movie; preserve these bytes.")
        before = movie.stat().st_mtime_ns
        run()
        run()
        self.assertEqual(movie.stat().st_mtime_ns, before)
        self.assertEqual(unrelated.read_bytes(), b"Unrelated user movie; preserve these bytes.")
        self.assertEqual(calls, [])

    def test_audio_conversion_remains_available_without_video_conversion(self):
        root, _, calls, run = self.fixture("missing-audio", missing_audio=True)
        run()
        self.assertEqual(len(calls), 1)
        self.assertEqual(Path(calls[0][-1]).name, selected[0] + ".wav")
        self.assertFalse((root / "build" / selected[1]).exists())


suite = unittest.defaultTestLoader.loadTestsFromTestCase(PrepareIntroPipeline)
result = unittest.TextTestRunner(verbosity=2).run(suite)
report = {
    "tests_run": result.testsRun,
    "failures": len(result.failures),
    "errors": len(result.errors),
    "success": result.wasSuccessful(),
    "source_sha256": hashlib.sha256((ROOT / "tools/prepare.py").read_bytes()).hexdigest(),
    "intro_sha256": MOVIE_HASH,
    "isolated_fixtures": str(WORK.relative_to(ROOT)),
    "real_ffmpeg_executed": False,
}
(WORK / "result.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
sys.exit(0 if result.wasSuccessful() else 1)
