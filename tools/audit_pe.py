"""Audit the PE32 executable's architecture, XP header and imported DLLs."""

from pathlib import Path
import struct, sys, json

root = Path(__file__).resolve().parents[1]
path = Path(sys.argv[1]) if len(sys.argv) > 1 else root / "bin/BeansQuest.exe"
b = path.read_bytes()
u16 = lambda p: struct.unpack_from("<H", b, p)[0]
u32 = lambda p: struct.unpack_from("<I", b, p)[0]
pe = u32(0x3C)
assert b[pe : pe + 4] == b"PE\0\0"
opt = pe + 24
assert u16(opt) == 0x10B and u16(pe + 4) == 0x14C
assert (u16(opt + 40), u16(opt + 42)) == (5, 1)
assert (u16(opt + 48), u16(opt + 50)) == (5, 1)
sections = opt + u16(pe + 20)


def offset(rva):
    for i in range(u16(pe + 6)):
        p = sections + i * 40
        va = u32(p + 12)
        if va <= rva < va + max(u32(p + 8), u32(p + 16)):
            return u32(p + 20) + rva - va
    raise ValueError(f"Unmapped RVA {rva:x}")


def string(p):
    return b[p : b.index(b"\0", p)].decode("ascii")


imports = {}
p = offset(u32(opt + 104))
while u32(p + 12):
    dll = string(offset(u32(p + 12)))
    names = []
    t = offset(u32(p) or u32(p + 16))
    while u32(t):
        entry = u32(t)
        assert not entry & 0x80000000, "Unexpected ordinal import"
        names.append(string(offset(entry) + 2))
        t += 4
    imports[dll] = names
    p += 20
assert set(d.upper() for d in imports) == {
    "KERNEL32.DLL",
    "USER32.DLL",
    "GDI32.DLL",
    "OPENGL32.DLL",
    "WINMM.DLL",
}, imports.keys()
report = {
    "file": str(path),
    "architecture": "PE32 / x86",
    "os_version": "5.01",
    "subsystem_version": "5.01",
    "crt_dependency": False,
    "imports": imports,
    "xp_machine_tested": False,
}
out = root / "build/pe-audit.json"
out.write_text(json.dumps(report, indent=2), encoding="utf-8")
print(
    "PASS: PE32/x86; OS/subsystem 5.01; only five system DLLs; no CRT. XP machine testing remains outstanding."
)
