#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import math
import Image, ImageDraw, ImageFont

print sys.argv[1]

img = Image.open(sys.argv[1])
width, height = img.size
cols = int(math.ceil(width / 256.0))
rows = int(math.ceil(height / 256.0))

for y in range(rows):
  for x in range(cols):
    tile = img.crop((x * 256, y * 256, (x + 1) * 256, (y + 1) * 256))
    tile.save('imgimg_%d_%d.png' % (x, y))
