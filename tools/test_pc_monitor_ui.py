"""Render the actual LVGL app05 UI on a host compiler, without device hardware.
Usage: python tools/test_pc_monitor_ui.py --cc C:/msys64/mingw64/bin/gcc.exe
Generated binaries and screenshots stay in ignored output/.
"""
import argparse
import concurrent.futures
import os
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--cc', default='gcc')
args = parser.parse_args()
out = root / 'output' / 'pc-monitor-ui'
out.mkdir(parents=True, exist_ok=True)
includes = [root/'lib', root/'lib/lvgl', root/'test/pc_monitor_ui']
sources = sorted((root/'lib/lvgl/src').rglob('*.c')) + [
    root/'src/app/app_05/asset/ui.c', root/'src/app/app_05/asset/ui_PC_Monitor.c',
    root/'src/app/app_05/asset/ui_font_pc_temp.c', root/'test/pc_monitor_ui/render.c']
def compile_one(source):
    obj = out / (str(source.relative_to(root)).replace('\\','_').replace('/','_') + '.o')
    # Rebuild for header changes too: this is a verification tool, not a cache.
    cmd = [args.cc, '-std=c99', '-O0', '-DIRAM_ATTR=', '-c', str(source), '-o', str(obj)]
    cmd += ['-I'+str(p) for p in includes]
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode: raise RuntimeError(result.stderr)
    return obj
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
    objects = list(pool.map(compile_one, sources))
response = out/'objects.rsp'
response.write_text('\n'.join('"'+p.as_posix()+'"' for p in objects), encoding='utf-8')
exe = out/'render.exe'
subprocess.run([args.cc, '@'+str(response), '-lm', '-o', str(exe)], check=True)
env = os.environ.copy()
env['PATH'] = str(Path(args.cc).resolve().parent) + os.pathsep + env.get('PATH','')
subprocess.run([str(exe)], cwd=root, env=env, check=True)
