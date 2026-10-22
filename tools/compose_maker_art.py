"""Composite the user's grain-edited Maker title into the existing PNG layers.

This uses only bitmap sampling and alpha compositing. It does not generate
artwork, add grain, or alter MAKER-scene.png / the edited source image.
"""

from pathlib import Path
import hashlib
import json

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
ART = ROOT / "resources" / "maker-edit"
# Registration of the supplied 2172x724 source to the existing 832x1024 title
# canvas, measured from 93 matching foreground image features.
SOURCE_TO_TITLE = (
    (0.344500244, 0.000195723246, 34.2514479),
    (-0.000195723246, 0.344500244, 50.9937028),
)


def main():
    source_path = ART / "MAKER-title-grasslands-source.png"
    scene_path = ART / "MAKER-scene.png"
    title_path = ART / "MAKER-title.png"
    source_hash = hashlib.sha256(source_path.read_bytes()).hexdigest()
    scene_hash = hashlib.sha256(scene_path.read_bytes()).hexdigest()
    with Image.open(source_path) as source, Image.open(title_path) as title:
        if source.size != (2172, 724) or title.size != (832, 1024):
            raise SystemExit("Canvas dimensions changed; realign the edited title before compositing.")
        # Retain the clean cutout from the existing title, including its
        # antialiased edge. Background RGB noise must not become a solid box.
        alpha = title.convert("RGBA").getchannel("A")
        (a, b, tx), (c, d, ty) = SOURCE_TO_TITLE
        determinant = a * d - b * c
        inverse = (d / determinant, -b / determinant, (b * ty - d * tx) / determinant,
                   -c / determinant, a / determinant, (c * tx - a * ty) / determinant)
        # Nearest-neighbour sampling preserves the supplied fine grain instead
        # of averaging it away while reducing the high-resolution source.
        edited = source.convert("RGB").transform(
            title.size, Image.Transform.AFFINE, inverse, resample=Image.Resampling.NEAREST
        ).convert("RGBA")
        edited.putalpha(alpha)
    with Image.open(scene_path) as scene:
        cover = Image.alpha_composite(scene.convert("RGBA"), edited)
    edited.save(title_path)
    cover.save(ART / "MAKER-cover.png")
    assert hashlib.sha256(source_path.read_bytes()).hexdigest() == source_hash
    assert hashlib.sha256(scene_path.read_bytes()).hexdigest() == scene_hash
    report = {
        "method": "affine nearest-neighbour sampling and alpha compositing",
        "generated": False,
        "source_sha256": source_hash,
        "scene_sha256": scene_hash,
        "source_to_title": SOURCE_TO_TITLE,
        "title_sha256": hashlib.sha256(title_path.read_bytes()).hexdigest(),
        "cover_sha256": hashlib.sha256((ART / "MAKER-cover.png").read_bytes()).hexdigest(),
    }
    (ROOT / "build").mkdir(exist_ok=True)
    (ROOT / "build" / "maker-noise-validation.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8"
    )
    print("Updated Maker title and cover from the grain-edited source; source and scene preserved.")


if __name__ == "__main__":
    main()
