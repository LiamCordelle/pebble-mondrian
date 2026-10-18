#!/usr/bin/env python3
"""One final QEMU smoke check; visual iteration uses render_scenes.py."""
import os
from pathlib import Path
import subprocess
import time

ROOT=Path(__file__).resolve().parents[1]
SHOTS=ROOT/'shots'
SHOTS.mkdir(exist_ok=True)
def run(*args):
    print(' '.join(args),flush=True)
    result=subprocess.run(args,cwd=ROOT,check=True,text=True,capture_output=True,timeout=60)
    with (SHOTS/'emulator-commands.txt').open('a') as f:
        f.write(' '.join(args)+'\n'+result.stdout+result.stderr+'\n')
    return result
def shot(name):run('pebble','screenshot','--no-open','--emulator','emery',str(SHOTS/name))
with (SHOTS/'emulator.log').open('w') as output:
    env=os.environ.copy();env['PYTHONUNBUFFERED']='1'
    logs=subprocess.Popen(['pebble','logs','--emulator','emery'],cwd=ROOT,stdout=output,stderr=subprocess.STDOUT,env=env)
    try:
        print('Starting emulator log capture; allowing boot to finish.',flush=True)
        time.sleep(20)
        run('pebble','emu-button','click','back','--emulator','emery')
        run('pebble','emu-steps','6842','--emulator','emery')
        run('pebble','emu-battery','--percent','76','--emulator','emery')
        run('pebble','install','--emulator','emery')
        run('pebble','emu-heart-rate','64','--emulator','emery')
        time.sleep(2)
        shot('emulator-live.png')
        run('pebble','emu-tap','--emulator','emery')
        time.sleep(1)
        shot('emulator-detail.png')
        time.sleep(8)
        shot('emulator-timeout.png')
        run('pebble','emu-tap','--emulator','emery')
        run('pebble','emu-tap','--emulator','emery')
        shot('emulator-toggle.png')
        run('pebble','emu-battery','--percent','28','--emulator','emery')
        time.sleep(1)
        shot('emulator-low-battery.png')
        run('pebble','emu-battery','--percent','62','--charging','--emulator','emery')
        time.sleep(1)
        shot('emulator-charging.png')
        run('pebble','emu-battery','--percent','76','--emulator','emery')
        print('Emulator screenshots captured; review images and emulator.log.',flush=True)
    finally:
        logs.terminate()
        try:logs.wait(timeout=5)
        except subprocess.TimeoutExpired:logs.kill();logs.wait()
