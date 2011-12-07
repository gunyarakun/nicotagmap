// Voronoi Treemap with CGAL
// (c) 2008 Tasuku SUENAGA a.k.a. gunyarakun

// 全くC++っぽくない。
// CGALはboostを必要とするので、boost使いまくってよいのだぞ。

#include <CGAL/basic.h>

// standard includes
#include <iostream>
#include <fstream>
#include <cassert>

#include <CGAL/Simple_cartesian.h>
#include <CGAL/Filtered_kernel.h>

// includes for defining the Voronoi diagram adaptor
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Regular_triangulation_2.h>
#include <CGAL/Regular_triangulation_euclidean_traits_2.h>
#include <CGAL/Voronoi_diagram_2.h>
#include <CGAL/Regular_triangulation_adaptation_traits_2.h>
#include <CGAL/Triangulation_vertex_base_with_info_2.h>
#include <CGAL/Triangulation_data_structure_2.h>

// for tn.region
#include <CGAL/Polygon_2.h>

// for convex full
#include <CGAL/ch_graham_andrew.h>

// for random point generation
#include <CGAL/enum.h>
#define DSFMT_MEXP 216091
#include "dSFMT.h"
dsfmt_t dsfmt;

// for draw
#include <cairo.h>
#include <cairo-pdf.h>

// C daisuki
#include <cstdio>

// for color
void cl2pix(double* R, double* G, double* B, double c, double l);

// precition
#include <CGAL/Lazy_exact_nt.h>
#include <CGAL/Gmpq.h>

#if 1
typedef CGAL::Lazy_exact_nt<CGAL::Gmpq> NT;
//typedef CGAL::Gmpq NT;
#else
typedef long double NT;
#endif
struct CK : public CGAL::Simple_cartesian<NT> {};
typedef CK Rep;
typedef CGAL::Filtered_kernel<CK> DT_GT;
struct RT_GT : public CGAL::Regular_triangulation_filtered_traits_2<DT_GT> {} ;
typedef RT_GT::Weighted_point_2 Weighted_point_2;

// typedefs for defining the adaptor
/*
typedef CGAL::Triangulation_vertex_base_with_info_2<TreeNode *, RT_GT,
            CGAL::Regular_triangulation_vertex_base_2<RT_GT> > Vb;
typedef CGAL::Regular_triangulation_face_base_2<RT_GT> Fb;
typedef CGAL::Triangulation_data_structure_2<Vb, Fb> Tds;
typedef CGAL::Regular_triangulation_2<RT_GT, Tds> RT2;
*/
typedef CGAL::Regular_triangulation_2<RT_GT> RT2;
typedef CGAL::Regular_triangulation_adaptation_traits_2<RT2> RT_AT2;
typedef CGAL::Voronoi_diagram_2<RT2,RT_AT2> PD2;

// typedef for the result type of the point location
typedef RT_AT2::Site_2                    Site_2;
typedef RT_AT2::Point_2                   Point_2;
typedef RT2::Geom_traits                  Geom_traits;
typedef Geom_traits::Ray_2                Ray_2;
typedef Geom_traits::Line_2               Line_2;
typedef Geom_traits::Segment_2            Segment_2;
typedef Geom_traits::Vector_2             Vector_2;
typedef CGAL::Bbox_2                      Bbox_2;
typedef CGAL::Polygon_2<Geom_traits>      Polygon_2;
typedef Geom_traits::Iso_rectangle_2      Rectangle_2;

typedef PD2::Locate_result             Locate_result;
typedef PD2::Vertex_handle             Vertex_handle;
typedef PD2::Face_handle               Face_handle;
typedef PD2::Halfedge_handle           Halfedge_handle;
typedef PD2::Ccb_halfedge_circulator   Ccb_halfedge_circulator;

struct TreeNode {
  Weighted_point_2 p;
  CGAL::Polygon_2<RT_GT> region;

  NT adesired;

  const char *name;
  NT given_area;    // area given for weight
  std::list<TreeNode> children;
  NT children_sum_given_areas;

  PD2::Face_handle face;

  TreeNode(const char *_name, double _given_area) :
    name(_name),
    given_area(_given_area),
    children_sum_given_areas(0)
  {
  }

  void add_child(TreeNode &tn) {
    children.push_back(tn);
    children_sum_given_areas += tn.given_area;
  }

  bool children_overlap_check() const;
  void draw() const;
  void draw_region(cairo_t *cr) const;
  void draw_text(cairo_t *cr) const;
  void write_centroid(std::ostream &os) const;

