#!/usr/bin/python
from distutils.core import setup
from distutils.extension import Extension

try:
  from Cython.Distutils import build_ext
  no_cython= False
except ImportError:
  no_cython = True

if no_cython:
    cmdclass = {}
    src = ['src/vorotree.c']
else:
    cmdclass = { 'build_ext': build_ext }
    src = ['src/vorotree.pyx']

setup(
  name = 'vorotree',
  version = '0.1',
  description = 'class for Voronoi Treemaps',
  long_description = '''
  class for Voronoi Treemaps
  ''',
  license='GNU LESSER GENERAL PUBLIC LICENSE',
  author = 'Brazil',
  author_email = 'a at razil.jp',
  ext_modules = [Extension('vorotree',
                         src,
                         libraries = ['m']),],
  cmdclass = cmdclass)

