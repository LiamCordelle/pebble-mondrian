#!/usr/bin/env python3
"""Check the live goal colours and second-timezone clock with shared QEMU APIs.

Run with pebble-tool's Python. Uses synthetic health and a disposable profile.
"""
import datetime as dt
import json
import os
from pathlib import Path
import subprocess
import sys
from PIL import Image, ImageChops

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'shared-kit/tools'))
from emulator import EmulatorSession
from emulator_smoke import common_settings


def check_clock(emu, zone, h12, large=False):
    settings = dict(common_settings(8000), WORLD=zone, TIME_MODE=1 if h12 else 2,
                    LARGE_LABELS=large)
    emu.apply_clay(settings)
    emu.restart()
    mark = emu.logs.mark()
    emu.cli('emu-tap')
    emu.logs.wait('Frame detail', since=mark)
    stamp = dt.datetime.now(dt.timezone.utc)
    name = f'clock-{zone}-{12 if h12 else 24}'+('-large' if large else '')
    actual = emu.screenshot(name+'.png')
    # Fixture local time is UTC+1 on 17 October. UTC/Tokyo have no DST,
    # so align its UTC minute to the live one for an independent glyph check.
    expected = emu.output/(name+'-expected.png')
    env = dict(os.environ, MONDRIAN_WORLD=str(zone), MONDRIAN_12H=str(int(h12)),
               MONDRIAN_LARGE_LABELS=str(int(large)),
               MONDRIAN_HOUR=str((stamp.hour+1)%24), MONDRIAN_MINUTE=str(stamp.minute))
    subprocess.run(['python3', str(ROOT.parent/'watchface-lab/tools/render.py'),
                    str(ROOT), str(expected), '--detail'], env=env, check=True,
                   capture_output=True)
    crop = (0, 0, 72, 56)
    left = Image.open(actual).convert('RGB').crop(crop)
    right = Image.open(expected).convert('RGB').crop(crop)
    assert ImageChops.difference(left, right).getbbox() is None, name
    print('Verified second-timezone clock:', name, flush=True)


with EmulatorSession(ROOT, ROOT/'shots/emulator-improvements') as emu:
    emu.install()
    emu.start()
    emu.logs.wait('Kit settings loaded: goal=8000')
    # Emulator steps are cumulative synthetic health values; a restart ensures
    # each frame uses a fresh model without waiting for a minute tick.
    reference = Image.open(ROOT/'shots/demo.png').convert('RGB')
    palette = [reference.getpixel(point) for point in
               [(194,10), (194,225), (74,195)]]
    palette.append((142,227,145))  # SDK LCD correction of GColorGreen.
    emu.apply_clay(common_settings(8000))
    for pct, top, bottom in [(100,1,1), (150,1,2), (200,2,2),
                              (250,2,3), (300,3,3), (350,3,1)]:
        emu.cli('emu-steps', str(pct*80))
        emu.restart()
        path = emu.screenshot(f'goal-{pct}.png')
        image = Image.open(path).convert('RGB')
        assert image.getpixel((194,111)) == palette[top], (pct,'previous colour')
        assert image.getpixel((194,225)) == palette[bottom], (pct,'current colour')
        print('Verified live step fill:', pct, flush=True)
    for zone, h12, large in [(5,False,False), (5,True,False), (1,False,True)]:
        check_clock(emu, zone, h12, large)
    mark = emu.logs.mark()
    emu.logs.wait('Frame face', since=mark, timeout=12)
    emu.screenshot('timeout.png')
    emu.stop()
    report = emu.logs.report()
    assert not report['faults'], report['faults']
    (emu.output/'result.json').write_text(json.dumps({
        'goal_percentages': [100,150,200,250,300,350],
        'clock_pixel_checks': ['Tokyo 24h','Tokyo 12h','UTC large'],
        'detail_timeout': True, 'report': report}, indent=2)+'\n')
    print('Mondrian improvement checks passed; clean exits and no app faults.', flush=True)