  std::ostream& print_ostream(std::ostream &os) const {
    if (name) {
      os << "name: " << name << " ";
    } else {
      os << "name: anonymous ";
    }
    os << "p: (" << CGAL::to_double(p.x()) << ", " << CGAL::to_double(p.y()) << ") ";
    os << "adesired: " << adesired << " ";
    os << "area: " << CGAL::to_double(region.area()) << " ";
    os << "childrens: " << children.size() << " ";
    os << "region:";
    Polygon_2::Vertex_iterator pit;
    for (pit = region.vertices_begin();
         pit != region.vertices_end();
         ++pit) {
      os << " (" << CGAL::to_double(pit->x()) << ", " << CGAL::to_double(pit->y()) << ")";
    }
    return os;
  }

  void print() const {
    this->print_ostream(std::cout);
    std::cout << std::endl;
    std::cout << "* childrens *" << std::endl;
    std::list<TreeNode>::const_iterator c;
    for (c = children.begin(); c != children.end(); ++c) {
      c->print_ostream(std::cout);
      std::cout << std::endl;
    }
  }
};

std::ostream &
operator<<(std::ostream &os, const TreeNode &tn)
{
  return tn.print_ostream(os);
}

NT epsilon(0.0008);
NT nt_one(1);

static Point_2 centroid(const Polygon_2& poly)
{
  assert(poly.size() >= 3);

  Polygon_2::Vertex_circulator vcir = poly.vertices_circulator();
  Polygon_2::Vertex_circulator vend = vcir;
  Polygon_2::Vertex_circulator vnext = vcir; ++vnext;

  Vector_2 centre(0, 0);
  NT a(0), asum(0);
  do {
    a = (vcir->x() * vnext->y()) - (vnext->x() * vcir->y());
    centre = centre + a * ((*vcir - CGAL::ORIGIN) + (*vnext - CGAL::ORIGIN)); // slow...
    asum += a;
    vcir = vnext;
    ++vnext;
  } while(vcir != vend);
  centre = centre / (asum * 3);
  return CGAL::ORIGIN + centre;
}

