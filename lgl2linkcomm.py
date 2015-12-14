#!/usr/bin/env python

import sys
from igraph import *

PRECISION = 12

def main(infile, outfile):
  g = Graph.Read_Lgl(infile, directed=False)

  max_weight = 0
  for e in g.es:
    if max_weight < e['weight']:
      max_weight = e['weight']

  with open(outfile, 'w') as f:
    for e in g.es:
      # NOTE: "%.10f" % rel_weight
      weight = ("%%.%df" % PRECISION) % (e['weight'] / max_weight)
      # Remove too small edge
      if weight != ('0.' + '0' * PRECISION):
        f.write("%s %s %.10f\n" % (g.vs[e.source]['name'], g.vs[e.target]['name'], e['weight'] / max_weight))

if __name__ == '__main__':
  if len(sys.argv) != 3:
    print('usage: %s input.lgl output.linkcomm.txt' % sys.argv[0])
    exit(1)
  main(sys.argv[1], sys.argv[2])
