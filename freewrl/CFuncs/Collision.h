#ifndef COLLISIONH
#define COLLISIONH


#include <math.h>

#include "Structs.h"

/* Collision detection results structure*/
struct sCollisionInfo {
    struct pt Offset;
    int Count;
    double Maximum2; /*squared. so we only need to root once */
};

typedef int prflags;
#define PR_DOUBLESIDED 0x01
#define PR_FRONTFACING 0x02 //overrides effect of doublesided.
#define PR_BACKFACING 0x04 //overrides effect of doublesided, all normals are reversed.


/*accumulator function, for displacements. */
void accumulate_disp(struct sCollisionInfo* ci, struct pt add);

//double closest_point_of_segment_to_y_axis(struct pt p1, struct pt p2);


/* [p1,p2[ is segment,  q1,q2 defines line */
/* ignores y coord. eg intersection is done on projection of segment and line on the y plane */
/* nowtice point p2 is NOT included, (for simplification elsewhere) */
//int intersect_segment_with_line_on_yplane(struct pt* pk, struct pt p1, struct pt p2, struct pt q1, struct pt q2);


/*projects a point on the y="y" plane, in the direction of n. *
  n probably needs to be normal. */
//struct pt project_on_yplane(struct pt p1, struct pt n,double y);

/*projects a point on the plane tangent to the surface of the cylinder at point kn (the prolonged normal) 
  , in the direction of n. 
  n probably needs to be normal. */
//struct pt project_on_cylindersurface_plane(struct pt p, struct pt n,double r);
/*projects a point on the surface of the cylinder, in the direction of n. 
  returns TRUE if exists.
   */
//int project_on_cylindersurface(struct pt* res, struct pt p, struct pt n,double r);


/*finds the intersection of the segment(pp1,pp2) with a cylinder on the y axis.
  returns the 0,1 or 2 values in the range [0..1]
 */
//int getk_intersect_segment_with_ycylinder(double* k1, double* k2, double r, struct pt pp1, struct pt pp2);

/*finds the intersection of the line pp1 + k n with a cylinder on the y axis.
  returns the 0,1 or 2 values.
 */
//int getk_intersect_line_with_ycylinder(double* k1, double* k2, double r, struct pt pp1, struct pt n);

struct pt weighted_sum(struct pt p1, struct pt p2, double k);

/*feed a poly, and stats of a cylinder, it returns the displacement in the direction of the
  normal of the poly that is needed for them not to intersect any more.*/
struct pt get_poly_normal_disp(double y1, double y2, double r, struct pt* p, int num, struct pt n);

/*feed a poly, and stats of a cylinder, it returns the vertical displacement that is needed for them not to intersect any more,
  if this displacement is less than the height of the cylinder (y2-y1).*/
struct pt get_poly_step_disp(double y1, double y2, double r, struct pt* p, int num, struct pt n);

/*feed a poly, and stats of a cylinder, it returns the displacement in the direction of the
  normal of the poly that is needed for them not to intersect any more, or vertically if contact point below ystep*/
struct pt get_poly_disp(double y1, double y2, double ystep, double r, struct pt* p, int num, struct pt n);

/*feed a box (a corner, and the three vertice sides) and the stats of a cylinder, it returns the
  displacement of the box that is needed for them not to intersect any more. */
struct pt box_disp(double y1, double y2, double ystep, double r,struct pt p0, struct pt i, struct pt j, struct pt k);

/*fast test to see if a box intersects a y-cylinder.
 * gives false positives */
int fast_ycylinder_box_intersect(double y1, double y2, double r,struct pt pcenter, double xs, double ys, double zs);
/* fast test to see if a sphere intersects a y-cylinder. 
   specify sphere center, and a point on it's surface 
  gives false positives. */
int fast_ycylinder_sphere_intersect(double y1, double y2, double r,struct pt pcenter, struct pt psurface);

void make_orthogonal_vector_space(struct pt* i, struct pt* j, struct pt n);

/*feed a poly, and stats of a cylinder, it returns the displacement in the direction of the
  normal of the poly that is needed for them not to intersect any more.*/
struct pt get_line_normal_disp(double y1, double y2, double r, struct pt p1, struct pt p2, struct pt n);

struct pt get_line_step_disp(double y1, double y2, double r, struct pt p1, struct pt p2, struct pt n);

struct pt get_line_disp(double y1, double y2, double ystep, double r, struct pt p1, struct pt p2, struct pt n);

struct pt get_point_normal_disp(double y1, double y2, double r, struct pt p1, struct pt n);

struct pt get_point_disp(double y1, double y2, double ystep, double r, struct pt p1, struct pt n);

/*algorithm is approximative */
/*basically, it does collision with a triangle on a plane that passes through the origin.*/
struct pt cone_disp(double y1, double y2, double ydisp, double r, struct pt base, struct pt top, double baseradius);

/*algorithm is approximative */
/*basically, it does collision with a rectangle on a plane that passes through the origin.*/
struct pt cylinder_disp(double y1, double y2, double ydisp, double r, struct pt base, struct pt top, double baseradius);

struct pt polyrep_disp(double y1, double y2, double ydisp, double r, struct VRML_PolyRep pr, GLdouble* mat, prflags flags);

struct pt planar_polyrep_disp(double y1, double y2, double ydisp, double r, struct VRML_PolyRep pr, GLdouble* mat, prflags flags, struct pt n);

struct pt elevationgrid_disp( double y1, double y2, double ydisp, double r, struct VRML_PolyRep pr, 
			      int xdim, int zdim, double xs, double zs, GLdouble* mat, prflags flags);

#ifdef DEBUGPTS
void printpolyrep(struct VRML_PolyRep pr, int npoints);

void printmatrix(GLdouble* mat);
#endif

#endif




