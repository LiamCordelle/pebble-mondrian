#!/usr/bin/env python3
"""Compare final QEMU status pixels with the verified host-rendered scenes."""
from pathlib import Path
import re
from PIL import Image,ImageChops

ROOT=Path(__file__).resolve().parents[1]
SHOTS=ROOT/'shots'
def crop(name,box):return Image.open(SHOTS/name).convert('RGB').crop(box)
def same(a,b):return ImageChops.difference(a,b).getbbox() is None
for emulator,render in [('emulator-low-battery.png','low-battery.png'),('emulator-charging.png','charging.png')]:
    assert same(crop(emulator,(0,212,80,228)),crop(render,(0,212,80,228))),emulator
assert same(crop('emulator-disconnected.png',(0,76,28,102)),crop('no-phone.png',(0,76,28,102)))
assert same(crop('emulator-final.png',(0,76,28,102)),crop('demo.png',(0,76,28,102)))
assert same(crop('emulator-original-live.png',(0,108,80,212)),crop('emulator-restarted.png',(0,108,80,212)))
logs='\n'.join((SHOTS/name).read_text() for name in ['emulator.log','emulator-data.log'])
assert not re.search(r'APP_FAULT|stack overflow|JS failed|app crash',logs,re.I)
assert 'Still allocated <0B>' in logs
frames=[int(n) for n in re.findall(r'Frame \w+ (\d+)ms',logs)]
print(f'Emulator checks passed: battery/charging/disconnect pixels, restored connection, persisted forecast, clean app exit. Draws {min(frames)}–{max(frames)} ms.')
Image.open(SHOTS/'emulator-final.png').save(ROOT/'screenshot_emery.png')