int voronoi_tessellation(TreeNode &tn)
{
  assert(tn.region.is_convex()); // tn.regionは凸であることを前提
  assert(tn.children.size() >= 2);

  PD2 pd2;
  std::list<TreeNode>::iterator cit;
  for (cit = tn.children.begin(); cit != tn.children.end(); ++cit) {
    cit->face = pd2.insert(cit->p);
    cit->region = Polygon_2(); // 全部初期化しておく
  }

  assert( pd2.is_valid() );

  PD2::Delaunay_graph dg = pd2.dual();
  PD2::Delaunay_graph::Finite_vertices_iterator vit;
  for (vit = dg.finite_vertices_begin();
       vit != dg.finite_vertices_end(); ++vit) {

    // tn.regionとintersectする。Ray/Lineは切り取られてSegmentとなる。
    PD2::Delaunay_graph::Edge_circulator eci = dg.incident_edges(vit);
    PD2::Delaunay_graph::Edge_circulator ece = eci;
    std::set<Point_2> point_set;
    if (eci != NULL) {
      Polygon_2::Vertex_iterator pvit;
      do {
        Line_2 l;
        Ray_2 r;
        Segment_2 s;
        if (!dg.is_infinite(eci)) {
          CGAL::Object o = dg.dual(eci);
          if (CGAL::assign(s, o)) {
            // std::cout << "Segment: (" << s.source().x() << ", " << s.source().y() << ") (" << s.target().x() << ", " << s.target().y() << ")" << std::endl;
            if (tn.region.bounded_side(s.source()) != CGAL::ON_UNBOUNDED_SIDE) {
              point_set.insert(s.source());
            }
            if (tn.region.bounded_side(s.target()) != CGAL::ON_UNBOUNDED_SIDE) {
              point_set.insert(s.target());
            }
            Polygon_2::Edge_const_iterator eit;
            for (eit = tn.region.edges_begin(); eit != tn.region.edges_end(); ++eit) {
              CGAL::Object obj = CGAL::intersection(*eit, s);
              Point_2 p;
              Segment_2 seg;
              if (CGAL::assign(p, obj)) {
                point_set.insert(p);
              } else if (CGAL::assign(seg, obj)) {
                assert(false);
              }
            }
          }
          if (CGAL::assign(r, o)) {
            // std::cout << "Ray: (" << r.source().x() << ", " << r.source().y() << ") " << r << std::endl;
            if (tn.region.bounded_side(r.source()) != CGAL::ON_UNBOUNDED_SIDE) {
              point_set.insert(r.source());
            }
            Polygon_2::Edge_const_iterator eit;
            for (eit = tn.region.edges_begin(); eit != tn.region.edges_end(); ++eit) {
              CGAL::Object obj = CGAL::intersection(*eit, r);
              Point_2 p;
              Segment_2 seg;
              if (CGAL::assign(p, obj)) {
                point_set.insert(p);
              } else if (CGAL::assign(seg, obj)) {
                assert(false);
              }
            }
          }
          if (CGAL::assign(l, o)) {
            // std::cout << "Line: (" << l.point(0).x() << ", " << l.point(0).y() << ") (" << l.direction().dx() << ", " << l.direction().dy() << ")" << std::endl;
            Polygon_2::Edge_const_iterator eit;
            for (eit = tn.region.edges_begin(); eit != tn.region.edges_end(); ++eit) {
              CGAL::Object obj = CGAL::intersection(*eit, l);
              Point_2 p;
              Segment_2 seg;
              if (CGAL::assign(p, obj)) {
                point_set.insert(p);
              } else if (CGAL::assign(seg, obj)) {
                assert(false);
              }
            }
          }
        } // infinite
      } while (++eci != ece);

      // tn.regionのpolygon中の点で、領域内のものをぜんぶ入れる
      for (pvit = tn.region.vertices_begin();
           pvit != tn.region.vertices_end();
           ++pvit) {
        /*
        // NOTE: locateはちょっとバグってるみたい。
        // 代わりに、距離が最短な点を探す。
        // NOTE: バグってないかも。代替ロジックでも問題が出た。
        PD2::Locate_result lr = pd2.locate(*pvit);
        if (PD2::Vertex_handle *v = boost::get<PD2::Vertex_handle>(&lr)) {
          // 交わってるはず
          assert(false);
        } elsentersection if (PD2::Halfedge_handle *e = boost::get<Halfedge_handle>(&lr)) {
          // 交わってるはず
          assert(false);
        } else if (PD2::Face_handle *f = boost::get<Face_handle>(&lr)) {
          // 今対象としている点の領域だったら、追加
          if ((*f)->dual()->point() == vit->point()) {
            point_set.insert(*pvit);
          }
        }
        */
        // tn.regionの頂点と子供の頂点とのpower distanceを
        // すべて計算し、着目している子供に属するかを調べる
        cit = tn.children.begin();
        Weighted_point_2 cp = cit->p;
        ++cit;
        for (; cit != tn.children.end(); ++cit) {
          if (CGAL::compare_power_distance(cp, cit->p, *pvit)
              == CGAL::LARGER) {
            cp = cit->p;
          }
        }
        if (cp == vit->point()) {
          // std::cout << "RegionPoint: (" << pvit->x() << ", " << pvit->y() << ")" << std::endl;
          point_set.insert(*pvit);
        }
      }
      assert(point_set.size() != 1 && point_set.size() != 2);
    }

    // 対応する子供を見つける
    for (cit = tn.children.begin(); cit != tn.children.end(); ++cit) {
      if (vit->point() == cit->p) {
        // 自前でのソートが面倒なのでconvex hullを求めてPolygonを作る。
        CGAL::ch_graham_andrew(point_set.begin(), point_set.end(),
                               std::inserter(cit->region, cit->region.vertices_begin()));
        assert(point_set.size() == 0 || cit->region.size() > 0);
        assert(point_set.size() == cit->region.size()); // 頂点の数あってるはず

        assert(cit->region.area() <= tn.region.area());
      }
    }
  }

  return 0;
}

bool
TreeNode::children_overlap_check() const
{
  std::set<Point_2> points;
  std::list<TreeNode>::const_iterator tit;
  for (tit = children.begin(); tit != children.end(); ++tit) {
    points.insert(tit->p);
  }
  return (points.size() != children.size());
}

// TODO: 入れ違いにする。つまり、x/yそれぞれがuniqueとなるようにする。
void
random_points_in_polygon(TreeNode &tn, NT local_epsilon)
{
  Bbox_2 bbox = tn.region.bbox();
  NT width = bbox.xmax() - bbox.xmin();
  NT height = bbox.ymax() - bbox.ymin();

  std::set<Point_2> pts;
  unsigned int i;
  do {
    Point_2 p(NT(dsfmt_genrand_open_open(&dsfmt)) * width + bbox.xmin(),
              NT(dsfmt_genrand_open_open(&dsfmt)) * height + bbox.ymin());
    if (tn.region.bounded_side(p) == CGAL::ON_BOUNDED_SIDE) {
      pts.insert(p);
    }
  } while (pts.size() < tn.children.size());

  bool loop;
  do {
    std::set<Point_2>::iterator pit;
    std::list<TreeNode>::iterator tit;
    loop = false;
    for (pit = pts.begin(), tit = tn.children.begin();
         pit != pts.end() && tit != tn.children.end();
         ++pit, ++tit) {
      tit->p = Weighted_point_2(*pit, 1.0); // weightの初期値は1
      tit->adesired = tit->given_area / tn.children_sum_given_areas;
      // epsilon未満の領域は削除
      if (tit->adesired < local_epsilon) {
        std::cout << "removed node: " << *tit << std::endl;
        tn.children_sum_given_areas -= tit->given_area;
        tn.children.erase(tit);
        // 削除したら、すべての点のadesiredは計算しなおし
        loop = true;
        break;
      }
    }
  } while (loop);
}

