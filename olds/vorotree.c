// 2008 Tasuku SUENAGA
// tekito implementation of vonoroi pw treemap

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <string.h>
#include <float.h>
#include <limits.h>
#include <math.h>

typedef struct _treenode treenode;
struct _treenode {
  short x;
  short y;

  short *region; // size region_cnt / 2, x y x y...
  unsigned int region_cnt;

  char *name;
  double area;    // area given for weight
  treenode **children;
  unsigned int children_cnt;
  unsigned int children_alloced;
  double children_areas;
};

#define INIT_CHILDRENS 16

treenode *treenode_init(char *name, double area)
{
  treenode *t;
  if ((t = calloc(1, sizeof(treenode)))) {
    if ((t->children = malloc(sizeof(treenode *) * INIT_CHILDRENS))) {
      t->name = strdup(name); // error check mendoi
      t->area = area;
      t->children_areas = 0.0;
      t->children_alloced = INIT_CHILDRENS;
      return t;
    }
    free(t);
  }
  return NULL;
}

void treenode_set_region(treenode *t, short *region, unsigned int region_cnt)
{
  t->region = region;
  t->region_cnt = region_cnt;
}

double treenode_pw_dist(treenode *t, short x, short y, double weight)
{
  short dx = t->x - x;
  short dy = t->y - y;
  return (double)((dx * dx) + (dy * dy)) - weight;
}

void treenode_alloc_region(treenode *t, size_t alloc)
{
  if (!t->region) {
    t->region = malloc(sizeof(short) * alloc * 2);
  }
}

inline void treenode_add_region_point(treenode *t, short x, short y)
{
  t->region[t->region_cnt * 2] = x;
  t->region[t->region_cnt * 2 + 1] = y;
  t->region_cnt += 1;
}

void treenode_calc_voronoi_tessellation(treenode *t, double *weights)
{
  unsigned int i;
  assert(t->children_cnt);
  assert(t->region_cnt);

  for (i = 0; i < t->children_cnt; i++) {
    t->children[i]->region_cnt = 0;
  }
  for (i = 0; i < t->region_cnt; i++) {
    unsigned int j;
    double min_dist = DBL_MAX;
    unsigned int min_index = 0;
    for (j = 0; j < t->children_cnt; j++) {
      double dist;
      dist = treenode_pw_dist(t->children[j],
                              t->region[i * 2],
                              t->region[i * 2 + 1],
                              weights[j]);
      if (dist < min_dist) {
        min_dist = dist;
        min_index = j;
      }
    }
    treenode_add_region_point(t->children[min_index], t->region[i * 2], t->region[i * 2 + 1]);
  }
}

void treenode_add_child(treenode *t, treenode *child)
{
  if (t->children_cnt >= t->children_alloced) {
    t->children_cnt *= 2;
    if (realloc(t->children, sizeof(treenode *) * t->children_cnt)) {
      exit(1); // 頑張ってもしゃーない
    }
  }
  t->children[t->children_cnt++] = child;
  t->children_areas += child->area;
};

int treenode_point_in_region(treenode *t, short x, short y)
{
  unsigned int i;
  for (i = 0; i < t->region_cnt; i++) {
    if (t->region[i * 2] == x && t->region[i * 2 + 1] == y) {
      return 1;
    }
  }
  return 0;
}

int treenode_scatter_child(treenode *t)
{
  unsigned int i, idxs[t->children_cnt];
  // 1つの子供に30pxはほしい
  if (t->region_cnt < t->children_cnt * 30) {
    return 0;
  }
  for(i = 0; i < t->children_cnt; ) {
    unsigned int idx, j;
    idx = lrand48() % t->region_cnt;
    idxs[i++] = idx;
    for (j = 0; j < i - 1; j++) {
      if (idxs[j] == idx) {
        i--;
      }
    }
  }

  for (i = 0; i < t->children_cnt; i++) {
    t->children[i]->x = t->region[idxs[i] * 2];
    t->children[i]->y = t->region[idxs[i] * 2 + 1];
  }

  return 1;
}

void treenode_move_centroid(treenode *t)
{
  unsigned int i;
  unsigned long sx = 0, sy = 0;

  if (!t->region_cnt) {
    puts("region count is zero in treenode_move_centroid.");
    exit(1);
  }

  for (i = 0; i < t->region_cnt; i++) {
    sx += t->region[i * 2];
    sy += t->region[i * 2 + 1];
  }
  t->x = sx / t->region_cnt;
  t->y = sy / t->region_cnt;
}

#define EPSILON 0.01
void treenode_calc_voronoi_treemap(treenode *t)
{
  unsigned int i;
  double weights[t->children_cnt];
  double adesired[t->children_cnt];

  for (i = 0; i < t->children_cnt; i++) {
    weights[i] = 1.0;
    adesired[i] = t->children[i]->area / t->children_areas;
    treenode_alloc_region(t->children[i], t->region_cnt);
  }
  if (!treenode_scatter_child(t)) {
    return;
  }
  while(1) {
    int end = 1;
    double diff;
    treenode_calc_voronoi_tessellation(t, weights);
    for (i = 0; i < t->children_cnt; i++) {

      // calc diff adesired
      diff = adesired[i] - (double)t->children[i]->region_cnt / t->region_cnt;
      // end check
      if (fabs(diff) > EPSILON) {
        end = 0;
      }
      // adjust weights
      weights[i] *= 1.0 + diff / adesired[i];
      if (weights[i] < 1.0) {
        weights[i] = 1.0;
      }
      // move centroid
      if (t->children[i]->region_cnt) {
        treenode_move_centroid(t->children[i]);
      }

      printf("[%d] x:%d y:%d adesired:%f diff:%f weight:%f", i, t->children[i]->x, t->children[i]->y, adesired[i], diff, weights[i]);
    }
    printf("\n");
    if (end) {
      break;
    }
  }
  for (i = 0; i < t->children_cnt; i++) {
    if (t->children[i]->children_cnt) {
      treenode_calc_voronoi_treemap(t->children[i]);
    }
  }
}

int
main(int argc, char *argv[])
{
  /* copy paste */
  {
    short x;
    short y;

    treenode_alloc_region(tn1, 600 * 600);
    for (x = 0; x < 600; x++) {
      for (y = 0; y < 600; y++) {
        treenode_add_region_point(tn1, x, y);
      }
    }
    treenode_calc_voronoi_treemap(tn1);
  }
}
