#!/usr/bin/env python
# -*- coding: utf-8 -*-

import random

cdef extern from "stdlib.h":
  ctypedef unsigned size_t # avoid...
  void *malloc(size_t size)
  void *realloc(void *ptr, size_t size)
  void free(void *ptr)

cdef extern from "math.h":
  double sqrt(double x)

cdef extern from "float.h":
  enum:
    DBL_MAX

cdef class TreeNode:
  cdef double x     # voronoi point x
  cdef double y     # voronoi point y
  cdef double cx    # centroid of region
  cdef double cy    # centroid of region

  cdef int *regionx # region x array
  cdef int *regiony # region y array

  cdef double area  # area
  cdef int region_cnt # count
  cdef int region_alloced # alloced count

  cdef object name  # name
  cdef object children # children

  INIT_ALLOCED = 512

  def __init__(self, name, area, *children):
    self.area = area
    self.name = name

    self.children = children

    self.region_cnt = 0

    self.regionx = <int *>malloc(sizeof(int) * self.INIT_ALLOCED)
    self.regiony = <int *>malloc(sizeof(int) * self.INIT_ALLOCED)
    self.region_alloced = self.INIT_ALLOCED

  def __del__(self):
    if self.regionx != NULL:
      free(self.regionx)
    if self.regiony != NULL:
      free(self.regiony)

  def euc_dist(self, double x, double y):
    cdef double dx
    cdef double dy
    dx = self.x - x
    dy = self.y - y
    return sqrt(dx * dx + dy * dy)

  def pw_dist(self, double x, double y, double weight):
    cdef double dx
    cdef double dy
    dx = self.x - x
    dy = self.y - y
    return dx * dx + dy * dy - weight

  def clear_region(self):
    self.region_cnt = 0

  def add_region_point(self, int x, int y):
    # TODO: error handing
    if self.region_cnt >= self.region_alloced:
      self.region_alloced *= 2
      self.regionx = <int *>realloc(self.regionx, sizeof(int) * self.region_alloced)
      self.regiony = <int *>realloc(self.regiony, sizeof(int) * self.region_alloced)
    self.regionx[self.region_cnt] = x
    self.regiony[self.region_cnt] = y
    self.region_cnt += 1

  def region_size(self):
    return self.region_cnt

  def calc_centroid(self):
    if self.region_cnt == 0:
      return
    cdef int cx = 0
    cdef int cy = 0
    for i in range(self.region_cnt):
      cx += self.regionx[i]
      cy += self.regiony[i]
    self.cx = float(cx) / self.region_cnt
    self.cy = float(cy) / self.region_cnt

  def move_to(self, x, y):
    self.x = x
    self.y = y

  def child_move_to(self, child_index, int x, int y):
    # FIXME: kousokuka!
    for i in range(self.region_cnt):
      if x == self.regionx[i] and y == self.regiony[i]:
        self.children[child_index].move_to(x, y)
        return 1
    return 0

  def child_adesired(self):
    ret = []
    cdef double all_areas = 0.0
    for c in self.children:
      all_areas += c.get_area()
    for c in self.children:
      ret.append(c.get_area() / all_areas)
    return ret

  def get_area(self):
    return self.area

  def move_to_centroid(self):
    self.calc_centroid()
    self.x = self.cx
    self.y = self.cy

  def region_draw(self, object im, object draw, object font):
    if self.children:
      for c in self.children:
        c.region_draw(im, draw, font)
    else:
      color = (random.randint(0,255), random.randint(0,255), random.randint(0,255))
      for i in range(self.region_cnt):
        im.putpixel((self.regionx[i], self.regiony[i]), color)
    self.calc_centroid()
    if font:
      draw.text((self.cx, self.cy), self.name, font = font, fill = 0)

  def nchildren(self):
    return len(self.children)

  def __getitem__(self, key):
    return self.children[key]

  def __iter__(self):
    return self.children.__iter__()

  def next(self):
    return self.children.next()

  def str_r(self, indent):
    s = indent + '(x: %s, y: %s, area: %d, region_size: %d)\n' % (self.x, self.y, self.area, self.region_size())
    for c in self.children:
      s += c.str_r(indent + '  ')
    return s

  def __str__(self):
    return self.str_r('')

  def calc_voronoi_tessellation(self, object weights):
    for c in self.children:
      c.clear_region()
    cdef int min_index
    cdef double min_dist
    cdef double dist
    for i in range(self.region_cnt):
      min_index = 0
      min_dist = DBL_MAX
      for j, tn in enumerate(self.children):
        dist = tn.pw_dist(self.regionx[i], self.regiony[i], weights[j])
        # print '(%d, %d): %d - %d' % (self.regionx[i], self.regiony[i], j, dist)
        if dist < min_dist:
          min_dist = dist
          min_index = j
      # print '(%d, %d): %d' % (self.regionx[i], self.regiony[i], min_index)
      self.children[min_index].add_region_point(self.regionx[i], self.regiony[i])