#define MAX_LOOPS 256
void
voronoi_treemap(TreeNode &tn)
{
  // std::cout << tn << std::endl;
  std::cout << "*" << std::endl;

  bool invalid;
  bool loop;
  NT local_epsilon(epsilon);
  do {
    loop = false;
    invalid = false;

    random_points_in_polygon(tn, local_epsilon);

    if (tn.children.size() == 1) {
      tn.children.front().region = tn.region;
    } else {
      unsigned int loop_cnt = MAX_LOOPS;
      do {
        loop = false;
        voronoi_tessellation(tn);

        std::list<TreeNode>::iterator tit;
        for (tit = tn.children.begin(); tit != tn.children.end(); ++tit) {
          NT diff;
          diff = tit->adesired - tit->region.area() / tn.region.area();

          if (CGAL::abs(diff) > local_epsilon) {
            loop = true;
          }

          // adjust weights / move centroid
          NT new_weight;
          new_weight = tit->p.weight() * (nt_one + diff / tit->adesired);
          if (new_weight < nt_one) { new_weight = nt_one; }

          if (tit->region.size() < 3) {
            // 領域が空の際には、点が重なっている場合とそれ以外がある
            if (tn.children_overlap_check()) {
              // 同じポイントに点群が移動している場合は、仕切りなおし
              invalid = true;
              loop = false;
              std::cerr << "* point overlap detected!!! reinit!" << std::endl;
              break;
            } else {
              // 点移動はせずに、重みだけを調整する
              tit->p = Weighted_point_2(tit->p.point(), new_weight);
              // loop = true;
            }
          } else {
            Point_2 cent = centroid(tit->region);
            assert(tn.region.bounded_side(cent) == CGAL::ON_BOUNDED_SIDE);
            // わざと精度をdoubleまで落とす。
            // この後のPower diagramの計算がむちゃ早くなる。
            tit->p = Weighted_point_2(Point_2(CGAL::to_double(cent.x()), CGAL::to_double(cent.y())), CGAL::to_double(new_weight));
          }
          // printf("[%s] x:%f y:%f adesired:%f diff:%f weight:%f", tit->name, CGAL::to_double(tit->p.x()), CGAL::to_double(tit->p.y()), CGAL::to_double(tit->adesired), CGAL::to_double(diff), CGAL::to_double(new_weight));
        }
        // printf("\n");
      } while (loop && loop_cnt--);
    }
    if (loop) {
      local_epsilon = local_epsilon * NT(2); // epsilonを緩和する
    }
  } while (loop || invalid);

  // children recursive
  std::list<TreeNode>::iterator tit;
  for (tit = tn.children.begin(); tit != tn.children.end(); ++tit) {
    // 孫がいて、子供の領域がある場合のみ再帰
    if (tit->children.size() && tit->region.size() >= 3) {
      voronoi_treemap(*tit);
    }
  }
}

#if 1
#define IMG_FILENAME "output.pdf"
#define PDF_OUTPUT
#else
#define IMG_FILENAME "output.png"
#define PNG_OUTPUT
#endif
#define NODE_FILENAME "output.txt"

void TreeNode::draw() const
{
  std::ofstream ofs(NODE_FILENAME);
  cairo_t *cr;
  cairo_surface_t *surface;
  CGAL::Bbox_2 bbox = region.bbox();

  assert(bbox.xmin() >= 0.0);
  assert(bbox.ymin() >= 0.0);

#ifdef PDF_OUTPUT
  surface = cairo_pdf_surface_create(IMG_FILENAME, bbox.xmax(), bbox.ymax());
#endif
#ifdef PNG_OUTPUT
  surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, bbox.xmax(), bbox.ymax());
