"""Audit every supplied scene and emit narrow, geometry-guarded backdrop policies.

No source resource is rewritten. Policies describe render-only continuation of
existing gradients, constant fills and demonstrably repeated scenery.
"""

import collections, json, struct, hashlib
from pathlib import Path
import resources as r

ROOT = Path(__file__).resolve().parents[1]
TITLE = 12192614360109026946


def uvhash(q):
    h = 2166136261
    for b in struct.pack("<8f", *(v for p in q for v in p[2:])):
        h = ((h ^ b) * 16777619) & 0xFFFFFFFF
    return h


def geometryhash(q):
    h = 2166136261
    for b in struct.pack("<16f", *(v for p in q for v in p)):
        h = ((h ^ b) * 16777619) & 0xFFFFFFFF
    return h


def rect(q):
    return [q[0][0], q[0][1], q[3][0], q[3][1]]


def dimensions(q):
    a = rect(q)
    return a[2] - a[0], a[3] - a[1]


def scenery_families(scene):
    """Group scenery only in back layers, never ordinary gameplay layers."""
    groups = collections.defaultdict(list)
    for ti, tile in enumerate(scene["tiles"]):
        layer = scene["layers"][tile["layer"]]
        parallax = min(layer["scroll"]) < 1 and not layer["flags"][2]
        if tile["layer"] != 0 and not (scene["props"].get("bean-world") and parallax):
            continue
        for off in range(0, len(tile["vertices"]), 4):
            q = tile["vertices"][off : off + 4]
            w, h = dimensions(q)
            if w < 64 or h < 16:
                continue
            key = (tile["layer"], tile["z"], tile["texture"], w, h, tuple(v[2:] for v in q))
            groups[key].append((ti, off, tile, q))
    return list(groups.values())


def consecutive_scenery(family):
    family.sort(key=lambda item: item[3][0][0])
    w, _ = dimensions(family[0][3])
    ys = [item[3][0][1] for item in family]
    return max(ys) - min(ys) <= 1 and all(
        abs(b[3][0][0] - a[3][0][0] - w) < 0.001 for a, b in zip(family, family[1:])
    )


def scenery_key(scene, item):
    """A shared atlas region proves a single placed copy uses an audited motif."""
    tile, q = item[2:]
    texture = scene.get("texture_file") or tuple(
        hashlib.sha256(t["pixels"]).hexdigest() for t in scene["embedded_textures"]
    )
    return texture, tile["texture"], tuple(v[2:] for v in q)


def scenery_texture(scene, item, textures):
    key = scene.get("texture_file")
    if key:
        if key not in textures:
            reader = r.Reader((ROOT / "original/Media/Texture Groups" / key).read_bytes())
            reader.take(16)
            textures[key] = [reader.texture() for _ in range(reader.i())]
        return textures[key][item[2]["texture"]]
    else:
        return scene["embedded_textures"][item[2]["texture"]]


def scenery_edge_samples(scene, item, textures, top=False):
    """Inspect actual atlas pixels so flipped ceiling scenery stays flipped."""
    texture = scenery_texture(scene, item, textures)
    q = item[3]
    width, height = dimensions(q)
    samples = []
    for x in range(int(width)):
        fx, fy = (x + 0.5) / width, (1 - 0.5 / height) if top else 0.5 / height
        uv = [
            (1 - fy) * (q[0][c] + (q[1][c] - q[0][c]) * fx)
            + fy * (q[2][c] + (q[3][c] - q[2][c]) * fx)
            for c in (2, 3)
        ]
        ix = min(texture["width"] - 1, max(0, int(uv[0] * texture["width"])))
        iy = min(texture["height"] - 1, max(0, int(uv[1] * texture["height"])))
        at = (iy * texture["width"] + ix) * 4
        samples.append(texture["pixels"][at : at + 4])
    return samples


def scenery_bottom_constant(scene, item, textures):
    """An opaque edge with at most one colour quantization step may continue down."""
    samples = scenery_edge_samples(scene, item, textures)
    return (
        bool(samples)
        and all(p[3] == 255 for p in samples)
        and all(max(p[c] for p in samples) - min(p[c] for p in samples) <= 1 for c in range(3))
    )


