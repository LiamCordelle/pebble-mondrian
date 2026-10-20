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
for name,code in [('sun',0),('partly',2),('cloud',3),('rain',63),('snow',73),('storm',95),('fog',45)]:
    scenes.append(('weather-'+name,0,False,{'WEATHER_CODE':code}))
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
cols=6;rows=(len(scenes)+cols-1)//cols
sheet=Image.new('RGB',(cols*216,rows*254),'#dedede');draw=ImageDraw.Draw(sheet)
for i,(name,*_) in enumerate(scenes):
    x=(i%cols)*216+8;y=(i//cols)*254
    draw.text((x,y+4),name,fill='black')
    sheet.paste(Image.open(SHOTS/(name+'.png')).convert('RGB'),(x,y+22))
sheet.save(SHOTS/'coverage.png')
(SHOTS/'coverage.json').write_text(json.dumps({'scenes':[s[0] for s in scenes],
  'default_demo_pixel_identical':True},indent=2)+'\n')
print(f'Rendered {len(scenes)} scenes; default demo matches original exactly.')
