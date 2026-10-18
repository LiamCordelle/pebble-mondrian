#!/usr/bin/env python3
"""Check the phone-lost marker after PebbleOS's 25-second disconnect debounce."""
import os
from pathlib import Path
import subprocess
import time
ROOT=Path(__file__).resolve().parents[1]
def cli(*args):subprocess.run(['pebble',*args,'--emulator','emery'],cwd=ROOT,check=True,timeout=60)
env=os.environ.copy();env['PYTHONUNBUFFERED']='1'
with (ROOT/'shots/emulator-connection.log').open('w') as output:
    logs=subprocess.Popen(['pebble','logs','--emulator','emery'],cwd=ROOT,stdout=output,stderr=subprocess.STDOUT,env=env)
    try:
        cli('emu-bt-connection','--connected','no')
        print('Waiting for PebbleOS disconnect debounce (25 seconds).',flush=True)
        time.sleep(27)
        cli('screenshot','--no-open',str(ROOT/'shots/emulator-disconnected.png'))
    finally:
        cli('emu-bt-connection','--connected','yes')
        time.sleep(1)
        cli('screenshot','--no-open',str(ROOT/'shots/emulator-final.png'))
        logs.terminate()
        try:logs.wait(timeout=5)
        except subprocess.TimeoutExpired:logs.kill();logs.wait()
