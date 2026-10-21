"""Verify whole-ELF inventory coverage. This does NOT prove semantic parity."""

from collections import Counter
from pathlib import Path
import hashlib
import json
from elftools.elf.elffile import ELFFile

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "build/native-review"


def main():
    blob = (ROOT / "original/libBeansQuest.so").read_bytes()
    summary = json.loads((OUT / "summary.json").read_text(encoding="utf-8"))
    functions = json.loads((OUT / "functions.json").read_text(encoding="utf-8"))
    symbols = json.loads((OUT / "symbols.json").read_text(encoding="utf-8"))
    assert hashlib.sha256(blob).hexdigest() == summary["library_sha256"]
    expected = Counter(
        (s["name"], int(s["address"], 16) & ~1)
        for s in symbols
        if s["type"] == "STT_FUNC" and s["section"] != "SHN_UNDEF" and int(s["address"], 16)
    )
    actual = Counter((a["symbol"], int(f["address"], 16)) for f in functions for a in f["aliases"])
    assert actual == expected, "Missing/duplicated defined function or alias"
    with (ROOT / "original/libBeansQuest.so").open("rb") as stream:
        elf = ELFFile(stream)
        segments = [dict(s.header) for s in elf.iter_segments() if s["p_type"] == "PT_LOAD"]
        sections = json.loads((OUT / "sections.json").read_text(encoding="utf-8"))
        for s, record in zip(elf.iter_sections(), sections):
            assert (
                s.name == record["name"]
                and hashlib.sha256(s.data()).hexdigest() == record["sha256"]
            )
        assert elf.num_sections() == len(sections)

        def read(address, size):
            for s in segments:
                if s["p_vaddr"] <= address and address + size <= s["p_vaddr"] + s["p_filesz"]:
                    p = s["p_offset"] + address - s["p_vaddr"]
                    return blob[p : p + size]
            raise AssertionError("Unmapped address " + hex(address))

        text = elf.get_section_by_name(".text")
        start = text["sh_addr"]
        end = start + text["sh_size"]
        ranges = []
        for f in functions:
            address = int(f["address"], 16)
            data = read(address, f["size"])
            assert hashlib.sha256(data).hexdigest() == f["sha256"], f["name"]
            # This supplied library's defined functions are ARM32, including
            # its zero-size unwinding stubs. Do not silently accept Thumb code.
            assert (
                f["instruction_count"] * 4 + f["data_bytes"] + f["undecoded_bytes"] == f["size"]
            ), f["name"]
            assert f["undecoded_bytes"] == 0, f["name"]
            if start <= address < end:
                ranges.append((address, min(end, address + f["size"])))
        for gap in summary["text_gaps"]:
            address = int(gap["address"], 16)
            data = read(address, gap["size"])
            assert hashlib.sha256(data).hexdigest() == gap["sha256"]
            ranges.append((address, address + gap["size"]))
        cursor = start
        for a, b in sorted(ranges):
            assert a <= cursor, "Unaccounted text gap at " + hex(cursor)
            cursor = max(cursor, b)
        assert cursor == end
    print(
        f"PASS all {sum(expected.values())} defined function symbols, {len(functions)} function ranges, every .text byte, and all section hashes."
    )
    print(
        "This is evidence integrity/coverage only; full semantic reconstruction remains incomplete."
    )


if __name__ == "__main__":
    main()
