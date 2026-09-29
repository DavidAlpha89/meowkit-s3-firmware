"""Package the user-tested HSM archive with its vendored source and docs.

Requires py7zr. No EXE is executed. Only the explicit runtime allowlist is
extracted; saved device IDs, uninstallers, debug files and autorun scripts
from the original installation are never redistributed.
"""
import argparse
import hashlib
from pathlib import Path
import tempfile
import zipfile
import py7zr

root = Path(__file__).resolve().parents[1]
base = root / 'software' / 'HardwareSerialMonitor'
runtime = ['HardwareSerialMonitor.exe', 'HardwareSerialMonitor.ico',
           'INIFileParser.dll', 'OpenHardwareMonitorLib.dll', 'System.CodeDom.dll',
           'LICENSE.txt']
parser = argparse.ArgumentParser()
parser.add_argument('archive', type=Path)
args = parser.parse_args()
expected_exe = '30765564bf2f6cc7021d58f203d53bf3ea9dc376a3b90f1617726061175f4c4b'
expected_ohm = 'c0c0c4b4e0c5958c2d6608b94c32c52107576fb34504820caa9b191f15757414'
def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

dest = base / 'downloads' / 'HardwareSerialMonitor-v1.4.4-MeowKit.zip'
dest.parent.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix='meowkit-hsm-') as tmp:
    targets = ['HardwareSerialMonitor/' + name for name in runtime]
    with py7zr.SevenZipFile(args.archive) as archive:
        if not set(targets).issubset(archive.getnames()):
            raise SystemExit('Archive is missing required runtime files')
        archive.extract(path=tmp, targets=targets)
    folder = Path(tmp) / 'HardwareSerialMonitor'
    if digest(folder/'HardwareSerialMonitor.exe') != expected_exe:
        raise SystemExit('Unexpected EXE: review source provenance before packaging')
    if digest(folder/'OpenHardwareMonitorLib.dll') != expected_ohm:
        raise SystemExit('Unexpected sensor library')
    files = [(folder/name, name) for name in runtime]
    files += [(base/name, name) for name in ['README.md', 'PROVENANCE.md', 'Start.cmd']]
    files += [(p, p.relative_to(base).as_posix()) for p in sorted((base/'source').rglob('*'))
              if p.is_file() and p.suffix not in ('.mdb', '.bak', '.pdb', '.user', '.suo')
              and p.name not in ('Resources - Copy', 'Config.ini')
              and not {'bin', 'obj', '.vs'}.intersection(p.relative_to(base).parts)]
    # Fixed timestamps and ordering make the same inputs reproducible.
    with zipfile.ZipFile(dest, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
        for path, name in sorted(files, key=lambda item:item[1]):
            info = zipfile.ZipInfo(name, date_time=(2026,9,29,0,0,0))
            info.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(info, path.read_bytes())
    with zipfile.ZipFile(dest) as archive:
        assert archive.testzip() is None
        assert 'Config.ini' not in archive.namelist()
print(f'{dest.name}: {dest.stat().st_size} bytes')
print('SHA256:', digest(dest))
