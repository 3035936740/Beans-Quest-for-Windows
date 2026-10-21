"""Print exact function evidence without mixing RTTI/data symbols into code.

The optional concise view shows calls, floating point operations and candidate
PC-relative string references. This is not a decompiler or an equivalence test.
"""

from pathlib import Path
import argparse
import bisect
import json
import struct
from capstone import Cs, CS_ARCH_ARM, CS_MODE_ARM, CS_MODE_THUMB
from capstone.arm import ARM_OP_REG, ARM_OP_IMM, ARM_OP_MEM, ARM_REG_PC
from elftools.elf.elffile import ELFFile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("query", nargs="+")
    parser.add_argument("--concise", action="store_true")
    parser.add_argument("--strings", action="store_true")
    args = parser.parse_args()
    functions = json.loads(
        (ROOT / "build/native-review/functions.json").read_text(encoding="utf-8")
    )
    symbols = json.loads((ROOT / "build/native-review/symbols.json").read_text(encoding="utf-8"))
    mappings = sorted(
        (int(s["address"], 16), s["name"][1])
        for s in symbols
        if s["name"].startswith(("$a", "$t", "$d")) and (len(s["name"]) == 2 or s["name"][2] == ".")
    )
    map_addresses = [m[0] for m in mappings]
    blob = (ROOT / "original/libBeansQuest.so").read_bytes()
    with (ROOT / "original/libBeansQuest.so").open("rb") as stream:
        elf = ELFFile(stream)
        segments = [dict(s.header) for s in elf.iter_segments() if s["p_type"] == "PT_LOAD"]

    def read(address, size):
        for s in segments:
            if s["p_vaddr"] <= address and address + size <= s["p_vaddr"] + s["p_filesz"]:
                offset = s["p_offset"] + address - s["p_vaddr"]
                return blob[offset : offset + size]
        return b""

    def cstring(address):
        raw = read(address, 128).split(b"\0")[0]
        return (
            raw.decode("ascii") if 3 <= len(raw) < 128 and all(32 <= c < 127 for c in raw) else ""
        )

    engine = Cs(CS_ARCH_ARM, CS_MODE_ARM)
    engine.detail = True
    thumb = Cs(CS_ARCH_ARM, CS_MODE_THUMB)
    thumb.detail = True
    for f in functions:
        if not any(
            q in f["name"] or q == f["address"] or any(q in a["symbol"] for a in f["aliases"])
            for q in args.query
        ):
            continue
        print(f"\nFUNCTION {f['address']} size={f['size']} {f['name']}")
        registers = {}
        calls = {int(c["at"], 16): c for c in f["calls"]}
        literals = {int(c["at"], 16): c for c in f["literal_words"]}
        strings = []
        instructions = []
        cursor = int(f["address"], 16)
        end = cursor + f["size"]
        while cursor < end:
            mi = bisect.bisect_right(map_addresses, cursor) - 1
            mode = mappings[mi][1] if mi >= 0 else "a"
            stop = min(end, map_addresses[mi + 1] if mi + 1 < len(mappings) else end)
            if mode != "d":
                instructions.extend(
                    (ins, mode)
                    for ins in (thumb if mode == "t" else engine).disasm(
                        read(cursor, stop - cursor), cursor
                    )
                )
            cursor = stop
        for i, mode in instructions:
            ops = i.operands
            value = None
            candidate = ""

            def val(op):
                if op.type == ARM_OP_IMM:
                    return op.imm
                if op.type == ARM_OP_REG:
                    return (
                        ((i.address + 4) & ~3 if mode == "t" else i.address + 8)
                        if op.reg == ARM_REG_PC
                        else registers.get(op.reg)
                    )
                return None

            if len(ops) >= 2 and ops[0].type == ARM_OP_REG:
                if i.mnemonic in ("mov", "movw"):
                    value = val(ops[1])
                elif i.mnemonic == "movt" and ops[0].reg in registers:
                    value = (registers[ops[0].reg] & 0xFFFF) | (ops[1].imm << 16)
                elif i.mnemonic in ("add", "sub") and len(ops) == 3:
                    a, b = val(ops[1]), val(ops[2])
                    if a is not None and b is not None:
                        value = (a + b if i.mnemonic == "add" else a - b) & 0xFFFFFFFF
                        if ops[1].type == ARM_OP_REG and ops[1].reg == ARM_REG_PC:
                            candidate = cstring(value)
                elif i.mnemonic == "ldr" and ops[1].type == ARM_OP_MEM:
                    mem = ops[1].mem
                    base = (
                        ((i.address + 4) & ~3 if mode == "t" else i.address + 8)
                        if mem.base == ARM_REG_PC
                        else registers.get(mem.base)
                    )
                    index = 0 if not mem.index else registers.get(mem.index)
                    if base is not None and index is not None:
                        data = read(base + index + mem.disp, 4)
                        if len(data) == 4:
                            value = struct.unpack("<I", data)[0]
            note = []
            if candidate:
                note.append("string=" + repr(candidate))
                strings.append(candidate)
            if i.address in calls:
                note.extend(calls[i.address]["names"])
            if i.address in literals:
                raw = int(literals[i.address]["word"], 16)
                note.append(
                    f"pool=0x{raw:08x} f32={struct.unpack('<f',struct.pack('<I',raw))[0]:.9g}"
                )
            if not args.strings and (not args.concise or note or i.mnemonic.startswith("v")):
                print(f"{i.address:08x}: {i.mnemonic:10} {i.op_str:35} " + " | ".join(note))
            try:
                for reg in i.regs_access()[1]:
                    registers.pop(reg, None)
            except Exception:
                registers.clear()
            if value is not None:
                registers[ops[0].reg] = value
            if i.address in calls:
                for reg in list(registers):
                    if engine.reg_name(reg) in ("r0", "r1", "r2", "r3", "ip"):
                        registers.pop(reg, None)
        if args.strings:
            print("\n".join(dict.fromkeys(strings)))


if __name__ == "__main__":
    main()
