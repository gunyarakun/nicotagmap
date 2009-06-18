#!/usr/bin/python

import sys
import igraph
from lxml import etree

def main(infile, outfile):
  g = igraph.read(infile, 'lgl')

  # write XGMML
  root = etree.XML('<?xml version="1.0" encoding="UTF-8" standalone="yes"?><graph label="Network" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#" xmlns:cy="http://www.cytoscape.org" xmlns="http://www.cs.rpi.edu/XGMML"  directed="0" />')
  for v in g.vs:
    etree.SubElement(root, 'node', label = v['name'].decode('utf-8'),
                                   id = str(v.index))
  for e in g.es:
    etree.SubElement(root, 'edge', source = str(e.source),
                                   target = str(e.target),
                                   label = '')
  file = open(outfile, 'w')
  file.write(etree.tostring(root, encoding = 'utf-8',
                                  pretty_print = True))
  file.close()

if __name__ == '__main__':
  if len(sys.argv) != 3:
    print 'usage: ./lgl2xgmml.py input.lgl output.xgmml'
    exit(1)
  main(sys.argv[1], sys.argv[2])
