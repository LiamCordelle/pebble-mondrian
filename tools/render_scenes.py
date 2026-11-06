#!/usr/bin/env python3
"""Lab-compatible deterministic visual regression scenes; never starts QEMU."""
import concurrent.futures
import json
import os
from pathlib import Path
import subprocess
from PIL import Image,ImageChops,ImageDraw

ROOT=Path(__file__).resolve().parents[1]
LAB=Path(os.environ.get('WATCHFACE_LAB',ROOT.parent/'watchface-lab'))
SHOTS=ROOT/'shots'
scenes=[('demo',0,False,{}),('stress',1,False,{}),('empty',2,False,{}),
        ('detail',0,True,{}),('detail-stress',1,True,{}),('detail-empty',2,True,{}),
        ('12h',0,False,{'12H':1,'HOUR':9,'MINUTE':5}),
        ('midnight',0,False,{'HOUR':0,'MINUTE':8}),
        ('after-sunset',0,False,{'HOUR':21,'MINUTE':15}),
        ('no-phone',0,False,{'PHONE':0}),('no-health',0,False,{'NO_HEALTH':1}),
        ('low-battery',0,False,{'BATTERY':28}),('critical-battery',0,False,{'BATTERY':5}),
        ('charging',0,False,{'BATTERY':62,'CHARGING':1}),
        ('fahrenheit',1,False,{'FAHRENHEIT':1}),
        ('narrow-heart',0,True,{'HR_MIN':55,'HR_MAX':70}),
        ('mid-heart',0,True,{'HR_MIN':90,'HR_MAX':130}),
        ('large-labels',0,False,{'LARGE_LABELS':1}),
        ('large-detail',0,True,{'LARGE_LABELS':1}),
        ('large-stress',1,False,{'LARGE_LABELS':1,'FAHRENHEIT':1,'UV':120}),
        ('large-detail-stress',1,True,{'LARGE_LABELS':1,'FAHRENHEIT':1}),
        ('large-empty',2,False,{'LARGE_LABELS':1}),
        ('partial-heart',0,True,{'HR_PARTIAL':1}),
        ('partial-heart-large',0,True,{'HR_PARTIAL':1,'LARGE_LABELS':1,'HR_MIN':90,'HR_MAX':130}),
        ('no-range',0,True,{'NO_RANGE':1}),
        ('partial-no-range',0,True,{'NO_RANGE':1,'HR_PARTIAL':1}),
        ('heart-220',0,False,{'HR':220}),('step-sliver',0,False,{'STEPS':100}),
        ('near-goal',0,False,{'STEPS':9870})]
scenes.append(('unknown-daylight',0,False,{'NO_SUN':1}))
for pct in [0,50,100,101,125,150,175,199,200,201,250,299,300,301,350,400]:
    for large in [False,True]:
        scenes.append((f'goal-{pct}'+('-large' if large else ''),0,False,
                       {'STEPS':pct*80,'STEP_GOAL':8000,'LARGE_LABELS':int(large)}))
for name,values in [
    ('world-nz',{}),('world-utc',{'WORLD':1}),('world-nyc',{'WORLD':3}),
    ('world-12h',{'12H':1}),('world-midnight',{'WORLD':1,'HOUR':1,'MINUTE':0}),
    ('world-large',{'LARGE_LABELS':1}),('world-disconnected',{'PHONE':0})]:
    scenes.append((name,0,True,values))
for name,zone in [('mumbai',289),('abu-dhabi',268),('moscow',463),
                  ('kathmandu',286),('chatham',533),('kiritimati',548)]:
    for detail in [False,True]:
        for large in [False,True]:
            scenes.append(('world-'+name+('-detail' if detail else '')+('-large' if large else ''),
                           0,detail,{'WORLD':zone,'LARGE_LABELS':int(large)}))
for name,code in [('sun',0),('partly',2),('cloud',3),('rain',63),('snow',73),('storm',95),('fog',45)]:
    scenes.append(('weather-'+name,0,False,{'WEATHER_CODE':code}))
# Exercise each palette through the original visual edge cases.
base_scenes=list(scenes)
scenes += [('dark-'+name,scenario,detail,dict(values,DARK_MODE=1))
           for name,scenario,detail,values in base_scenes]
scenes += [('white-grid-'+name,scenario,detail,dict(values,DARK_MODE=1,DARK_GRID=1))
           for name,scenario,detail,values in base_scenes]
auto_checks=[]
for name,hour,minute,dark,extra in [
    ('before-rise',7,28,True,{}),('at-rise',7,29,False,{}),('after-rise',7,30,False,{}),
    ('before-set',18,2,False,{}),('at-set',18,3,True,{}),('after-set',18,4,True,{}),
    ('midnight',0,0,True,{}),('noon',12,0,False,{}),
    ('unknown-sun',21,0,False,{'NO_SUN':1}),
    ('disabled-weather',21,0,False,{'WEATHER_DISABLED':1})]:
    for grid in [0,1]:
        for detail in [False,True]:
            suffix=('-white' if grid else '-grey')+('-detail' if detail else '')
            values=dict(extra,HOUR=hour,MINUTE=minute,DARK_GRID=grid)
            automatic='auto-'+name+suffix;manual='manual-'+name+suffix
            scenes.append((automatic,0,detail,dict(values,DARK_MODE=2)))
            scenes.append((manual,0,detail,dict(values,DARK_MODE=int(dark))))
            auto_checks.append((automatic,manual))
