/* Copyright (c) David Dalrymple 2011 */
#include <math.h>
#define TAU 6.283185307179586476925287 // also known as "two pi" to the unenlightened

double finv(double t) {
  return (t>(6.0/29.0))?(t*t*t):(3*(6.0/29.0)*(6.0/29.0)*(t-4.0/29.0));
}

double correct(double cl) {
  double a = 0.055;
  return (cl<=0.0031308)?(12.92*cl):((1+a)*pow(cl,1/2.4)-a);
}

/* Convert from L*a*b* doubles to XYZ doubles
 * Formulas drawn from http://en.wikipedia.org/wiki/Lab_color_spaces
 */
void lab2xyz(double* x, double* y, double* z, double l, double a, double b) {
  double sl = (l+0.16)/1.16;
  double ill[3] = {0.9643,1.00,0.8251}; //D50
  *y = ill[1] * finv(sl);
  *x = ill[0] * finv(sl + (a/5.0));
  *z = ill[2] * finv(sl - (b/2.0));
}

/* Convert from XYZ doubles to sRGB bytes
 * Formulas drawn from http://en.wikipedia.org/wiki/Srgb
 */
void xyz2rgb(double* r, double* g, double* b, double x, double y, double z) {
  double rl =  3.2406*x - 1.5372*y - 0.4986*z;
  double gl = -0.9689*x + 1.8758*y + 0.0415*z;
  double bl =  0.0557*x - 0.2040*y + 1.0570*z;
  int clip = (rl < 0.0 || rl > 1.0 || gl < 0.0 || gl > 1.0 || bl < 0.0 || bl > 1.0);
  if(clip) {
    rl = (rl<0.0)?0.0:((rl>1.0)?1.0:rl);
    gl = (gl<0.0)?0.0:((gl>1.0)?1.0:gl);
    bl = (bl<0.0)?0.0:((bl>1.0)?1.0:bl);
  }
  //Uncomment the below to detect clipping by making clipped zones red.
  //if(clip) {rl=1.0;gl=bl=0.0;}
  *r = correct(rl);
  *g = correct(gl);
  *b = correct(bl);
}

/* Convert from LAB doubles to sRGB bytes
 * (just composing the above transforms)
 */
void lab2rgb(double* R, double* G, double* B, double l, double a, double b) {
  double x,y,z;
  lab2xyz(&x,&y,&z,l,a,b);
  xyz2rgb(R,G,B,x,y,z);
}

/* Convert from a qualitative parameter c and a quantitative parameter l to a 24-bit pixel
 * These formulas were invented by me to obtain maximum contrast without going out of gamut
 * if the parameters are in the range 0-1
 */
void cl2pix(double* R, double* G, double* B, double c, double l) {
  double L = l*0.61+0.09; //L of L*a*b*
  double angle = TAU/6.0-c*TAU;
  double r = l*0.311+0.125; //~chroma
  double a = sin(angle)*r;
  double b = cos(angle)*r;
  lab2rgb(R,G,B,L,a,b);
}
