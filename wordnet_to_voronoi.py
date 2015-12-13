#!/usr/bin/env python
from __future__ import print_function

from igraph import *
import re
import sys
import math
import string

MERGE_ADJACENT_LEAF = True

def main(infile):
  # 無向グラフとしてLGLを読み込む
  g = Graph.Read_Lgl(infile, directed=False)
  # コミュニティのデンドログラムを作る
  d = g.community_fastgreedy()
  # タグ名(動画数)となっているので、それを取り出す
  r = re.compile(r'(?P<name>.+)\((?P<count>\d+)\)')

  results = [None] * g.vcount()
  cstrs = []
  for elem in range(g.vcount()):
    matched = r.match(g.vs[elem]['name'])
    sc = {
      'area': int(matched.group('count')),
      'leaf': True,
      'children': [elem],
    }

    escaped_string = matched.group('name').replace('\\', '\\\\')
    cstrs.append('TreeNode tn%d("%s", %f);' % (elem,
                                               escaped_string,
                                               sc['area']))
    results[elem] = sc

  for v1, v2 in d.merges:
    # if v1 >= len(results) or v2 >= len(results): continue
    nn = len(results)
    n1 = results[v1]
    n2 = results[v2]

    # if n1 is None or n2 is None: continue
    merge_node = {
      'area': n1['area'] + n2['area'],
      'leaf': False,
      'children': [v1, v2],
    }

    if MERGE_ADJACENT_LEAF:
      if n1['leaf'] and n2['leaf']:
        merge_node['children'].append(v1)
        merge_node['children'].append(v2)
      elif n1['leaf'] and not n2['leaf']:
        merge_node['children'].append(v1)
        merge_node['children'].extend(n2['children'])
      elif not n1['leaf'] and n2['leaf']:
        merge_node['children'].extend(n1['children'])
        merge_node['children'].append(v2)
      else:
        print("kitaktak")
        merge_node['children'].append(nn)
        cstrs.append('TreeNode tn%d(NULL, %f);' % (nn, merge_node['area']))
        for idx in n1['children']:
          cstrs.append('tn%d.add_child(tn%d);' % (nn, idx))
        for idx in n2['children']:
          cstrs.append('tn%d.add_child(tn%d);' % (nn, idx))
    else:
      cstrs.append('TreeNode tn%d(NULL, %f);' % (nn, merge_node['area']))
      cstrs.append('tn%d.add_child(tn%d);' % (nn, v1))
      cstrs.append('tn%d.add_child(tn%d);' % (nn, v2))

    results.append(merge_node)

  cstrs.append('#define ROOT_TREE_NODE tn%d' % nn)
  print('\n'.join(cstrs))

if __name__ == '__main__':
  if len(sys.argv) != 2:
    print('usage: ./wordnet_to_voronoi.py input_tag_wordnet.lgl')
    exit(1)
  main(sys.argv[1])
