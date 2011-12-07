#!/usr/bin/python

import igraph
import re
import sys

CUT = 100

def main(infile):
  g = igraph.read(infile, 'lgl')
  d = g.community_fastgreedy()
  mem = d.cut(CUT)
  categories = [[] for i in xrange(CUT)]
  r = re.compile(r'(?P<name>.+)\((?P<count>\d+)\)')
  for i, cat_no in enumerate(mem):
    m = r.match(g.vs[i]['name'])
    categories[cat_no].append((int(m.group('count')), m.group('name')))
  for i, cat in enumerate(categories):
    print '** category %d **' % i
    cat.sort(lambda x, y: -cmp(x, y))
    for c in cat:
      print "%s : %d" % (c[1], c[0])

if __name__ == '__main__':
  if len(sys.argv) != 2:
    print 'usage: ./wordcatcut.py input_tag_wordnet.lgl'
    exit(1)
  main(sys.argv[1])
