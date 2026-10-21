"""Inventory every defined ELF function, preserving ARM mapping symbols and aliases.

This emits evidence for a manual native-to-port review. Decoding code does not
establish semantic parity. Platform and dependency functions are included too.
Requires pyelftools, capstone, and a local GNU c++filt (no network access).
"""

from collections import Counter, defaultdict
from pathlib import Path
import argparse
import bisect
import hashlib
import json
import math
import re
import shutil
import struct
import subprocess

from capstone import Cs, CS_ARCH_ARM, CS_MODE_ARM, CS_MODE_THUMB, CS_GRP_CALL, CS_GRP_JUMP
from capstone.arm import ARM_OP_IMM, ARM_OP_MEM, ARM_REG_PC
from elftools.elf.elffile import ELFFile
from elftools.elf.relocation import RelocationSection

ROOT = Path(__file__).resolve().parents[1]


def group_name(name):
    if name.startswith("App::"):
        return "App." + name[5:].split("::")[0].split("<")[0]
    if name.startswith("_GLOBAL__I_"):
        return "Initialization"
    if name.startswith(("b2", "b2_")):
        return "Box2D"
    if name.startswith(("std::", "__gnu_cxx::", "boost::")):
        return "CxxRuntime"
    for prefix in ("ZAudio", "ZRenderer", "ZInput", "ZUtil", "ZTask", "ZPlatform"):
        if name.startswith(prefix + "::"):
            return prefix
    return "PlatformAndOther"


