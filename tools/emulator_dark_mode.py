#!/usr/bin/env python3
"""Verify Mondrian's theme through real Clay transport and watch persistence.

Run with pebble-tool's Python; the shared session restores the SDK profile.
"""
import json
from pathlib import Path
import sys
from PIL import Image
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'shared-kit/tools'))
from emulator import EmulatorSession
from emulator_smoke import common_settings

# Use the SDK display-corrected palette from deterministic renders.
DARK_PANEL = Image.open(ROOT/'shots/dark-demo.png').convert('RGB').getpixel((135,10))
DARK_GRID = Image.open(ROOT/'shots/dark-demo.png').convert('RGB').getpixel((140,10))
assert DARK_PANEL == (0,0,0) and DARK_GRID == (84,84,84)


def panel(emu, name, dark):
    path = emu.screenshot(name+'.png')
    image = Image.open(path).convert('RGB')
    expected = DARK_PANEL if dark else (255,255,255)
    assert image.getpixel((135,10)) == expected, (name,image.getpixel((135,10)))
    assert image.getpixel((140,10)) == (DARK_GRID if dark else (0,0,0)), name
    return path


with EmulatorSession(ROOT, ROOT/'shots/emulator-dark-mode') as emu:
    emu.install()
    emu.start()
    panel(emu, 'fresh-default', False)
    values = dict(common_settings(18000), DARK_MODE=True, WORLD=289)
    emu.apply_clay(values)
    panel(emu, 'dark-face', True)
    mark = emu.restart()
    emu.logs.wait('Kit settings loaded: goal=18000', since=mark)
    panel(emu, 'dark-restarted', True)
    # Malformed or absent new settings cannot clobber the saved theme.
    for value in [2, -1, 'invalid']:
        emu.apply_settings({'DARK_MODE':value})
        panel(emu, 'dark-invalid-'+str(value), True)
    emu.apply_settings({'READY':1})
    panel(emu, 'dark-no-theme-message', True)
    values['LARGE_LABELS'] = True
    emu.apply_clay(values)
    panel(emu, 'dark-large', True)
    mark = emu.logs.mark()
    emu.cli('emu-tap')
    emu.logs.wait('Frame detail', since=mark)
    image = Image.open(emu.screenshot('dark-detail.png')).convert('RGB')
    assert image.getpixel((65,10)) == DARK_PANEL
    assert image.getpixel((73,10)) == DARK_GRID
    mark = emu.logs.mark()
    emu.logs.wait('Frame face', since=mark, timeout=12)
    panel(emu, 'dark-timeout', True)
    values['DARK_MODE'] = False
    emu.apply_clay(values)
    panel(emu, 'light-restored', False)
    mark = emu.restart()
    emu.logs.wait('Kit settings loaded: goal=18000', since=mark)
    panel(emu, 'light-restarted', False)
    emu.stop()
    report = emu.logs.report()
    assert not report['faults'], report['faults']
    (emu.output/'result.json').write_text(json.dumps({
        'fresh_light_default':True, 'clay_toggle_both_directions':True,
        'restart_persistence_both_themes':True, 'invalid_and_missing_theme_preserved':True,
        'saved_goal_preserved':18000, 'large_labels_and_detail':True,
        'detail_timeout':True, 'report':report}, indent=2)+'\n')
    print('Dark mode Clay, restart, validation, detail and clean-exit checks passed.')
