"""Encode the editable Maker PNG layers as native RGBA textures, without changing their source files."""

from pathlib import Path
import hashlib
import json
import struct

try:
    from PIL import Image
except ImportError as error:
    raise SystemExit("Maker artwork preparation requires Pillow: python -m pip install Pillow") from error


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "resources" / "maker-edit"
OUTPUT = ROOT / "bin" / "data" / "Media" / "Windows" / "Maker"
CANVAS = (832, 1024)


def main():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    manifest = []
    for layer in ("scene", "title"):
        source = SOURCE / f"MAKER-{layer}.png"
        with Image.open(source) as image:
            if image.size != CANVAS:
                raise SystemExit(f"{source.name}: keep the editable canvas at 832 x 1024 pixels")
            rgba = image.convert("RGBA")
            if rgba.getchannel("A").getextrema()[0] != 0:
                raise SystemExit(f"{source.name}: retain the transparent area outside the artwork")
            # Pad to a power-of-two atlas for OpenGL 1.1; never stretch or resample the edited pixels.
            atlas = Image.new("RGBA", (1024, 1024), (0, 0, 0, 0))
            atlas.paste(rgba, (0, 0))
            pixels = atlas.tobytes()
        # Match textureRead's single-level RGBA/unsigned-byte layout and nearest-neighbour filters.
        header = b"BQMakerCardRGBA".ljust(16, b"\0")
        header += struct.pack("<10i", 0, 0, 1, 1024, 1024, 9728, 9728, 6408, 5121, len(pixels))
        encoded = header + pixels
        destination = OUTPUT / f"{layer}.ptex"
        if not destination.exists() or destination.read_bytes() != encoded:
            destination.write_bytes(encoded)
        manifest.append({
            "source": source.relative_to(ROOT).as_posix(),
            "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
            "runtime": destination.relative_to(ROOT).as_posix(),
            "runtime_sha256": hashlib.sha256(encoded).hexdigest(),
            "canvas": list(CANVAS),
            "atlas": [1024, 1024],
            "resampled": False,
        })
    (ROOT / "build").mkdir(exist_ok=True)
    (ROOT / "build" / "maker-art-manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
    )
    print("Prepared Maker scene/title textures from editable PNG layers (source pixels unchanged).")


if __name__ == "__main__":
    main()
