#!/usr/bin/env python3
"""Run the shared regression suite and this consumer's configuration checks."""
from pathlib import Path
import subprocess
import sys
ROOT = Path(__file__).resolve().parents[1]
subprocess.run([sys.executable, str(ROOT/'shared-kit/tests/run.py')], check=True)
subprocess.run(['node', 'tests/consumer.test.js'], cwd=ROOT, check=True)

output=ROOT/'build/tests/face-settings'
output.parent.mkdir(parents=True,exist_ok=True)
subprocess.run(['cc','-std=c11','-Itests/face-stubs','-Wall','-Wextra','-Werror',
    'tests/face_settings.test.c','src/c/face_settings.c',
    'shared-kit/src/c/kit/kit_settings.c','shared-kit/src/c/kit/kit_calc.c',
    '-o',str(output)],cwd=ROOT,check=True)
subprocess.run([str(output)],cwd=ROOT,check=True)
