#!/usr/bin/env python3
"""Generate the native 25x25 launcher icon. Requires Pillow; no antialiasing."""
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]


def create_icon():
    """A De Stijl grid echoing the face's asymmetric planes."""
    # Emery-native primary colours with opaque planes and crisp black dividers.
    black, white = '#000000', '#FFFFFF'
    red, blue, yellow = '#FF0000', '#0055FF', '#FFFF00'
    image = Image.new('RGBA', (25, 25), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    draw.rectangle((1, 1, 23, 23), fill=white, outline=black, width=2)
    draw.rectangle((18, 3, 21, 11), fill=red)
    draw.rectangle((3, 15, 7, 21), fill=yellow)
    draw.rectangle((11, 15, 21, 21), fill=blue)
    draw.rectangle((15, 2, 17, 13), fill=black)
    draw.rectangle((2, 12, 22, 14), fill=black)
    draw.rectangle((8, 14, 10, 22), fill=black)
    draw.rectangle((17, 6, 22, 7), fill=black)
    draw.rectangle((2, 18, 7, 19), fill=black)
    return image


if __name__ == '__main__':
    output = ROOT / 'resources/images/menu_icon.png'
    output.parent.mkdir(parents=True, exist_ok=True)
    create_icon().save(output, optimize=True)
    print(output)