#endif
  cr = cairo_create(surface);

  cairo_select_font_face(cr, "serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(cr, 10.0);

  draw_region(cr);
  draw_text(cr);
  write_centroid(ofs);

  cairo_show_page(cr);
  cairo_destroy(cr);
#ifdef PNG_OUTPUT
  cairo_surface_write_to_png(surface, IMG_FILENAME);
#endif
  cairo_surface_destroy(surface);
}

void TreeNode::draw_region(cairo_t *cr) const {
  if (region.size() >= 3) {
    if (this->name) {
      Polygon_2::Vertex_circulator vcir, vend;
      vcir = vend = region.vertices_circulator();
      Bbox_2 bbox = region.bbox();

      cairo_pattern_t *fill =
        cairo_pattern_create_linear(bbox.xmin(), bbox.ymin(),
                                    bbox.xmax(), bbox.ymax());
      cairo_pattern_add_color_stop_rgb(fill, 0.0, 1.0, 1.0, 1.0);

      /* L*a*b* 色空間
      {
        double r, g, b;
        cl2pix(&r, &g, &b,
          dsfmt_genrand_open_open(&dsfmt),
          dsfmt_genrand_open_open(&dsfmt));
        cairo_pattern_add_color_stop_rgb(fill, 1.0, r, g, b);
      }
      */
      // 単なるRGB
      cairo_pattern_add_color_stop_rgb(fill, 1.0,
        dsfmt_genrand_open_open(&dsfmt),
        dsfmt_genrand_open_open(&dsfmt),
        dsfmt_genrand_open_open(&dsfmt));

      cairo_move_to(cr, CGAL::to_double(vcir->x()), CGAL::to_double(vcir->y()));
      ++vcir;

      do {
        cairo_line_to(cr, CGAL::to_double(vcir->x()), CGAL::to_double(vcir->y()));
        ++vcir;
      } while (vcir != vend);
      cairo_line_to(cr, CGAL::to_double(vend->x()), CGAL::to_double(vend->y()));
      cairo_set_source(cr, fill);
      cairo_fill_preserve(cr);
      cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
      cairo_stroke(cr);
      cairo_pattern_destroy(fill);
    }
  }

  std::list<TreeNode>::const_iterator tit;
  for (tit = children.begin(); tit != children.end(); ++tit) {
    tit->draw_region(cr);
  }
}

void TreeNode::draw_text(cairo_t *cr) const {
  if (this->name) {
    cairo_text_extents_t extents;
    cairo_text_extents(cr, name, &extents);
    cairo_move_to(cr,
      CGAL::to_double(p.x() - (extents.width / 2)),
      CGAL::to_double(p.y() - (extents.height / 2)));
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_show_text(cr, name);
  }

  std::list<TreeNode>::const_iterator tit;
  for (tit = children.begin(); tit != children.end(); ++tit) {
    tit->draw_text(cr);
  }
}

void TreeNode::write_centroid(std::ostream &os) const {
  if (name) {
    os << name;
  }
  os << "\t" << CGAL::to_double(p.x()) << "\t" << CGAL::to_double(p.y())
     << std::endl;

  std::list<TreeNode>::const_iterator tit;
  for (tit = children.begin(); tit != children.end(); ++tit) {
    tit->write_centroid(os);
  }
}

int
main(int argc, char *argv[])
{
  // 乱数初期化
  dsfmt_init_gen_rand(&dsfmt, 0);
  dsfmt_gv_init_gen_rand(0);

  // 周りの環境でクリップする
  Polygon_2 world;
  world.push_back(Polygon_2::Point_2(1, 1));
  world.push_back(Polygon_2::Point_2(2400, 1));
  world.push_back(Polygon_2::Point_2(2400, 2400));
  world.push_back(Polygon_2::Point_2(1, 2400));

#include "treedata.inc"

  /*
  world.push_back(Polygon_2::Point_2(500.0, 0.0));
  world.push_back(Polygon_2::Point_2(1000.0, 1000.0));
  world.push_back(Polygon_2::Point_2(0.0, 1000.0));
  TreeNode tn1("tn1", 500);
  TreeNode tn2("tn2", 100);
  TreeNode tn3("tn3", 300);
  TreeNode tn4("tn4", 400);
  TreeNode tn5("tn5", 800);
  TreeNode tn6("tn6", 400);
  TreeNode tnroot("root", 0);
  tnroot.add_child(tn1);
  tnroot.add_child(tn2);
  tnroot.add_child(tn3);
  tnroot.add_child(tn4);
  tnroot.add_child(tn5);
  tnroot.add_child(tn6);
#define ROOT_TREE_NODE tnroot
*/
  ROOT_TREE_NODE.region = world;
  voronoi_treemap(ROOT_TREE_NODE);
  ROOT_TREE_NODE.draw();
}
