#!/usr/bin/env python
from __future__ import print_function

from igraph import *
import re
import sys
import json
import string

MERGE_ADJACENT_LEAF = True

def main(infile, out_cppfile, out_jsonfile):
  # 無向グラフとしてLGLを読み込む wordnetとして処理済みの奴を
　# 重みは読まない
  g = Graph.Read_Lgl(infile, weights=False, directed=False)
  # コミュニティのデンドログラムを作る
  d = g.community_fastgreedy()
  # タグ名(動画数)となっているので、それを取り出す
  r = re.compile(r'(?P<name>.+)\((?P<count>\d+)\)')

  results = [None] * g.vcount()
  # set original nodes
  for elem in range(g.vcount()):
    matched = r.match(g.vs[elem]['name'])
    sc = {
      'id': elem,
      'name': matched.group('name'),
      'area': int(matched.group('count')),
      'leaf': True,
      'children': None,
    }
    results[elem] = sc

  # expand merges
  for v1, v2 in d.merges:
    # if v1 >= len(results) or v2 >= len(results): continue
    n1 = results[v1]
    n2 = results[v2]

    # if n1 is None or n2 is None: continue
    merge_node = {
      'id': len(results),
      'name': '',
      'area': n1['area'] + n2['area'],
      'leaf': False,
      'children': [v1, v2],
    }

    results.append(merge_node)

  tree = traverse_children(results, len(results) - 1)
  if MERGE_ADJACENT_LEAF:
    tree = merge_adjacent_leaf(tree)

  cstrs = []
  dump_cpp(cstrs, tree)
  cstrs.append('#define ROOT_TREE_NODE tn%d' % tree['id'])
  with open(out_cppfile, 'w') as file:
    file.write('\n'.join(cstrs))

  sys.setrecursionlimit(10000)
  with open(out_jsonfile, 'w') as file:
    json.dump(tree, file, ensure_ascii=False, indent=2)

def traverse_children(source, node_number):
  tree = {}
  node = source[node_number]
  tree['name'] = node['name']
  tree['id'] = node['id']
  if node['children'] is None:
    tree['size'] = node['area']
  else:
    children = []
    for children_node_number in node['children']:
      child = traverse_children(source, children_node_number)
      children.append(child)
    tree['children'] = children
  return tree

def merge_adjacent_leaf(node):
  leaf_nodes = []
  not_leaf_nodes = []

  if not 'children' in node:
    return node

  for child in node['children']:
    if 'children' in child:
      not_leaf_nodes.append(merge_adjacent_leaf(child))
    else:
      leaf_nodes.append(child)

  if len(not_leaf_nodes) == 1 and len(leaf_nodes) == 1:
    # 子のうち、リーフノードが1つ、非リーフノードが１つの場合
    not_leaf_nodes[0]['children'].append(leaf_nodes[0])
    # マージしたグループを作る
    node['children'] = not_leaf_nodes[0]['children']
  elif len(not_leaf_nodes) == 0 and len(leaf_nodes) > 0:
    # 全リーフの場合、グループ作成
    node['children'] = leaf_nodes

  return node

def dump_cpp(cstrs, node):
  if 'children' in node:
    # FIXME: add merged node area size
    cstrs.append('TreeNode tn%d(NULL, %f);' % (node['id'], 0))
    for child in node['children']:
      dump_cpp(cstrs, child)
      cstrs.append('tn%d.add_child(tn%d);' % (node['id'], child['id']))
  else:
    escaped_string = node['name'].replace('\\', '\\\\')
    cstrs.append('TreeNode tn%d("%s", %f);' % (node['id'],
                                               escaped_string,
                                               node['size']))

if __name__ == '__main__':
  if len(sys.argv) != 4:
    print('usage: ./wordnet_to_voronoi.py input_tag_wordnet.lgl output.cpp output.json')
    exit(1)
  main(sys.argv[1], sys.argv[2], sys.argv[3])
