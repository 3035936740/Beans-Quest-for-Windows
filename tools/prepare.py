"""Prepare assets from the user's APK. Python/FFmpeg are build tools only."""

from pathlib import Path
import shutil, subprocess, zipfile, gzip

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "bin"


def main():
    (ROOT / "build").mkdir(exist_ok=True)
    out = OUT / "data"
    out.mkdir(parents=True, exist_ok=True)
    z = zipfile.ZipFile(ROOT / "Bean's Quest.apk")
    count = 0
    for name in z.namelist():
        if name.startswith("assets/Media/") and (
            name.endswith((".pgame", ".plevel", ".panims", ".ptex", ".ttf"))
            or name.startswith("assets/Media/Movies/")
            and name.endswith((".mp4", ".avi"))
        ):
            p = out / name.removeprefix("assets/")
            p.parent.mkdir(parents=True, exist_ok=True)
            b = z.read(name)
            b = gzip.decompress(b) if b[:2] == b"\x1f\x8b" else b
            if not p.exists() or p.read_bytes() != b:
                p.write_bytes(b)
            count += 1
    (OUT / "saves").mkdir(exist_ok=True)
    audio = out / "audio"
    audio.mkdir(exist_ok=True)
    import sys

    sys.path.insert(0, str(ROOT / "tools"))
    import resources

    # Parse the extracted project for sound identifiers and music names.
    r = resources.Reader((out / "Media/project.pgame").read_bytes())
    r.take(16)
    r.d()
    for _ in range(r.i()):
        r.q()
        r.s()
        r.d()
    sounds = r.d()
    tracks = {}
    for _ in range(r.i()):
        uid = r.q()
        file = r.s()
        r.d()
        tracks[str(uid)] = file
    ffmpeg = shutil.which("ffmpeg")
    # Movies copied above, or supplied in the runtime directory, stay byte-for-byte
    # original. The game plays MP4 directly; FFmpeg is for audio preparation only.
    for key, file in list(sounds.items()) + list(tracks.items()):
        target = audio / (key + ".wav")
        if target.exists():
            continue
        previous = ROOT / "dist/BeanQuest/data/audio" / (key + ".wav")
        if previous.exists():
            shutil.copy2(previous, target)
            continue
        if not ffmpeg:
            raise RuntimeError(
                "FFmpeg is required only for the first audio conversion. Install it or retain the prepared data/audio folder."
            )
        folder = "Samples" if file.endswith(".ogg") else "Music"
        source = ROOT / "build" / file
        source.write_bytes(z.read("assets/Media/" + folder + "/" + file))
        subprocess.run(
            [
                ffmpeg,
                "-v",
                "error",
                "-y",
                "-i",
                str(source),
                "-ac",
                "2",
                "-ar",
                "22050",
                "-c:a",
                "pcm_s16le",
                str(target),
            ],
            check=True,
        )
        source.unlink()
    try:
        from PIL import Image
        import io

        im = Image.open(io.BytesIO(z.read("res/drawable-xhdpi-v4/icon.png"))).convert("RGBA")
        im.save(ROOT / "build/game.ico", sizes=[(16, 16), (32, 32), (48, 48)], bitmap_format="bmp")
    except ImportError:
        pass
    if (ROOT / "build/game.ico").exists():
        (ROOT / "build/game.rc").write_text('101 ICON "game.ico"\n', encoding="ascii")
    print(
        f"Prepared {count} original resource files and {len(sounds)+len(tracks)} PCM audio files in {OUT}"
    )


if __name__ == "__main__":
    main()