def inventory(output):
    path = ROOT / "original/libBeansQuest.so"
    blob = path.read_bytes()
    cxxfilt = shutil.which("c++filt")
    if not cxxfilt:
        raise RuntimeError("GNU c++filt is required for named, reviewable evidence")
    output.mkdir(parents=True, exist_ok=True)
    (output / "assembly").mkdir(exist_ok=True)
    with path.open("rb") as stream:
        elf = ELFFile(stream)
        symbols = list(elf.get_section_by_name(".symtab").iter_symbols())
        functions = [
            s
            for s in symbols
            if s["st_info"]["type"] == "STT_FUNC" and s["st_shndx"] != "SHN_UNDEF" and s["st_value"]
        ]
        function_starts = sorted(set(s["st_value"] & ~1 for s in functions))
        known_sizes = {s["st_value"] & ~1: s["st_size"] for s in functions if s["st_size"]}

        def function_size(symbol):
            start = symbol["st_value"] & ~1
            if start in known_sizes:
                return known_sizes[start]
            section = elf.get_section(symbol["st_shndx"])
            end = section["sh_addr"] + section["sh_size"]
            index = bisect.bisect_right(function_starts, start)
            if index < len(function_starts):
                end = min(end, function_starts[index])
            return end - start

        names = [s.name for s in symbols]
        decoded = subprocess.run(
            [cxxfilt], input="\n".join(names) + "\n", text=True, capture_output=True, check=True
        ).stdout.splitlines()
        demangled = dict(zip(names, decoded))
        symbol_manifest = [
            dict(
                name=s.name,
                demangled=demangled.get(s.name, s.name),
                address=f"0x{s['st_value']:08x}",
                size=s["st_size"],
                type=s["st_info"]["type"],
                binding=s["st_info"]["bind"],
                section=s["st_shndx"],
            )
            for s in symbols
        ]
        (output / "symbols.json").write_text(
            json.dumps(symbol_manifest, ensure_ascii=False, indent=2), encoding="utf-8"
        )
        section_manifest, relocations = [], []
        for section in elf.iter_sections():
            section_manifest.append(
                dict(
                    name=section.name,
                    type=section["sh_type"],
                    address=f"0x{section['sh_addr']:08x}",
                    size=section["sh_size"],
                    flags=section["sh_flags"],
                    sha256=hashlib.sha256(section.data()).hexdigest(),
                )
            )
            if isinstance(section, RelocationSection):
                table = elf.get_section(section["sh_link"])
                for relocation in section.iter_relocations():
                    entry = relocation.entry
                    symbol = table.get_symbol(entry["r_info_sym"]) if entry["r_info_sym"] else None
                    relocations.append(
                        dict(
                            section=section.name,
                            offset=f"0x{entry['r_offset']:08x}",
                            type=entry["r_info_type"],
                            symbol=symbol.name if symbol else "",
                            addend=entry.get("r_addend"),
                        )
                    )
        (output / "sections.json").write_text(
            json.dumps(section_manifest, indent=2), encoding="utf-8"
        )
        (output / "relocations.json").write_text(
            json.dumps(relocations, ensure_ascii=False, indent=2), encoding="utf-8"
        )
        segments = [dict(s.header) for s in elf.iter_segments() if s["p_type"] == "PT_LOAD"]
        text = elf.get_section_by_name(".text")
        text_start, text_end = text["sh_addr"], text["sh_addr"] + text["sh_size"]
        mappings = sorted(
            (s["st_value"], s.name[1]) for s in symbols if re.match(r"^\$[atd](?:\.|$)", s.name)
        )
        map_addresses = [m[0] for m in mappings]
        all_targets = defaultdict(list)
        for s in symbols:
            if s["st_value"] and not s.name.startswith("$"):
                all_targets[s["st_value"] & ~1].append(demangled.get(s.name, s.name))
        plt = elf.get_section_by_name(".plt")
        plt_relocations = elf.get_section_by_name(".rel.plt")
        if plt and plt_relocations:
            table = elf.get_section(plt_relocations["sh_link"])
            entries = list(plt_relocations.iter_relocations())
            assert plt["sh_size"] == 20 + len(entries) * 12, "Unrecognized ARM PLT layout"
            for index, entry in enumerate(entries):
                name = table.get_symbol(entry["r_info_sym"]).name
                all_targets[plt["sh_addr"] + 20 + index * 12].append(
                    demangled.get(name, name) + "@plt"
                )
        arm = Cs(CS_ARCH_ARM, CS_MODE_ARM)
        thumb = Cs(CS_ARCH_ARM, CS_MODE_THUMB)
        arm.detail = thumb.detail = True
        records, artifacts, owned = [], defaultdict(list), []

        def read(address, size):
            for seg in segments:
                if seg["p_vaddr"] <= address and address + size <= seg["p_vaddr"] + seg["p_filesz"]:
                    off = seg["p_offset"] + address - seg["p_vaddr"]
                    return blob[off : off + size]
            return b""

        data_symbols = []
        native_physics = {}
        for symbol in symbols:
            if (
                symbol["st_info"]["type"] != "STT_OBJECT"
                or symbol["st_shndx"] == "SHN_UNDEF"
                or not symbol["st_size"]
            ):
                continue
            data = read(symbol["st_value"], symbol["st_size"])
            record = dict(
                name=demangled.get(symbol.name, symbol.name),
                address=f"0x{symbol['st_value']:08x}",
                size=symbol["st_size"],
                file_bytes=len(data),
                sha256=hashlib.sha256(data).hexdigest(),
            )
            if len(data) <= 256:
                record["data_hex"] = data.hex()
            if len(data) == 4:
                value = struct.unpack("<f", data)[0]
                record["u32"] = struct.unpack("<I", data)[0]
                if math.isfinite(value):
                    record["f32"] = value
            if symbol.name == "b2_version" and len(data) == 12:
                native_physics["b2_version"] = list(struct.unpack("<iii", data))
            if symbol.name in (
                "_ZN3App14CollisionWorld19WorldToPhysicsScaleE",
                "_ZN3App14CollisionWorld19PhysicsToWorldScaleE",
            ):
                native_physics[record["name"]] = struct.unpack("<f", data)[0]
            data_symbols.append(record)
        (output / "data-symbols.json").write_text(
            json.dumps(data_symbols, ensure_ascii=False, indent=2), encoding="utf-8"
        )

        by_range = defaultdict(list)
        for s in functions:
            by_range[(s["st_value"] & ~1, function_size(s))].append(s)
        for (start, size), aliases in sorted(by_range.items()):
            chosen = next((s for s in aliases if s.name.startswith("_ZN3App")), aliases[0])
            name = demangled.get(chosen.name, chosen.name)
            group = re.sub(r"[^A-Za-z0-9_.-]+", "_", group_name(name))
            code = read(start, size)
            if len(code) != size:
                raise RuntimeError("Unreadable function: " + name)
            rec = dict(
                address=f"0x{start:08x}",
                size=size,
                name=name,
                aliases=[dict(symbol=s.name, name=demangled.get(s.name, s.name)) for s in aliases],
                zero_size_aliases=[s.name for s in aliases if not s["st_size"]],
                group=group,
                sha256=hashlib.sha256(code).hexdigest(),
                review="unreviewed",
                calls=[],
                branches=[],
                indirect=[],
                literal_words=[],
                data_bytes=0,
                undecoded_bytes=0,
                instruction_count=0,
            )
            lines = [
                f"FUNCTION 0x{start:08x} size={size} {name}",
                "ALIASES " + " | ".join(s.name for s in aliases),
            ]
            cursor = start
            while cursor < start + size:
                mi = bisect.bisect_right(map_addresses, cursor) - 1
                mode = mappings[mi][1] if mi >= 0 else ("t" if chosen["st_value"] & 1 else "a")
                stop = min(
                    start + size, map_addresses[mi + 1] if mi + 1 < len(mappings) else start + size
                )
                if mode == "d":
                    data = read(cursor, stop - cursor)
                    rec["data_bytes"] += len(data)
                    lines.append(f"{cursor:08x}: .data {data.hex()}")
                    cursor = stop
                    continue
                engine = thumb if mode == "t" else arm
                advanced = 0
                for ins in engine.disasm(read(cursor, stop - cursor), cursor):
                    rec["instruction_count"] += 1
                    advanced += ins.size
                    notes = []
                    if ins.group(CS_GRP_CALL) or ins.group(CS_GRP_JUMP):
                        if ins.operands and ins.operands[0].type == ARM_OP_IMM:
                            target = ins.operands[0].imm & 0xFFFFFFFF
                            refs = all_targets.get(target & ~1, [])
                            notes.extend(refs)
                            edge = dict(at=f"0x{ins.address:08x}", to=f"0x{target:08x}", names=refs)
                            rec["calls" if ins.group(CS_GRP_CALL) else "branches"].append(edge)
                        elif ins.mnemonic not in ("bx",) or ins.op_str != "lr":
                            rec["indirect"].append(
                                dict(
                                    at=f"0x{ins.address:08x}",
                                    instruction=ins.mnemonic + " " + ins.op_str,
                                )
                            )
                    for op in ins.operands:
                        if op.type == ARM_OP_MEM and op.mem.base == ARM_REG_PC and not op.mem.index:
                            pc = (ins.address + (4 if mode == "t" else 8)) & (
                                ~3 if mode == "t" else 0xFFFFFFFF
                            )
                            loc = pc + op.mem.disp
                            data = read(loc, 4)
                            if len(data) == 4:
                                word = struct.unpack("<I", data)[0]
                                value = struct.unpack("<f", data)[0]
                                rec["literal_words"].append(
                                    dict(
                                        at=f"0x{ins.address:08x}",
                                        pool=f"0x{loc:08x}",
                                        word=f"0x{word:08x}",
                                    )
                                )
                                notes.append(f"pool@{loc:08x}=0x{word:08x} float={value:.9g}")
                    lines.append(
                        f"{ins.address:08x}: {ins.mnemonic:9} {ins.op_str:36} " + " | ".join(notes)
                    )
                cursor += advanced
                if cursor < stop:
                    gap = read(cursor, stop - cursor)
                    rec["undecoded_bytes"] += len(gap)
                    lines.append(f"{cursor:08x}: .undecoded {gap.hex()}")
                    cursor = stop
            rec["assembly"] = f"assembly/{group}.asm"
            records.append(rec)
            artifacts[group].extend(lines + [""])
            if text_start <= start < text_end:
                owned.append((start, min(start + size, text_end)))
        for group, lines in artifacts.items():
            (output / "assembly" / (group + ".asm")).write_text("\n".join(lines), encoding="utf-8")
        cursor, gaps = text_start, []
        for start, end in sorted(owned):
            if start > cursor:
                gaps.append(dict(address=f"0x{cursor:08x}", size=start - cursor))
            cursor = max(cursor, end)
        if cursor < text_end:
            gaps.append(dict(address=f"0x{cursor:08x}", size=text_end - cursor))
        gap_lines = []
        for gap in gaps:
            start = int(gap["address"], 16)
            data = read(start, gap["size"])
            gap["sha256"] = hashlib.sha256(data).hexdigest()
            mi = bisect.bisect_right(map_addresses, start) - 1
            mode = mappings[mi][1] if mi >= 0 else "a"
            gap["mapping"] = mode
            gap_lines.append(f"REGION {gap['address']} size={len(data)} mapping={mode}")
            if mode == "d":
                gap_lines.append(".data " + data.hex())
            else:
                engine = thumb if mode == "t" else arm
                gap_lines.extend(
                    f"{i.address:08x}: {i.mnemonic:9} {i.op_str}"
                    for i in engine.disasm(data, start)
                )
                gap_lines.append(".raw " + data.hex())
        (output / "assembly/TextOutsideFunctions.asm").write_text(
            "\n".join(gap_lines) + "\n", encoding="utf-8"
        )
        vtables = []
        for symbol in symbols:
            if symbol.name.startswith("_ZTV") and symbol["st_size"]:
                data = read(symbol["st_value"], symbol["st_size"])
                words = []
                for offset in range(0, len(data) - 3, 4):
                    value = struct.unpack_from("<I", data, offset)[0]
                    words.append(
                        dict(
                            offset=offset,
                            value=f"0x{value:08x}",
                            targets=all_targets.get(value & ~1, []),
                        )
                    )
                vtables.append(
                    dict(
                        name=demangled.get(symbol.name, symbol.name),
                        address=f"0x{symbol['st_value']:08x}",
                        entries=words,
                    )
                )
        (output / "vtables.json").write_text(
            json.dumps(vtables, ensure_ascii=False, indent=2), encoding="utf-8"
        )
        plt = elf.get_section_by_name(".plt")
        if plt:
            (output / "assembly/PLT.asm").write_text(
                "\n".join(
                    f"{i.address:08x}: {i.mnemonic:9} {i.op_str}"
                    for i in arm.disasm(plt.data(), plt["sh_addr"])
                ),
                encoding="utf-8",
            )
        summary = dict(
            library_sha256=hashlib.sha256(blob).hexdigest(),
            function_symbols=len(functions),
            unique_function_ranges=len(records),
            sized_function_symbols=sum(bool(s["st_size"]) for s in functions),
            zero_size_function_symbols=sum(not s["st_size"] for s in functions),
            function_addresses=len(set(s["st_value"] & ~1 for s in functions)),
            groups=dict(sorted(Counter(r["group"] for r in records).items())),
            text_bytes=text["sh_size"],
            text_bytes_outside_functions=sum(g["size"] for g in gaps),
            text_gaps=gaps,
            instruction_count=sum(r["instruction_count"] for r in records),
            inline_data_bytes=sum(r["data_bytes"] for r in records),
            undecoded_bytes=sum(r["undecoded_bytes"] for r in records),
            all_symbols=len(symbol_manifest),
            sections=len(section_manifest),
            relocations=len(relocations),
            vtables=len(vtables),
            native_physics=native_physics,
            review_status="Inventory complete; semantic review is NOT complete.",
        )
    (output / "functions.json").write_text(
        json.dumps(records, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    (output / "summary.json").write_text(
        json.dumps(summary, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    index = [
        "# Native function inventory",
        "",
        summary["review_status"],
        "",
        f"Library SHA256: `{summary['library_sha256']}`",
        "",
        "| Group | Unique function ranges | Assembly |",
        "| --- | ---: | --- |",
    ]
    index.extend(f"| {g} | {n} | [{g}](assembly/{g}.asm) |" for g, n in summary["groups"].items())
    (output / "index.md").write_text("\n".join(index) + "\n", encoding="utf-8")
    print(
        json.dumps({k: v for k, v in summary.items() if k not in ("groups", "text_gaps")}, indent=2)
    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "build/native-review")
    inventory(parser.parse_args().output)
