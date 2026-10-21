"""Package a runnable copy without test output or the user's save files."""

from pathlib import Path
import argparse, zipfile, shutil
import os, tempfile
from audit_release import audit_binary

ROOT = Path(__file__).resolve().parents[1]
source = ROOT / "bin"
out = source / "BeanQuest-Windows-x86.zip"
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument(
    "--executable",
    default="BeansQuest.exe",
    help="Built executable in bin to package under the normal BeansQuest.exe name",
)
executable = source / parser.parse_args().executable
if not executable.is_file():
    raise SystemExit(f"Executable does not exist: {executable}")
audit_binary(executable)
# Only runtime assets and required license notices belong in a release.
# Remove old development-document copies as well as excluding them from ZIPs.
license_notices = ("LICENSE", "THIRD_PARTY_NOTICES.txt")
for name in license_notices:
    shutil.copy2(ROOT / name, source / name)
development_docs = ("README.md", "README_zh.md", "NATIVE_REVIEW.md", "PORTING_AUDIT.txt")
for name in development_docs:
    stale = source / name
    if stale.is_file():
        stale.unlink()
# Build and verify beside compiler outputs before replacing the published ZIP.
# A failed compression or a locked destination must leave the old package intact.
scratch = ROOT / "build"
scratch.mkdir(exist_ok=True)
handle, temporary_name = tempfile.mkstemp(
    prefix="BeanQuest-Windows-x86-", suffix=".zip", dir=scratch
)
os.close(handle)
temporary = Path(temporary_name)
with zipfile.ZipFile(temporary, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as z:
    # Use the normal executable name even when validating a staged candidate.
    z.write(executable, "BeanQuest/BeansQuest.exe")
    for p in sorted(source.rglob("*")):
        if p.is_file() and (
            (p.parent == source and p.name in license_notices)
            or p.relative_to(source).parts[0] == "data"
        ):
            z.write(p, "BeanQuest/" + p.relative_to(source).as_posix())
    # Ship an empty save directory; never read or archive personal progress.
    z.writestr("BeanQuest/saves/", b"")
with zipfile.ZipFile(temporary) as z:
    # Verify the written artifact, including the absence of development docs.
    assert z.testzip() is None
    assert "BeanQuest/BeansQuest.exe" in z.namelist()
    assert "BeanQuest/saves/" in z.namelist()
    assert z.read("BeanQuest/BeansQuest.exe") == executable.read_bytes()
    assert not any("BeanQuest/" + name in z.namelist() for name in development_docs)
    for name in license_notices:
        assert z.read("BeanQuest/" + name) == (ROOT / name).read_bytes()
    assert not any(
        n.startswith("BeanQuest/saves/") and n != "BeanQuest/saves/" for n in z.namelist()
    )
try:
    os.replace(temporary, out)
except PermissionError:
    raise SystemExit(f"Destination ZIP is locked; verified package retained at: {temporary}")
print(f"Created and verified: {out} ({out.stat().st_size/1024/1024:.1f} MiB)")
