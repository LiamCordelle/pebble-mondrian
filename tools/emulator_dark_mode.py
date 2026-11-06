#!/usr/bin/env python3
"""Verify Mondrian's theme through real Clay transport and watch persistence.

Run with pebble-tool's Python; the shared session restores the SDK profile.
"""
import json
import subprocess
import time
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


def panel(emu, name, dark, white=False):
    path = emu.screenshot(name+'.png')
    image = Image.open(path).convert('RGB')
    expected = DARK_PANEL if dark else (255,255,255)
    assert image.getpixel((135,10)) == expected, (name,image.getpixel((135,10)))
    assert image.getpixel((140,10)) == ((255,255,255) if dark and white else DARK_GRID if dark else (0,0,0)), name
    return path


def boundaries(phase):
    with EmulatorSession(ROOT, ROOT/'shots/emulator-appearance'/phase) as emu:
        emu.install();emu.start()
        emu.apply_clay(dict(common_settings(18000), DARK_MODE=2, DARK_GRID=1))
        panel(emu,'auto-weather-disabled',False)
        subprocess.run(['node',str(ROOT/'tools/appearance_weather_fixture.js'),str(emu.output),phase],check=True)
        scenario=json.loads((emu.output/'boundary.json').read_text())
        emu.weather_reply_once((emu.output/'boundary-weather.bin').read_bytes())
        # Keep the phone's weather disabled; use only the correlated synthetic reply.
        mark=emu.logs.mark()
        emu.apply_settings(dict(common_settings(18000,weather=True),DARK_MODE=2,DARK_GRID=1))
        emu.logs.wait('Weather snapshot accepted',since=mark)
        before_dark=phase=='sunrise'
        panel(emu,'before-'+phase,before_dark,True)
        mark=emu.logs.mark()
        while time.time()<scenario['boundary']+1:
            time.sleep(min(10,max(.1,scenario['boundary']+1-time.time())))
        emu.logs.wait('Frame face',since=mark,timeout=10)
        panel(emu,'after-'+phase,not before_dark,True)
        image=Image.open(emu.output/('after-'+phase+'.png')).convert('RGB')
        expected=(0,0,0) if phase=='sunset' else Image.open(ROOT/'shots/demo.png').convert('RGB').getpixel((74,195))
        assert image.getpixel((74,195))==expected, ('sun cell',phase)
        emu.apply_settings({'WEATHER_ENABLED':0, **{k:v for k,v in common_settings(18000).items() if k!='WEATHER_ENABLED'}})
        panel(emu,'fallback-weather-disabled',False)
        emu.stop();report=emu.logs.report();assert not report['faults'],report['faults']
        (emu.output/'result.json').write_text(json.dumps({'phase':phase,'boundary':scenario['boundary'],
            'automatic_minute_transition':True,'sun_cell_matches_theme':True,'weather_disabled_fallback':True,
            'report':report},indent=2)+'\n')
        print('Verified automatic '+phase+' on the live minute tick.',flush=True)


with EmulatorSession(ROOT, ROOT/'shots/emulator-dark-mode') as emu:
    old=Path('/tmp/mondrian-1.2.0-upgrade.pbw')
    legacy_upgrade=old.exists()
    if legacy_upgrade:
        emu.install(old);emu.start()
        emu.apply_clay(dict(common_settings(18000),DARK_MODE=True))
        panel(emu,'legacy-dark',True)
        emu.stop();emu.install();emu.start()
        panel(emu,'legacy-upgraded',True)
    else:
        emu.install();emu.start()
        panel(emu,'fresh-default',False)
    values=dict(common_settings(18000),DARK_MODE=1,DARK_GRID=0,WORLD=289)
    emu.apply_clay(values);panel(emu,'dark-face',True)
    mark=emu.restart();emu.logs.wait('Kit settings loaded: goal=18000',since=mark)
    panel(emu,'dark-restarted',True)
    for key,value in [('DARK_MODE',3),('DARK_MODE',-1),('DARK_MODE','invalid'),('DARK_GRID',2)]:
        emu.apply_settings({key:value});panel(emu,'invalid-'+key+'-'+str(value),True)
    emu.apply_settings({'READY':1});panel(emu,'dark-no-theme-message',True)
    values.update(DARK_GRID=1,LARGE_LABELS=True)
    emu.apply_clay(values);panel(emu,'white-grid-large',True,True)
    mark=emu.restart();emu.logs.wait('Kit settings loaded: goal=18000',since=mark)
    panel(emu,'white-grid-restarted',True,True)
    mark=emu.logs.mark();emu.cli('emu-tap');emu.logs.wait('Frame detail',since=mark)
    image=Image.open(emu.screenshot('white-grid-detail.png')).convert('RGB')
    assert image.getpixel((65,10))==DARK_PANEL and image.getpixel((73,10))==(255,255,255)
    mark=emu.logs.mark();emu.logs.wait('Frame face',since=mark,timeout=12)
    panel(emu,'dark-timeout',True,True)
    values['DARK_MODE']=2
    emu.apply_clay(values);panel(emu,'auto-disabled-fallback',False)
    mark=emu.restart();emu.logs.wait('Kit settings loaded: goal=18000',since=mark)
    panel(emu,'auto-disabled-restarted',False)
    values['DARK_MODE']=0
    emu.apply_clay(values);panel(emu,'light-restored',False)
    mark=emu.restart();emu.logs.wait('Kit settings loaded: goal=18000',since=mark)
    panel(emu,'light-restarted',False)
    emu.stop();report=emu.logs.report();assert not report['faults'],report['faults']
    (emu.output/'result.json').write_text(json.dumps({'legacy_1_2_upgrade':legacy_upgrade,
        'clay_all_appearance_modes':True,'restart_persistence_all_modes':True,'white_grid_and_detail':True,
        'invalid_and_missing_theme_preserved':True,'saved_goal_preserved':18000,
        'detail_timeout':True,'report':report},indent=2)+'\n')
    print('Appearance upgrade, Clay, persistence, validation, detail and clean-exit checks passed.',flush=True)

for phase in ['sunset','sunrise']:boundaries(phase)