scenes += [('light-white-grid',0,False,{'DARK_GRID':1}),
           ('light-white-grid-detail',0,True,{'DARK_GRID':1})]
def render(scene):
    name,scenario,detail,values=scene
    env=os.environ.copy()
    for k,v in values.items():env['MONDRIAN_'+k]=str(v)
    args=['python3',str(LAB/'tools/render.py'),str(ROOT),str(SHOTS/(name+'.png')),'--scenario',str(scenario)]
    if detail:args.append('--detail')
    result=subprocess.run(args,env=env,capture_output=True,text=True)
    if result.returncode:raise RuntimeError(result.stdout+result.stderr)
    return name
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
    list(pool.map(render,scenes))
for name in ['demo','detail','large-labels','large-detail']:
    subprocess.run(['python3',str(LAB/'tools/glance.py'),str(SHOTS/(name+'.png'))],check=True,capture_output=True)
reference=Image.open(LAB/'designs/05-mondrian/shots/demo.png').convert('RGB')
actual=Image.open(SHOTS/'demo.png').convert('RGB')
assert ImageChops.difference(reference,actual).getbbox() is None,'Default art changed from original'
assert Image.open(SHOTS/'unknown-daylight.png').convert('RGB').getpixel((74,195))==(255,255,255), 'Unknown daylight must retain a neutral sun cell'
# The Lab renderer applies its display palette; sample the existing planes.
white=actual.getpixel((194,10))
blue=actual.getpixel((194,225))
yellow=actual.getpixel((74,195))
green=(142,227,145)  # SDK LCD correction of GColorGreen (00ff00).
for pct,top,bottom in [(0,white,white),(50,white,blue),(100,blue,blue),
                       (150,blue,yellow),(200,yellow,yellow),(250,yellow,green),
                       (300,green,green),(350,green,blue),(400,blue,blue)]:
    for suffix in ['', '-large']:
        image=Image.open(SHOTS/f'goal-{pct}{suffix}.png').convert('RGB')
        assert image.getpixel((194,111))==top,(pct,suffix,'previous colour')
        assert image.getpixel((194,225))==bottom,(pct,suffix,'current colour')
# The detail clock must change when the world setting changes, including 12h.
def clock(name):return Image.open(SHOTS/(name+'.png')).crop((0,0,72,56)).convert('RGB')
assert ImageChops.difference(clock('world-nz'),clock('world-utc')).getbbox()
assert ImageChops.difference(clock('world-nz'),clock('world-12h')).getbbox()
assert ImageChops.difference(clock('world-nz'),clock('world-disconnected')).getbbox() is None
# Sample black neutral panels, theme-specific rules and unchanged coloured planes.
dark=Image.open(SHOTS/'dark-demo.png').convert('RGB')
black=dark.getpixel((194,10))
assert black == (0,0,0)
for point in [(135,10),(194,10),(74,110)]:
    assert dark.getpixel(point)==black, ('dark neutral panel',point)
for point in [(140,10),(10,104),(82,110)]:
    assert actual.getpixel(point)==(0,0,0), ('light black grid',point)
    assert dark.getpixel(point)==(84,84,84), ('dark grey grid',point)
for point in [(194,52),(74,195),(194,225)]:
    assert dark.getpixel(point)==actual.getpixel(point), ('primary colour',point)
for name in ['demo','detail','empty','detail-empty','large-labels','large-detail']:
    light=Image.open(SHOTS/(name+'.png')).convert('RGB')
    night=Image.open(SHOTS/('dark-'+name+'.png')).convert('RGB')
    assert ImageChops.difference(light,night).getbbox(), name
    # Pure colours retain their locations; only neutral surfaces/ink change.
    for colour in [blue,yellow,actual.getpixel((194,52)),green]:
        assert bytes(c==colour for c in light.get_flattened_data()) == bytes(c==colour for c in night.get_flattened_data()), (name,colour)
for point in [(140,10),(10,104),(82,110)]:
    assert Image.open(SHOTS/'white-grid-demo.png').convert('RGB').getpixel(point)==white
for automatic,manual in auto_checks:
    left=Image.open(SHOTS/(automatic+'.png')).convert('RGB')
    right=Image.open(SHOTS/(manual+'.png')).convert('RGB')
    assert ImageChops.difference(left,right).getbbox() is None, automatic
for optional,original in [('light-white-grid','demo'),('light-white-grid-detail','detail')]:
    assert ImageChops.difference(Image.open(SHOTS/(optional+'.png')),Image.open(SHOTS/(original+'.png'))).getbbox() is None
cols=6;rows=(len(scenes)+cols-1)//cols
sheet=Image.new('RGB',(cols*216,rows*254),'#dedede');draw=ImageDraw.Draw(sheet)
for i,(name,*_) in enumerate(scenes):
    x=(i%cols)*216+8;y=(i//cols)*254
    draw.text((x,y+4),name,fill='black')
    sheet.paste(Image.open(SHOTS/(name+'.png')).convert('RGB'),(x,y+22))
sheet.save(SHOTS/'coverage.png')
(SHOTS/'coverage.json').write_text(json.dumps({'scenes':[s[0] for s in scenes],
  'default_demo_pixel_identical':True,'goal_colour_checks':True,'world_clock_checks':True,'dark_palette_checks':True,'white_grid_checks':True,'automatic_full_frame_checks':len(auto_checks)},indent=2)+'\n')
print(f'Rendered {len(scenes)} scenes; default demo matches original exactly.')
