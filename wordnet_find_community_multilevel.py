#!/usr/bin/env python
from __future__ import print_function

from igraph import *
import re
import sys
import json
import random
import string

RECURSIVE_CLUSTER = True
USE_LAST_LEVEL_OF_COMMUNITY = False

def find_community_multilevel(graph):
  # タグ名(動画数)となっているので、それを取り出す
  r = re.compile(r'(?P<name>.+)\((?P<count>\d+)\)')

  cm = graph.community_multilevel(weights='weight', return_levels=True)
  root_node = {'name': '', 'children': [], 'color': random.randint(0, 19)}
  children = []
  # cmは添え字が進めば進むほど学習が進んでる
  # 再帰的に適用するため、クラスタを細かくしすぎないように: cm[0]
  # とにかく各段階で学習を進める: cm[-1]
  cm_index = -1 if USE_LAST_LEVEL_OF_COMMUNITY else 0
  for node_ids in cm[cm_index]:
    if len(node_ids) >= 40 and RECURSIVE_CLUSTER:
      subgraph = graph.subgraph(node_ids)
      subcom = find_community_multilevel(subgraph)
      children = subcom['children']
    else:
      children = []
      for node_id in node_ids:
        node = graph.vs[node_id]
        matched = r.match(node['name'])
        children.append({
          'name': matched.group('name'),
          'size': int(matched.group('count')),
          'color': random.randint(0, 19)
        })
    root_node['children'].append({
      'name': '',
      'children': children,
      'color': random.randint(0, 19)
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
