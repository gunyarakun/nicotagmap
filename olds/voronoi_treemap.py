#!/usr/bin/env python
# -*- coding: utf-8 -*-

from vorotree import TreeNode
import sys

DEBUG = False

# NOTE: p2 must be prime number
# TODO: width / height set
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
    # check and add
    in_region += tree_nodes.child_move_to(in_region, x, y)
    k += 1

EPSILON = 0.1

def adjust_weight_pw(weights, size_diffs, adesired):
  return map(lambda i: max(1, weights[i] * (1.0 + size_diffs[i] / adesired[i])), xrange(len(weights)))

def move_generators_pw(tree_nodes):
  for n in tree_nodes:
    n.move_to_centroid()

def pp(v1, v2):
  print "%d + %d" % (v1, v2)
  return 0

def calc_voronoi_treemap(width, height, tree_nodes):
  set_vpoint_halton(width, height, tree_nodes) # FIXME: in region!
  adesired = tree_nodes.child_adesired()
  weights = [1.0] * tree_nodes.nchildren();

  results = []
  while True:
    tree_nodes.calc_voronoi_tessellation(weights)

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

  def main():
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

    for x in xrange(width):
      for y in xrange(height):
        tree_nodes.add_region_point(x, y)

    import Image, ImageDraw, ImageFont
    im = Image.new('RGB', (width, height))
    draw = ImageDraw.Draw(im)
    if sys.platform == 'win32':
      font = ImageFont.truetype('C:\\Windows\\Fonts\\msgothic.ttc', 24, index=0, encoding='unic')
    else:
      font = None
    calc_voronoi_treemap(width, height, tree_nodes)
    print tree_nodes
    for p in tree_nodes:
      p.region_draw(im, draw, font)
    if sys.platform == 'win32':
      im.show()
    else:
      im.save('out.png')

if __name__ == '__main__':
  main()