def scenery_grid_bounds(family):
    """Recognize complete small-tile room backgrounds on any parallax layer."""
    w, h = dimensions(family[0][3])
    if len(family) < 8 or max(w, h) > 128:
        return None
    bounds = [
        min(v[3][0][0] for v in family),
        min(v[3][0][1] for v in family),
        max(v[3][3][0] for v in family),
        max(v[3][3][1] for v in family),
    ]
    nx, ny = (bounds[2] - bounds[0]) / w, (bounds[3] - bounds[1]) / h
    cells = {
        (round((v[3][0][0] - bounds[0]) / w, 6), round((v[3][0][1] - bounds[1]) / h, 6))
        for v in family
    }
    return (
        bounds
        if nx >= 2
        and ny >= 2
        and nx.is_integer()
        and ny.is_integer()
        and len(cells) == nx * ny
        and all(x.is_integer() and y.is_integer() for x, y in cells)
        else None
    )


def scenery_sparse_pattern(family):
    """Prove a small repeating occupancy motif, preserving intentional windows."""
    w, h = dimensions(family[0][3])
    if len(family) < 16 or max(w, h) > 128:
        return None
    bounds = [
        min(v[3][0][0] for v in family),
        min(v[3][0][1] for v in family),
        max(v[3][3][0] for v in family),
        max(v[3][3][1] for v in family),
    ]
    nx, ny = (bounds[2] - bounds[0]) / w, (bounds[3] - bounds[1]) / h
    if not nx.is_integer() or not ny.is_integer():
        return None
    occupied = {((v[3][0][0] - bounds[0]) / w, (v[3][0][1] - bounds[1]) / h) for v in family}
    if not all(x.is_integer() and y.is_integer() for x, y in occupied) or len(occupied) == nx * ny:
        return None
    # Require two observed complete cycles on both axes, including all holes.
    candidates = sorted(
        (c * rows, c, rows)
        for c in range(1, min(16, int(nx) // 2) + 1)
        for rows in range(1, min(16, int(ny) // 2) + 1)
        if c * rows <= 64
    )
    for _, columns, rows in candidates:
        motif = {}
        valid = True
        for y in range(int(ny)):
            for x in range(int(nx)):
                key = x % columns, y % rows
                solid = (x, y) in occupied
                if key in motif and motif[key] != solid:
                    valid = False
                    break
                motif[key] = solid
            if not valid:
                break
        if valid:
            mask = sum(1 << (y * columns + x) for (x, y), solid in motif.items() if solid)
            return bounds, columns, rows, mask
    return None


def main():
    project = r.project()
    # Learn periodic artwork from real adjacent copies, then recognize that
    # same atlas region when another parallax layer contains only one copy.
    scenes = {lid: r.level(info["file"]) for lid, info in project["levels"].items()}
    periodic = set()
    textures = {}
    for scene in scenes.values():
        for family in scenery_families(scene):
            if len(family) >= 2 and consecutive_scenery(family):
                periodic.add(scenery_key(scene, family[0]))
    records = []
    inventory = []
    for lid, info in project["levels"].items():
        scene = scenes[lid]
        staged_story = any(
            b["type"] == "EndOfGameCutsceneController"
            for o in scene["objects"]
            for b in o["behaviors"]
        )
        local = []
        quads = []
        for ti, tile in enumerate(scene["tiles"]):
            for off in range(0, len(tile["vertices"]), 4):
                q = tile["vertices"][off : off + 4]
                if len(q) != 4:
                    continue
                assert (
                    q[0][0] == q[2][0]
                    and q[1][0] == q[3][0]
                    and q[0][1] == q[1][1]
                    and q[2][1] == q[3][1]
                )
                quads.append((ti, off, tile, q))

        def add(item, mode, bounds=None, step=(0, 0), reason=""):
            ti, off, tile, q = item
            alternate = 0
            if lid == 669776786229 and ti == 55:
                widened = [list(v) for v in q]
                widened[1][0] = widened[3][0] = 864
                alternate = geometryhash(widened)
            record = {
                "scene": lid,
                "tile": ti,
                "vertex": off,
                "mode": mode,
                "layer": tile["layer"],
                "z": tile["z"],
                "texture": tile["texture"],
                "uv_hash": uvhash(q),
                "geometry_hash": geometryhash(q),
                "alternate_geometry_hash": alternate,
                "bounds": bounds or rect(q),
                "step": step,
                "reason": reason,
                "pattern": [0, 0, 0],
            }
            local.append(record)
            return record

        for item in quads:
            ti, off, t, q = item
            w, h = dimensions(q)
            us = [v[2] for v in q]
            vs = [v[3] for v in q]
            du = max(us) - min(us)
            dv = max(vs) - min(vs)
            sky_layer = t["layer"] == 0 or (lid == TITLE and t["layer"] == 1)
            if (
                sky_layer
                and w >= 512
                and h >= 192
                and min(du, dv) <= 32 / 1024 + 1e-8
                and max(du, dv) >= 8 * min(du, dv)
            ):
                b = rect(q)
                b[1] = min(0, b[1])
                b[3] = max(384, b[3])
                add(
                    item,
                    0,
                    b,
                    reason="Authored sky gradient; continue only outside its original reference frame, preserving all original interior geometry",
                )
            elif (
                not scene["props"].get("bean-world")
                and q[0][0] == 0
                and q[0][1] == 0
                and w >= 512
                and h == 384
                and max(du, dv) <= 2 / 1024 + 1e-8
            ):
                add(item, 0, reason="Authored full-screen constant colour/tint fill")
            elif lid == 8808768056724772154 and ti == 1:
                add(
                    item,
                    3,
                    reason="World1 selector bottom fill continues horizontally/downward only; never covers sky above the original ground strip",
                )
        low = [q for q in quads if q[2]["layer"] == 0]
        families = collections.defaultdict(list)
        for item in low:
            ti, off, t, q = item
            w, h = dimensions(q)
            if w <= 0 or h <= 0:
                continue
            families[(t["z"], t["texture"], w, h, tuple(v[2:] for v in q))].append(item)
        uniform = False
        if len(families) == 1 and len(low) >= 8:
            family = next(iter(families.values()))
            w, h = dimensions(family[0][3])
            bounds = [
                min(v[3][0][0] for v in family),
                min(v[3][0][1] for v in family),
                max(v[3][3][0] for v in family),
                max(v[3][3][1] for v in family),
            ]
            cells = {
                (round((v[3][0][0] - bounds[0]) / w, 6), round((v[3][0][1] - bounds[1]) / h, 6))
                for v in family
            }
            aligned = {xy for xy in cells if xy[0].is_integer() and xy[1].is_integer()}
            nx = (bounds[2] - bounds[0]) / w
            ny = (bounds[3] - bounds[1]) / h
            uniform = nx.is_integer() and ny.is_integer() and len(aligned) == nx * ny
            if uniform:
                add(
                    family[0],
                    2,
                    bounds,
                    (w, h),
                    "Identical layer0 texture/UV/size covers a complete rectangular grid; retain any authored overlapping copies and continue the same two-dimensional period only outside that rectangle",
                )
        if not uniform:
            for family in families.values():
                if len(family) < 2:
                    continue
                family.sort(key=lambda item: item[3][0][0])
                w, h = dimensions(family[0][3])
                ys = [v[3][0][1] for v in family]
                if w < 32 or h < 16 or max(ys) - min(ys) > 1:
                    continue
                if not all(
                    abs(b[3][0][0] - a[3][0][0] - w) < 0.001 for a, b in zip(family, family[1:])
                ):
                    continue
                bounds = [
                    family[0][3][0][0],
                    min(ys),
                    family[-1][3][3][0],
                    max(v[3][3][1] for v in family),
                ]
                add(
                    family[0],
                    1,
                    bounds,
                    (w, h),
                    "Consecutive identical layer0 scenery copies establish a horizontal period; continue only beyond the existing group",
                )
        # Native maps also place far scenery on nonzero parallax layers. The
        # previous layer0-only pass missed mountains, woods and distant cities.
        for family in scenery_families(scene):
            family.sort(key=lambda item: item[3][0][0])
            first = family[0]
            # The final level is a sequence of separately authored environments.
            # A courtyard far down the map must never wrap into the boss room.
            if staged_story and first[2]["layer"] > 0:
                continue
            if any(p["mode"] == 2 and p["layer"] == first[2]["layer"] for p in local):
                continue
            if any(
                p["tile"] == first[0] and p["vertex"] == first[1] and p["mode"] != 1 for p in local
            ):
                continue
            grid = scenery_grid_bounds(family)
            if grid:
                add(
                    first,
                    2,
                    grid,
                    dimensions(first[3]),
                    "Complete repeated room texture on a nonzero parallax layer",
                )
                continue
            sparse = scenery_sparse_pattern(family)
            if sparse:
                bounds, columns, rows, mask = sparse
                record = add(
                    first,
                    8,
                    bounds,
                    dimensions(first[3]),
                    "Repeated sparse masonry occupancy proven over at least two cycles per axis; preserve authored window openings and adapt the whole motif uniformly on ultra-wide windows",
                )
                record["pattern"] = [columns, rows, mask]
                continue
            if not consecutive_scenery(family):
                continue
            w, h = dimensions(first[3])
            q = first[3]
            if (
                w >= 512
                and max(max(v[c] for v in q) - min(v[c] for v in q) for c in (2, 3))
                <= 2 / 1024 + 1e-8
            ):
                add(
                    first,
                    3,
                    reason="Small constant-colour atlas region forms a parallax ground/fog strip; continue sideways and below, never above",
                )
                continue
            known = scenery_key(scene, first) in periodic
            # A single large transparent-topped landscape with a flat opaque
            # base is a horizon, unlike isolated palms/columns. Reflect its
            # continuation at each edge to avoid inventing a mismatched seam.
            single_horizon = (
                bool(scene["props"].get("bean-world"))
                and first[2]["layer"] > 0
                and len(family) == 1
                and w >= 384
                and h >= 160
                and scenery_bottom_constant(scene, first, textures)
            )
            if not known and not single_horizon:
                continue
            bounds = [
                first[3][0][0],
                min(v[3][0][1] for v in family),
                family[-1][3][3][0],
                max(v[3][3][1] for v in family),
            ]
            mode = 6 if not known else (5 if scenery_bottom_constant(scene, first, textures) else 4)
            if (
                known
                and not all(v[3] == 255 for v in scenery_edge_samples(scene, first, textures))
                and all(v[3] == 255 for v in scenery_edge_samples(scene, first, textures, top=True))
            ):
                mode = 7
            existing = next(
                (p for p in local if p["tile"] == first[0] and p["vertex"] == first[1]), None
            )
            if existing and existing["mode"] != 1:
                continue
            if existing:
                existing["mode"] = mode
                existing["reason"] += "; responsive horizontal spacing limits visual repetition"
            else:
                add(
                    first,
                    mode,
                    bounds,
                    dimensions(first[3]),
                    "Shared periodic atlas motif on a background/parallax layer; retain authored pixels at ordinary widths, widen distant scenery only beyond three visible periods; extend downward only for a proven constant opaque edge",
                )
        records.extend(local)
        inventory.append(
            {
                "id": lid,
                "name": scene["props"].get("name"),
                "file": info["file"],
                "tile_count": len(scene["tiles"]),
                "policies": local,
                "unextended_reason": (
                    None
                    if local
                    else (
                        "Staged story environments retain their original spatial artwork boundaries"
                        if staged_story
                        else "No confirmed sky/constant full-screen fill or complete periodic background grid; preserve authored black, logos, overlays and local room artwork"
                    )
                ),
            }
        )
    rows = [
        "#pragma once",
        "// Generated by tools/background_extension_inventory.py from all74 original scenes.",
        "const BackgroundExtensionPolicy backgroundExtensionPolicies[]={",
    ]
    for p in records:
        f = lambda n: format(float(n), ".9g") + ("f" if "." in format(float(n), ".9g") else ".0f")
        rows.append(
            "    {%dULL,%d,%d,%d,%d,%d,%d,0x%08xu,0x%08xu,0x%08xu,{%s},%s,%s,0x%xULL,%d,%d},"
            % (
                p["scene"],
                p["tile"],
                p["vertex"],
                p["mode"],
                p["layer"],
                p["z"],
                p["texture"],
                p["uv_hash"],
                p["geometry_hash"],
                p["alternate_geometry_hash"],
                ",".join(map(f, p["bounds"])),
                f(p["step"][0]),
                f(p["step"][1]),
                p["pattern"][2],
                p["pattern"][0],
                p["pattern"][1],
            )
        )
    rows += [
        "};",
        "const int backgroundExtensionPolicyCount=sizeof(backgroundExtensionPolicies)/sizeof(backgroundExtensionPolicies[0]);",
        "",
    ]
    (ROOT / "src/background_extension_policies.h").write_text("\n".join(rows), encoding="utf-8")
    summary = {
        "scene_count": len(inventory),
        "covered_scenes": sum(bool(s["policies"]) for s in inventory),
        "policies": len(records),
        "modes": dict(collections.Counter(p["mode"] for p in records)),
        "scenes": inventory,
    }
    (ROOT / "build/background-extension-inventory.json").write_text(
        json.dumps(summary, indent=2), encoding="utf-8"
    )
    print({k: v for k, v in summary.items() if k != "scenes"})


if __name__ == "__main__":
    main()
