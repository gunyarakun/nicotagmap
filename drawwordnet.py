#!/usr/bin/python

import igraph
import cairo
from lxml import etree

def main():
  g = igraph.read('tag_wordnet.lgl', 'lgl')

  # draw graph
  for v in g.vs:
    v['label'] = v['name']
  p = igraph.drawing.Plot('tag_wordnet.pdf', igraph.drawing.BoundingBox(6000, 6000))
  p.add(g)
  p.save()

if __name__ == '__main__':
  main()
