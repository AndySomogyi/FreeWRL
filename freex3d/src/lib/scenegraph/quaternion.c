/*


???

*/

/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2009 CRC Canada. (http://www.crc.gc.ca)

    FreeWRL/FreeX3D is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FreeWRL/FreeX3D is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FreeWRL/FreeX3D.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/


#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h"
#include "../opengl/OpenGL_Utils.h"
#include "../main/headers.h"

#include "LinearAlgebra.h"
#include "quaternion.h"

#ifndef MATH_PI
#define MATH_PI 3.14159265358979323846
#endif

/*
 * Quaternion math ported from Perl to C
 * (originally in Quaternion.pm)
 *
 * VRML rotation representation:
 *
 *  axis (x, y, z) and angle (radians), default is unit vector = (0, 0, 1)
 *  and angle = 0 (see VRML97 spec. clauses 4.4.5 'Standard units and
 *  coordinate system', 5.8 'SFRotation and MFRotation')
 *
 * Quaternion representation:
 *
 * q = w + xi + yj + zk or q = (w, v) where w is a scalar and
 * v = (x, y, z) is a vector
 *
 * Quaternion addition:
 *
 * q1 + q2 = (w1 + w2, v1 + v2)
 *         = (w1 + w2) + (x1 + x2)i + (y1 + y2)j + (z1 + z2)k
 *
 * Quaternion multiplication
 *  (let the dot product of v1 and v2 be v1 dp v2):
 *
 * q1q2 = (w1, v1) dp (w2, v2)
 *      = (w1w2 - v1 dp v2, w1v2 + w2v1 + v1 x v2)
 * q1q2 != q2q1 (not commutative)
 *
 *
 * norm(q) = || q || = sqrt(w^2 + x^2 + y^2 + z^2) is q's magnitude
 * normalize a quaternion: q' = q / || q ||
 *
 * conjugate of q = q* = (w, -v)
 * inverse of q = q^-1 = q* / || q ||
 * unit quaternion: || q || = 1, w^2 + x^2 + y^2 + z^2 = 1, q^-1 = q*
 *
 * Identity quaternions: q = (1, (0, 0, 0)) is the multiplication
 *  identity and q = (0, (0, 0, 0)) is the addition identity
 *
 * References:
 *
 * * www.gamedev.net/reference/programming/features/qpowers/
 * * www.gamasutra.com/features/19980703/quaternions_01.htm
 * * mathworld.wolfram.com/
 * * skal.planet-d.net/demo/matrixfaq.htm
 */



/* change matrix rotation to/from a quaternion */
void
matrix_to_quaternion (Quaternion *quat, double *mat) {
	double T, S, X, Y, Z, W;

	/* get the trace of the matrix */
	T = 1 + MAT00 + MAT11 + MAT22;
	/* printf ("T is %f\n",T);*/

	if (T > 0) {
		S = 0.5/sqrt(T);
		W = 0.25 / S;
		/* x =  (m21 - m12) *S*/
		/* y =  (m02 - m20) *s*/
		/* z =  (m10 - m01) *s*/
		X=(MAT12-MAT21)*S;
		Y=(MAT20-MAT02)*S;
		Z=(MAT01-MAT10)*S;
	} else {
		/* If the trace of the matrix is equal to zero then identify*/
		/* which major diagonal element has the greatest value.*/
		/* Depending on this, calculate the following:*/

		if ((MAT00>MAT11)&&(MAT00>MAT22))  {/*  Column 0:*/
			S  = sqrt( 1.0 + MAT00 - MAT11 - MAT22 ) * 2;
			X = 0.25 * S;
			Y = (MAT01 + MAT10) / S;
			Z = (MAT02 + MAT20) / S;
			W = (MAT21 - MAT12) / S;
		} else if ( MAT11>MAT22 ) {/*  Column 1:*/
			S  = sqrt( 1.0 + MAT11 - MAT00 - MAT22) * 2;
			X = (MAT01 + MAT10) / S;
			Y = 0.25 * S;
			Z = (MAT12 + MAT21) / S;
			W = (MAT20 - MAT02) / S;
		} else {/*  Column 2:*/
			S  = sqrt( 1.0 + MAT22 - MAT00 - MAT11) * 2;
			X = (MAT02 + MAT20) / S;
			Y = (MAT12 + MAT21) / S;
			Z = 0.25 * S;
			W = (MAT10 - MAT01) / S;
		}
	}

	/* printf ("Quat x %f y %f z %f w %f\n",X,Y,Z,W);*/
	quat->x = X;
	quat->y = Y;
	quat->z = Z;
	quat->w = W;
}

// http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/
// - says vrml yz is interchanged, but not quite: theirs is RHS too, so x->y, y->z, z->x 
// - I fixed by re-assigning their heading, attitude, bank to our yaw-pitch-roll (ypr)
/** assumes q1 is a normalised quaternion */
void quat2euler0(double *axyz, Quaternion *q1) {
	double heading, attitude, bank, sqx, sqy,sqz;
	double test = q1->x*q1->y + q1->z*q1->w;
	if (test > 0.499) { // singularity at north pole
		heading = 2 * atan2(q1->x,q1->w);
		attitude = MATH_PI/2.0;
		bank = 0;
		axyz[0] = bank;
		axyz[1] = heading;
		axyz[2] = attitude;
		return;
	}
	if (test < -0.499) { // singularity at south pole
		heading = -2 * atan2(q1->x,q1->w);
		attitude = - MATH_PI/2.0;
		bank = 0;
		axyz[0] = bank;
		axyz[1] = heading;
		axyz[2] = attitude;
		return;
	}
    sqx = q1->x*q1->x;
    sqy = q1->y*q1->y;
    sqz = q1->z*q1->z;
    heading = atan2(2*q1->y*q1->w - 2*q1->x*q1->z , 1.0 - 2*sqy - 2*sqz);
	attitude = asin(2*test);
	bank = atan2(2*q1->x*q1->w - 2*q1->y*q1->z , 1.0 - 2*sqx - 2*sqz);
	axyz[0] = bank;
	axyz[1] = heading;
	axyz[2] = attitude;

}
int iprev(int icur, int max){
	int ip = icur - 1;
	if(ip < 0) ip = max;
	return ip;
}
int inext(int icur, int max){
	int ip = icur + 1;
	if(ip > max) ip = 0;
	return ip;
}
void quat2euler(double *rxyz, int iaxis_halfcircle, Quaternion *q){
	// the quaternion to euler formula aren't perfectly symmetrical. 
	// One axis uses an asin()/half-circle instead of atan2(,) with a singularity at +-90 degrees
	// and depending on which axis you want the singularity/half-circle on you would roll axes forward and back in
	// a function like this.
	// In this function we want singularity to show up on the iaxis_halfcircle [0,1, or 2] axis 
	// normally you would pick an axis like x=0 and stick with it
	//returns angle about x, y, and z axes (what some would call pitch, roll, yaw)
	int i0,i1,i2;
	Quaternion q1;
	double axyz[3], q4[4],q14[4];
	quat2double(q4,q);
	i2 = iaxis_halfcircle;
	i1 = iprev(i2,2);
	i0 = iprev(i1,2);
	q14[3] = q4[3];
	q14[0] = q4[i0];
	q14[1] = q4[i1];
	q14[2] = q4[i2]; //half-circle
	double2quat(&q1,q14);
	quat2euler0(axyz,&q1);
	rxyz[i0] = axyz[0];
	rxyz[i1] = axyz[1]; 
	rxyz[i2] = axyz[2]; //half-circle
}
double quaternion_to_pitch0(Quaternion *q)
{
	//pitch around x axis, half circle
	return -asin(2.0*(q->w*q->x - q->y*q->z));
	//return -asin(2.0*(q->w*q->y - q->x*q->z));
}
double quaternion_to_pitch(Quaternion *q)
{
	//pitch around x axis
	double rxyz[3];
	quat2euler(rxyz, 0, q);
	return rxyz[0];
	//return atan2(2.0*(q->w*q->x + q->y*q->z), -q->w*q->w - q->x*q->x + q->y*q->y + q->z*q->z);
	////return atan2(2.0*(q->w*q->z + q->x*q->y), q->w*q->w + q->x*q->x - q->y*q->y - q->z*q->z);
}

double quaternion_to_yaw(Quaternion *q)
{
	//yaw around y axis
	return atan2(2.0*(q->w*q->y + q->x*q->z), q->w*q->w + q->x*q->x - q->y*q->y - q->z*q->z);
	//return atan2(2.0*(q->w*q->z + q->x*q->y), q->w*q->w + q->x*q->x - q->y*q->y - q->z*q->z);
}

double quaternion_to_roll(Quaternion *q)
{
	//roll around z axis
	return -atan2(2.0*(q->w*q->z + q->y*q->x), -q->w*q->w + q->x*q->x + q->y*q->y - q->z*q->z);
	//return -atan2(2.0*(q->w*q->x + q->y*q->z), -q->w*q->w + q->x*q->x + q->y*q->y - q->z*q->z);
}
#define DEGREES_PER_RADIAN (double)57.2957795130823208768
void quaternion_to_euler(double *ypr, Quaternion *q){
	Quaternion tmp, de;
	static int irev = 1;
	static double isgn = -1.0;
	quaternion_set(&tmp,q);

	ypr[0] = quaternion_to_yaw(&tmp);
	vrmlrot_to_quaternion(&de,0.0,1.0,0.0,isgn*ypr[0]);
	if(irev) quaternion_multiply(&tmp,&tmp,&de);
	else quaternion_multiply(&tmp,&de,&tmp);
	quaternion_normalize(&tmp);

	ypr[1] = quaternion_to_pitch(&tmp);
	vrmlrot_to_quaternion(&de,1.0,0.0,0.0,isgn*ypr[1]);
	if(irev) quaternion_multiply(&tmp,&tmp,&de);
	else quaternion_multiply(&tmp,&de,&tmp);
	quaternion_normalize(&tmp);


	ypr[2] = quaternion_to_roll(q);
	vrmlrot_to_quaternion(&de,0.0,0.0,1.0,isgn*ypr[2]);
	if(irev) quaternion_multiply(&tmp,&tmp,&de);
	else quaternion_multiply(&tmp,&de,&tmp); //take out roll
	quaternion_normalize(&tmp);

	if(1){
		printf("yaw %lf pitch %lf roll %lf\n",ypr[0]*DEGREES_PER_RADIAN,ypr[1]*DEGREES_PER_RADIAN,ypr[2]*DEGREES_PER_RADIAN);
		quaternion_print(&tmp,"in quaternion_to_euler: should be no rotations");
	}
}

void test_euler(){
	double aval,bval,dval,ypr[3];
	Quaternion q1;
	aval = -MATH_PI/3.0;

	vrmlrot_to_quaternion(&q1,1.0,0.0,0.0,aval);
	quaternion_normalize(&q1);
	bval = quaternion_to_pitch(&q1);
	dval = aval - bval;
	if(fabs(dval) > .00001) 
		printf("ouch quaternion_to_pitch off by %lf\n",dval);
	quat2euler(ypr,0,&q1);
	printf("pitch ypr= %lf %lf %lf\n",ypr[0],ypr[1],ypr[2]);

	bval = quaternion_to_pitch0(&q1);
	dval = aval - bval;
	if(fabs(dval) > .00001) 
		printf("ouch quaternion_to_pitch0 off by %lf\n",dval);
	quat2euler(ypr,0,&q1);
	printf("x pry = %lf %lf %lf\n",ypr[0],ypr[1],ypr[2]);

	vrmlrot_to_quaternion(&q1,0.0,1.0,0.0,aval);
	quaternion_normalize(&q1);
	bval = quaternion_to_yaw(&q1);
	dval = aval - bval;
	if(fabs(dval) > .00001) 
		printf("ouch quaternion_to_yaw off by %lf\n",dval);
	quat2euler(ypr,0,&q1);
	printf("y pry= %lf %lf %lf\n",ypr[0],ypr[1],ypr[2]);

	vrmlrot_to_quaternion(&q1,0.0,0.0,1.0,aval);
	quaternion_normalize(&q1);
	bval = quaternion_to_roll(&q1);
	dval = aval - bval;
	if(fabs(dval) > .00001) 
		printf("ouch quaternion_to_roll off by %lf\n",dval);
	quat2euler(ypr,0,&q1);
	printf("z pry= %lf %lf %lf\n",ypr[0],ypr[1],ypr[2]);
	printf("\n");
}

/* http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/index.htm */
/* note that the above web site uses "mathematicians" not "opengl" method of matrix id */
void
quaternion_to_matrix (double *mat, Quaternion *q) {
	double sqw, sqx, sqy, sqz, tmp1, tmp2;
	double invs;

	/* assumes matrix is identity, or identity + transform */
	/* assumes matrix in OpenGL format */
	sqw = q->w*q->w;
	sqx = q->x*q->x;
	sqy = q->y*q->y;
	sqz = q->z*q->z;
	
	/* inverse square length - if the quat is not normalized; but lets do
	   this anyway */
	invs = 1.0 / (sqx + sqy + sqz + sqw);

	/* scale */
	MATHEMATICS_MAT00 =  (sqx - sqy - sqz + sqw)*invs; /*  since sqw + sqx + sqy + sqz =1*/
	MATHEMATICS_MAT11 = (-sqx + sqy - sqz + sqw)*invs;
	MATHEMATICS_MAT22 = (-sqx - sqy + sqz + sqw)*invs;

	tmp1 = q->x*q->y;
	tmp2 = q->z*q->w;
	MATHEMATICS_MAT10 = 2.0 * (tmp1 + tmp2) * invs; /* m[1][0]*/
	MATHEMATICS_MAT01 = 2.0 * (tmp1 - tmp2) * invs; /* m[0][1]*/

	tmp1 = q->x*q->z;
	tmp2 = q->y*q->w;
	MATHEMATICS_MAT20 = 2.0 * (tmp1 - tmp2) * invs; /* m[2][0]*/
	MATHEMATICS_MAT02 = 2.0 * (tmp1 + tmp2) * invs; /* m[0][2]*/
	tmp1 = q->y*q->z;
	tmp2 = q->x*q->w;
	MATHEMATICS_MAT21 = 2.0 * (tmp1 + tmp2) * invs; /* m[2][1]*/
	MATHEMATICS_MAT12 = 2.0 * (tmp1 - tmp2) * invs; /* m[1][2]*/
}

/*
 * VRML rotation (axis, angle) to quaternion (q = (w, v)):
 *
 * To simplify the math, the rotation vector needs to be normalized.
 *
 * q.w = cos(angle / 2);
 * q.x = (axis.x / || axis ||) * sin(angle / 2)
 * q.y = (axis.y / || axis ||) * sin(angle / 2)
 * q.z = (axis.z / || axis ||) * sin(angle / 2)
 *
 * Normalize quaternion: q /= ||q ||
 */

void
vrmlrot_to_quaternion(Quaternion *quat, const double x, const double y, const double z, const double a)
{
	double s;
	double scale = sqrt((x * x) + (y * y) + (z * z));

	/* no rotation - use (multiplication ???) identity quaternion */
	if (APPROX(scale, 0)) {
		quat->w = 1;
		quat->x = 0;
		quat->y = 0;
		quat->z = 0;

	} else {
		s = sin(a/2);
		/* normalize rotation axis to convert VRML rotation to quaternion */
		quat->w = cos(a / 2);
		quat->x = s * (x / scale);
		quat->y = s * (y / scale);
		quat->z = s * (z / scale);
		quaternion_normalize(quat);
	}
}

/*
 * Quaternion (q = (w, v)) to VRML rotation (axis, angle):
 *
 * angle = 2 * acos(q.w)
 * axis.x = q.x / scale
 * axis.y = q.y / scale
 * axis.z = q.z / scale
 *
 * unless scale = 0, in which case, we'll use the default VRML
 * rotation
 *
 * One can use either scale = sqrt(q.x^2 + q.y^2 + q.z^2) or
 * scale = sin(acos(q.w)).
 * Since we are using unit quaternions, 1 = w^2 + x^2 + y^2 + z^2.
 * Also, acos(x) = asin(sqrt(1 - x^2)) (for x >= 0, but since we don't
 * actually compute asin(sqrt(1 - x^2)) let's not worry about it).
 * scale = sin(acos(q.w)) = sin(asin(sqrt(1 - q.w^2)))
 *       = sqrt(1 - q.w^2) = sqrt(q.x^2 + q.y^2 + q.z^2)
 */
void
quaternion_to_vrmlrot(const Quaternion *quat, double *x, double *y, double *z, double *a)
{
	double scale = sqrt(VECSQ(*quat));
	if (APPROX(scale, 0)) {
		*x = 0;
		*y = 0;
		*z = 1;
		*a = 0;
	} else {
		*x = quat->x / scale;
		*y = quat->y / scale;
		*z = quat->z / scale;
		*a = 2 * acos(quat->w);
	}
}

void
quaternion_conjugate(Quaternion *quat)
{
	quat->x *= -1;
	quat->y *= -1;
	quat->z *= -1;
}

void
quaternion_inverse(Quaternion *ret, const Quaternion *quat)
{
/* 	double n = norm(quat); */

	quaternion_set(ret, quat);
	quaternion_conjugate(ret);

	/* unit quaternion, so take conjugate */
	quaternion_normalize(ret);
 	/* printf("Quaternion inverse: ret = {%f, %f, %f, %f}, quat = {%f, %f, %f, %f}\n",
 	 	   ret->w, ret->x, ret->y, ret->z, quat->w, quat->x, quat->y, quat->z); */
}

double
quaternion_norm(const Quaternion *quat)
{
	return(sqrt(
				quat->w * quat->w +
				quat->x * quat->x +
				quat->y * quat->y +
				quat->z * quat->z
				));
}

void
quaternion_normalize(Quaternion *quat)
{
	double n = quaternion_norm(quat);
	if (APPROX(n, 1)) {
		return;
	}
	quat->w /= n;
	quat->x /= n;
	quat->y /= n;
	quat->z /= n;
}

void quaternion_add(Quaternion *ret, const Quaternion *q1, const Quaternion *q2) {
	double t1[3];
	double t2[3];

	/* scale_v3f (Q(*q2)[3], (v3f *) q1, &t1); */
	t1[0] = q2->w * q1->x;
	t1[1] = q2->w * q1->y;
	t1[2] = q2->w * q1->z;

	/* scale_v3f (Q(*q1)[3], (v3f *) q2, &t2); */
	t2[0] = q1->w * q2->x;
	t2[1] = q1->w * q2->y;
	t2[2] = q1->w * q2->z;

	/* add_v3f (&t1, &t2, &t1); */
	t1[0] = t1[0] + t2[0];
	t1[1] = t1[1] + t2[1];
	t1[2] = t1[2] + t2[2];

	/* cross_v3f ((v3f *) q2, (v3f *) q1, &t2); */
	t2[0] = ( q2->y * q1->z - q2->z * q1->y );
	t2[1] = ( q2->z * q1->x - q2->x * q1->z );
	t2[2] = ( q2->x * q1->y - q2->y * q1->x );

	/* add_v3f (&t1, &t2, (v3f *) dest); */
	ret->x = t1[0] + t2[0];
	ret->y = t1[1] + t2[1];
	ret->z = t1[2] + t2[2];

	/* Q(*dest)[3] = Q(*q1)[3] * Q(*q2)[3] - inner_v3f((v3f *) q1, (v3f *) q2); */
	ret->w = q1->w * q2->w - ( q1->x * q2->x + q1->y * q2->y + q1->z * q2->z );
}

void
quaternion_multiply(Quaternion *ret, const Quaternion *q1, const Quaternion *q2)
{
	Quaternion qa, qb;
	quaternion_set(&qa,q1);
	quaternion_set(&qb,q2);
	ret->w = (qa.w * qb.w) - (qa.x * qb.x) - (qa.y * qb.y) - (qa.z * qb.z);
	ret->x = (qa.w * qb.x) + (qa.x * qb.w) + (qa.y * qb.z) - (qa.z * qb.y);
	ret->y = (qa.w * qb.y) + (qa.y * qb.w) - (qa.x * qb.z) + (qa.z * qb.x);
	ret->z = (qa.w * qb.z) + (qa.z * qb.w) + (qa.x * qb.y) - (qa.y * qb.x);
/* 	printf("Quaternion multiply: ret = {%f, %f, %f, %f}, q1 = {%f, %f, %f, %f}, q2 = {%f, %f, %f, %f}\n", ret->w, ret->x, ret->y, ret->z, q1->w, q1->x, q1->y, q1->z, q2->w, q2->x, q2->y, q2->z); */
}

void
quaternion_scalar_multiply(Quaternion *quat, const double s)
{
	quat->w *= s;
	quat->x *= s;
	quat->y *= s;
	quat->z *= s;
}

/*
 * Rotate vector v by unit quaternion q:
 *
 * v' = q q_v q^-1, where q_v = [0, v]
 */
void
quaternion_rotation(struct point_XYZ *ret, const Quaternion *quat, const struct point_XYZ *v)
{
	Quaternion q_v, q_i, q_r1, q_r2;

	q_v.w = 0.0;
	q_v.x = v->x;
	q_v.y = v->y;
	q_v.z = v->z;
	quaternion_inverse(&q_i, quat);
	quaternion_multiply(&q_r1, &q_v, &q_i);
	quaternion_multiply(&q_r2, quat, &q_r1);

	ret->x = q_r2.x;
	ret->y = q_r2.y;
	ret->z = q_r2.z;
 	/* printf("Quaternion rotation: ret = {%f, %f, %f}, quat = {%f, %f, %f, %f}, v = {%f, %f, %f}\n", ret->x, ret->y, ret->z, quat->w, quat->x, quat->y, quat->z, v->x, v->y, v->z); */
}
void
quaternion_rotationd(double *ret, Quaternion *quat, double *v){
	struct point_XYZ rp,vp;
	double2pointxyz(&vp,v);
	quaternion_rotation(&rp,quat,&vp);
	pointxyz2double(ret,&rp);
}

void
quaternion_togl(Quaternion *quat)
{
	if (APPROX(fabs(quat->w), 1)) { return; }

	if (quat->w > 1) { quaternion_normalize(quat); }

	/* get the angle, but turn us around 180 degrees */
	/* printf ("togl: setting rotation %f %f %f %f\n",quat->w,quat->x,quat->y,quat->z);*/
	FW_GL_ROTATE_RADIANS(2.0 * acos(quat->w), quat->x, quat->y, quat->z);
}

void
quaternion_set(Quaternion *ret, const Quaternion *quat)
{
	ret->w = quat->w;
	ret->x = quat->x;
	ret->y = quat->y;
	ret->z = quat->z;
}

/*
 * Code from www.gamasutra.com/features/19980703/quaternions_01.htm,
 * Listing 5.
 *
 * SLERP(p, q, t) = [p sin((1 - t)a) + q sin(ta)] / sin(a)
 *
 * where a is the arc angle, quaternions pq = cos(q) and 0 <= t <= 1
 */
void
quaternion_slerp(Quaternion *ret, const Quaternion *q1, const Quaternion *q2, const double t)
{
	double omega, cosom, sinom, scale0, scale1, q2_array[4];

	cosom =
		q1->x * q2->x +
		q1->y * q2->y +
		q1->z * q2->z +
		q1->w * q2->w;

	if (cosom < 0.0) {
		cosom = -cosom;
		q2_array[0] = -q2->x;
		q2_array[1] = -q2->y;
		q2_array[2] = -q2->z;
		q2_array[3] = -q2->w;
	} else {
		q2_array[0] = q2->x;
		q2_array[1] = q2->y;
		q2_array[2] = q2->z;
		q2_array[3] = q2->w;
	}

	/* calculate coefficients */
	if ((1.0 - cosom) > DELTA) {
		/* standard case (SLERP) */
		omega = acos(cosom);
		sinom = sin(omega);
		scale0 = sin((1.0 - t) * omega) / sinom;
		scale1 = sin(t * omega) / sinom;
	} else {
		/* q1 & q2 are very close, so do linear interpolation */
		scale0 = 1.0 - t;
		scale1 = t;
	}
	ret->x = scale0 * q1->x + scale1 * q2_array[0];
	ret->y = scale0 * q1->y + scale1 * q2_array[1];
	ret->z = scale0 * q1->z + scale1 * q2_array[2];
	ret->w = scale0 * q1->w + scale1 * q2_array[3];
}
void quaternion_print(const Quaternion *quat, char* description ){
	printf("quat %s",description);
	printf(" xyzw=[%lf %lf %lf %lf]\n",quat->x,quat->y,quat->z,quat->w);
}
void double2quat(Quaternion *quat, double *quat4){
	quat->x = quat4[0];
	quat->y = quat4[1];
	quat->z = quat4[2];
	quat->w = quat4[3];
}
void quat2double(double *quat4,Quaternion *quat){
	quat4[0] = quat->x;
	quat4[1] = quat->y;
	quat4[2] = quat->z;
	quat4[3] = quat->w;
}

void vrmlrot_normalize(float *ret)
{
	float s = ret[0]*ret[0] + ret[1]*ret[1] + ret[2]*ret[2];
	s = (float) sqrt(s);
	if( s != 0.0f )
	{
		s = 1.0f/s;
		ret[0] *= s;
		ret[1] *= s;
		ret[2] *= s;
	}
	else
	{
		ret[2] = 1.0f;
	}
	ret[3] = (float) fmod(ret[3],MATH_PI); //acos(-1.0));
}

void vrmlrot_multiply(float* ret, float *a, float *b) 
{
   ret[0] = (b[0] * b[3]) + (a[0] * a[3]);
   ret[1] = (b[1] * b[3]) + (a[1] * a[3]);
   ret[2] = (b[2] * b[3]) + (a[2] * a[3]);
   ret[3] = (float) sqrt(ret[0]*ret[0] + ret[1]*ret[1] + ret[2]*ret[2]);
   if (ret[3] == 0.0f) 
   {
	   ret[0] = 1.0f;
	   ret[1]=ret[2] = 0.0f;
	   return;
   }
   ret[0] = ret[0]/ret[3];
   ret[1] = ret[1]/ret[3];
   ret[2] = ret[2]/ret[3];
   //vrmlrot_normalize(ret);
}

