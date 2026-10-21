"""Reject developer test and diagnostic payloads in distributed executables."""

from pathlib import Path
import argparse
import struct

MARKERS = (
    "test-output",
    "--self-test",
    "--audio-test",
    "live-jump-audio.csv",
    "live-jump-physics.csv",
    "sequence,tick_ms,end_tick_ms",
    "sound_sequence_before",
)


def audit_binary(path):
    data = Path(path).read_bytes()
    assert data[:2] == b"MZ", "Expected a Windows executable"
    found = [s for s in MARKERS if s.encode("ascii") in data or s.encode("utf-16le") in data]
    if found:
        raise ValueError("Test/diagnostic payload in release: " + ", ".join(found))
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    assert data[pe : pe + 4] == b"PE\0\0"
    assert struct.unpack_from("<II", data, pe + 12) == (0, 0), "COFF symbol table remains"
    opt = pe + 24
    assert struct.unpack_from("<H", data, opt)[0] == 0x10B
    rva, size = struct.unpack_from("<II", data, opt + 144)
    if rva:
        count = struct.unpack_from("<H", data, pe + 6)[0]
        sections = opt + struct.unpack_from("<H", data, pe + 20)[0]
        for i in range(count):
            section = sections + 40 * i
            extent, address, raw_size, raw = struct.unpack_from("<IIII", data, section + 8)
            if address <= rva < address + max(extent, raw_size):
                offset = raw + rva - address
                break
        else:
            raise ValueError("Unmapped debug directory")
        assert size % 28 == 0
        for entry in range(offset, offset + size, 28):
            kind = struct.unpack_from("<I", data, entry + 12)[0]
            # MSVC can retain POGO section-group metadata (type13) without
            # symbols. CodeView/PDB and COFF debug-symbol entries are rejected.
            assert kind not in (1, 2), "Debug symbol/PDB entry remains"
    return len(data)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable")
    args = parser.parse_args()
    size = audit_binary(args.executable)
    print(f"PASS release payload: {size} bytes; no test entry/output or diagnostic markers.")
