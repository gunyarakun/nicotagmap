#!/usr/bin/python
import igraph
import re
import sys
import math
import string

MERGE_ADJACENT_LEAF = False
LOG_AREA = False
CONST_AREA = False

def main(infile):
  g = igraph.read(infile, 'lgl')
  d = g.community_fastgreedy()
  r = re.compile(r'(?P<name>.+)\((?P<count>\d+)\)')

  results = [None] * d._n
  inorder = d._traverse_inorder()
  cstrs = []
  for elem in range(d._n):
    matched = r.match(g.vs[elem]['name'])
    sc = {
      'area': int(matched.group('count')),
      'leaf': True,
      'children': [elem],
    }
    if LOG_AREA:
      sc['area'] = math.log(sc['area'], 1.1)
    if CONST_AREA:
      sc['area'] = CONST_AREA

    escaped_string = string.replace(matched.group('name'), '\\', '\\\\')
    # FIXME: escape name string!!!
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
    sc = {
      'area': n1['area'] + n2['area'],
      'leaf': False,
      'children': [],
    }

    if MERGE_ADJACENT_LEAF:
      if n1['leaf'] and n2['leaf']:
        sc['children'].append(v1)
        sc['children'].append(v2)
      elif n1['leaf'] and not n2['leaf']:
        sc['children'].append(v1)
        sc['children'].extend(n2['children'])
      elif not n1['leaf'] and n2['leaf']:
        sc['children'].extend(n1['children'])
        sc['children'].append(v2)
      else:
        sc['children'].append(nn)
        cstrs.append('TreeNode tn%d(NULL, %f);' % (nn, sc['area']))
        for idx in n1['children']:
          cstrs.append('tn%d.add_child(tn%d);' % (nn, idx))
        for idx in n2['children']:
          cstrs.append('tn%d.add_child(tn%d);' % (nn, idx))
    else:
      cstrs.append('TreeNode tn%d(NULL, %f);' % (nn, sc['area']))
      cstrs.append('tn%d.add_child(tn%d);' % (nn, v1))
      cstrs.append('tn%d.add_child(tn%d);' % (nn, v2))
    results.append(sc)

  cstrs.append('#define ROOT_TREE_NODE tn%d' % nn)
  print '\n'.join(cstrs)

if __name__ == '__main__':
  if len(sys.argv) != 2:
    print 'usage: ./lgl2xgmml.py input_tag_wordnet.lgl'
    exit(1)
  main(sys.argv[1])
