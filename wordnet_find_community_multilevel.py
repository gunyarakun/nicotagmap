#!/usr/bin/env python
from __future__ import print_function

from igraph import *
import re
import sys
import json
import string

def find_community_multilevel(graph):
  # タグ名(動画数)となっているので、それを取り出す
  r = re.compile(r'(?P<name>.+)\((?P<count>\d+)\)')

  cm = graph.community_multilevel(weights='weight', return_levels=True)
  root_node = {'name': '', 'children': []}
  children = []
  for node_ids in cm[-1]:
    # cm[-1]はVertexClusteringのインスタンス
    children = []
    for node_id in node_ids:
      node = graph.vs[node_id]
      matched = r.match(node['name'])
      children.append({
        'name': matched.group('name'),
        'size': int(matched.group('count')),
      })
    root_node['children'].append({
      'name': '',
      'children': children,
    })
  return root_node

def main(infile, out_jsonfile):
  # 無向グラフとしてLGLを読み込む wordnetとして処理済みの奴を
  g = Graph.Read_Lgl(infile, directed=False)
  tree = find_community_multilevel(g)

  with open(out_jsonfile, 'w') as file:
    json.dump(tree, file, ensure_ascii=False, indent=2)

if __name__ == '__main__':
  if len(sys.argv) != 3:
    print('usage: %s input_tag_wordnet.lgl output.json' % sys.argv[0])
    exit(1)
  main(sys.argv[1], sys.argv[2])
