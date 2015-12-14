#!/usr/bin/env python

import sys
from igraph import *

PRECISION = 8

def main(infile, outfile, threshold):
  g = Graph.Read_Lgl(infile, directed=False)

  max_weight = 0
  for e in g.es:
    if max_weight < e['weight']:
      max_weight = e['weight']

  with open(outfile, 'w') as f:
    for e in g.es:
      weight = ("%%.%df" % PRECISION) % (e['weight'] / max_weight)
      # Remove too small edge
      if float(weight) > threshold:
        f.write("%s %s %.10f\n" % (g.vs[e.source]['name'], g.vs[e.target]['name'], e['weight'] / max_weight))

if __name__ == '__main__':
  if len(sys.argv) != 4:
    print('usage: %s input.lgl output.linkcomm.txt threshold' % sys.argv[0])
    exit(1)
  main(sys.argv[1], sys.argv[2], float(sys.argv[3]))
