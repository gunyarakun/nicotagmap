#!/usr/bin/python

import igraph
import cairo

def main():
  g = igraph.read('tag_wordnet.lgl', 'lgl')
  for v in g.vs:
    v['label'] = v['name']
  d = g.community_fastgreedy()

  p = igraph.drawing.Plot('graphtree.pdf', igraph.drawing.BoundingBox(5000, 13000))
  # p.add(g)
  p.add(d)
  p.save()

if __name__ == '__main__':
  main()
