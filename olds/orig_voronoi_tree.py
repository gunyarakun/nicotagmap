#!/usr/bin/env python
# -*- coding: utf-8 -*-

DEBUG = False

import sys
import random
from math import *

class TreeNode:
  def __init__(self, name, area, *children):
    self.area = area
    self.name = name

    self.x = None    # voronoi point x
    self.y = None    # voronoi point y
    self.cx = None   # centroid of region
    self.cy = None   # centroid of region
    self.region = [] # point tuple list in region

    self.children = children
  def euc_dist(self, obj):
    dx = self.x - obj[0]
    dy = self.y - obj[1]
    return sqrt(dx * dx + dy * dy)
  def pw_dist(self, obj, weight):
    dx = self.x - obj[0]
    dy = self.y - obj[1]
    return dx * dx + dy * dy - weight
  def clear_region(self):
    self.region = []
  def add_region_point(self, point):
    self.region.append(point)
  def region_size(self):
    return len(self.region)
  def calc_centroid(self):
    cx = 0.0
    cy = 0.0
    for p in self.region:
      cx += p[0]
      cy += p[1]
    cx /= len(self.region)
    cy /= len(self.region)
    self.cx = cx
    self.cy = cy
  def move_to_centroid(self):
    if not self.cx or not self.cy:
      self.calc_centroid()
    self.x = self.cx
    self.y = self.cy
    self.cx = None
    self.cy = None
  def region_draw(self, im, draw, font):
    if self.children:
      for c in self.children:
        c.region_draw(im, draw, font)
    else:
      color = (random.randint(0,255), random.randint(0,255), random.randint(0,255))
      for p in self.region:
        im.putpixel((p[0], p[1]), color)
    if not self.cx or not self.cy:
      self.calc_centroid()
    draw.text((self.cx, self.cy), self.name, font = font, fill = 0)

  def nchildren(self):
    return len(self.children)
  def __getitem__(self, key):
    return self.children[key]
  def __iter__(self):
    return self.children.__iter__()
  def next(self):
    return self.children.next()
  def str_r(self, indent):
    s = indent + '(x: %s, y: %s, area: %d, region_size: %d)\n' % (self.x, self.y, self.area, self.region_size())
    for c in self.children:
      s += c.str_r(indent + '  ')
    return s
  def __str__(self):
    return self.str_r('')

# NOTE: p2 must be prime number
def set_vpoint_halton(width, height, tree_nodes, p2 = 3):
  n = tree_nodes.nchildren()
  in_region = 0
  k = 0
  while in_region < n:
    u = 0
    p = 0.5
    kk = k
    while kk > 0:
      if kk & 1:
        u += p
      p *= 0.5
      kk >>= 1
    v = 0
    ip = 1.0 / p2
    p = ip
    kk = k
    while kk > 0:
      a = kk % p2
      if a != 0:
        v += a*p
      p *= ip
      kk = int(kk/p2)
    x = int(u * width)
    y = int(v * height)
    if (x, y) in tree_nodes.region:
      tree_nodes[k].x = x
      tree_nodes[k].y = y
      in_region += 1
    k += 1

def calc_voronoi_tessellation(tree_nodes, weights):
  for tn in tree_nodes:
    tn.clear_region()
  for cp in tree_nodes.region:
    min_index = min([(p[1].pw_dist(cp, weights[p[0]]), p[0]) for p in enumerate(tree_nodes)])[1]
    tree_nodes[min_index].add_region_point(cp)

EPSILON = 0.1

def adjust_weight_pw(weights, size_diffs, adesired):
  return map(lambda i: max(1, weights[i] * (1.0 + size_diffs[i] / adesired[i])), xrange(len(weights)))

def move_generators_pw(tree_nodes):
  map(lambda tn: tn.move_to_centroid(), tree_nodes)

def pp(v1, v2):
  print "%d + %d" % (v1, v2)
  return 0

def calc_voronoi_treemap(width, height, tree_nodes):
  set_vpoint_halton(width, height, tree_nodes) # FIXME: in region!
  all_areas = reduce(lambda a, b: a + b.area, tree_nodes, 0)
  adesired = map(lambda a: float(a.area) / all_areas, tree_nodes)
  weights = [1.0] * tree_nodes.nchildren();

  results = []
  while True:
    calc_voronoi_tessellation(tree_nodes, weights)

    all_area_size = reduce(lambda a, b: a + b, map((lambda p: p.region_size()), tree_nodes))
    size_diffs = map(lambda p: adesired[p[0]] - float(p[1].region_size()) / all_area_size, enumerate(tree_nodes))
    if not filter(lambda d: abs(d) > EPSILON, size_diffs):
      break
    weights = adjust_weight_pw(weights, size_diffs, adesired)
    move_generators_pw(tree_nodes)

    if DEBUG:
      print tree_nodes
      print size_diffs
      print weights

  for tn in tree_nodes:
    if tn.nchildren() > 1:
      calc_voronoi_treemap(width, height, tn)

  return results

def voronoi_treemap(width, height, world, tree_nodes):
  tree_nodes.region = world
  return calc_voronoi_treemap(width, height, tree_nodes)

width = 300
height = 300

# treeの面積。第１引数は自分の名前。第２引数は自分の面積。第３引数以降は子のTreeNodeたち。
# 子の面積の和と、親の面積が一致していなくてもかまわない。
# 同じ階層での面積比しか計算に利用しない。
tree_nodes = TreeNode(u'全体', 0,
  TreeNode(u'でかい', 100,
    TreeNode(u'でかちいさい', 10),
    TreeNode(u'でか中', 30)),
  TreeNode(u'ちいさい', 20),
  TreeNode(u'中', 30),
)

# 世界にある全てのpixelのtuple。任意の形でOKだが、連結してないと変な結果になりそう。
world = []
for x in xrange(width):
  for y in xrange(height):
    world.append((x, y))

import Image, ImageDraw, ImageFont
im = Image.new('RGB', (width, height))
draw = ImageDraw.Draw(im)
if sys.platform == 'win32':
  font = ImageFont.truetype('C:\\Windows\\Fonts\\msgothic.ttc', 24, index=0, encoding='unic')
else:
  font = ImageFont.load_default()
voronoi_treemap(width, height, world, tree_nodes)
print tree_nodes
for p in tree_nodes:
  p.region_draw(im, draw, font)
im.show()
