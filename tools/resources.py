"""Read the supplied APK's Pimento binary resources without executing Android code."""

from pathlib import Path
import struct, gzip, zipfile, json

ROOT = Path(__file__).resolve().parents[1]


class Reader:
    # Resource files are little-endian. Every scalar and string consumes the
    # same checked cursor so malformed lengths cannot silently shift parsing.
    def __init__(self, b):
        self.b, self.p = b, 0

    def take(self, n):
        assert 0 <= n <= len(self.b) - self.p, (self.p, n, len(self.b))
        v = self.b[self.p : self.p + n]
        self.p += n
        return v

    def unpack(self, fmt):
        return struct.unpack("<" + fmt, self.take(struct.calcsize("<" + fmt)))

    def i(self):
        return self.unpack("i")[0]

    def q(self):
        return self.unpack("Q")[0]

    def s(self):
        return self.take(self.i()).decode("utf-8")

    def d(self):
        return {self.s(): self.s() for _ in range(self.i())}

    def obj(self):
        o = dict(zip(("id", "base", "class"), self.unpack("QQQ")))
        o["paused"], o["visible"], o["z"] = self.unpack("iii")
        o["rect"] = self.unpack("ffff")
        o["angle"] = self.unpack("f")[0]
        o["props"] = self.d()
        o["behaviors"] = []
        for _ in range(self.i()):
            o["behaviors"].append({"id": self.q(), "type": self.s(), "props": self.d()})
        return o

    def texture(self):
        self.take(16)
        h = self.unpack("10i")
        assert 1 <= h[2] <= 12 and h[7] == 6408 and h[8] == 5121, h
        pixels = self.take(h[9])
        for _ in range(1, h[2]):
            self.take(self.i())
        return {"width": h[3], "height": h[4], "pixels": pixels}


def extract():
    z = zipfile.ZipFile(ROOT / "Bean's Quest.apk")
    for n in z.namelist():
        if n.startswith("assets/Media/"):
            p = ROOT / "original" / n.removeprefix("assets/")
            p.parent.mkdir(parents=True, exist_ok=True)
            b = z.read(n)
            p.write_bytes(gzip.decompress(b) if b[:2] == b"\x1f\x8b" else b)


def project():
    r = Reader((ROOT / "original/Media/project.pgame").read_bytes())
    r.take(16)
    p = {"props": r.d(), "levels": {}, "sounds": {}, "music": {}, "classes": {}}
    for _ in range(r.i()):
        uid = r.q()
        file = r.s()
        p["levels"][uid] = {"file": file, "props": r.d()}
    p["sounds"] = r.d()
    for _ in range(r.i()):
        uid = r.q()
        p["music"][uid] = {"file": r.s(), "props": r.d()}
    for _ in range(r.i()):
        o = r.obj()
        p["classes"][o["id"]] = o
    assert r.p == len(r.b), (r.p, len(r.b))
    return p


def level(file):
    r = Reader((ROOT / "original/Media/Levels" / file).read_bytes())
    r.take(16)
    l = {"size": r.unpack("ff"), "id": r.q(), "props": r.d(), "layers": []}
    for _ in range(r.i()):
        l["layers"].append(
            {
                "id": r.q(),
                "scroll": r.unpack("ff"),
                "auto_scroll": r.unpack("ff"),
                "size": r.unpack("ff"),
                "flags": r.unpack("iii"),
            }
        )
    l["backdrop_magic"] = r.take(16).hex()
    nt = r.i()
    if nt == -1:
        l["texture_group"] = r.s()
        l["texture_file"] = r.s()
    else:
        l["embedded_textures"] = [r.texture() for _ in range(nt)]
        l["texture_group"] = ""
        l["texture_file"] = ""
    l["tiles"] = []
    for _ in range(r.i()):
        t = {
            "layer": r.i(),
            "z": r.i(),
            "flags": r.i(),
            "texture": r.i(),
            "count": r.i(),
            "rect": r.unpack("ffff"),
        }
        t["meta"] = r.unpack("iiiii")
        n = r.i()
        t["vertices"] = [r.unpack("ffff") for _ in range(n // 16)]
        assert n % 16 == 0
        l["tiles"].append(t)
    l["geometry_magic"] = r.take(16).hex()
    l["geometry"] = []
    for _ in range(r.i()):
        kind, flags, n = r.unpack("iii")
        pts = [r.unpack("ii") for _ in range(n)]
        l["geometry"].append({"kind": kind, "flags": flags, "points": pts})
    l["objects"] = []
    for _ in range(r.i()):
        o = r.obj()
        o["layer"] = r.i()
        l["objects"].append(o)
    assert r.p == len(r.b), (file, r.p, len(r.b), r.b[r.p : r.p + 100])
    return l


if __name__ == "__main__":
    extract()
    p = project()
    (ROOT / "build/project.json").write_text(json.dumps(p, indent=2), encoding="utf-8")
    print("classes", len(p["classes"]))
    for k, v in p["levels"].items():
        try:
            l = level(v["file"])
            (ROOT / "build" / v["file"].replace(".plevel", ".json")).write_text(
                json.dumps(l, indent=2, default=lambda b: "<%d bytes>" % len(b)), encoding="utf-8"
            )
            print(v["props"]["name"], len(l["tiles"]), len(l["geometry"]), len(l["objects"]))
        except Exception as e:
            print("ERROR", v["file"], repr(e))
            raise
