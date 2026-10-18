#!/usr/bin/env python3
"""Make static store icons from the approved demo render, preserving its artwork."""
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
image=Image.open(ROOT/'shots/demo.png').convert('RGB')
for size in (80,144):
    image.resize((size,size),Image.Resampling.LANCZOS).save(ROOT/f'icon_{size}x{size}.png')
print('Created 80×80 and 144×144 store icons from the original composition.')
