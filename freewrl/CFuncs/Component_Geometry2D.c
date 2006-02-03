/*******************************************************************
 Copyright (C) 2005, 2006 John Stewart, Ayla Khan, Sarah Dumoulin, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*******************************************************************

	X3D Geometry2D  Component

*********************************************************************/

#include "headers.h"
#include "Collision.h"

#define SEGMENTS_PER_CIRCLE 36
#define PIE 10
#define CHORD 20
#define NONE 30

void *createLines (float start, float end, float radius, int closed, int *size);
void createDisk2D (struct X3D_Disk2D *node);
void createTriangleSet2D (struct X3D_TriangleSet2D *node);

void render_Arc2D (struct X3D_Arc2D *node) {
        if (node->_ichange != node->_change) {
                /*  have to regen the shape*/
                node->_ichange = node->_change;
		
		FREE_IF_NZ (node->__points);
		node->__numPoints = 0;
		node->__points = createLines (node->startAngle,
			node->endAngle, node->radius, NONE, &node->__numPoints);
	}

	if (node->__numPoints>0) {	
	        LIGHTING_OFF
	        DISABLE_CULL_FACE
		glColor3f (1.0, 1.0, 1.0);

		glDisableClientState (GL_NORMAL_ARRAY);
		glVertexPointer (2,GL_FLOAT,0,(GLfloat *)node->__points);
        	glDrawArrays (GL_LINE_STRIP, 0, node->__numPoints);
		glEnableClientState (GL_NORMAL_ARRAY);
	}
}

void render_ArcClose2D (struct X3D_ArcClose2D *node) {
	STRLEN xx;
	char *ct;

        if (node->_ichange != node->_change) {
                /*  have to regen the shape*/
                node->_ichange = node->_change;
		
		FREE_IF_NZ (node->__points);
		node->__numPoints = 0;

		ct = SvPV(node->closureType,xx);

		if (strncmp(ct,"PIE",xx) == 0) {
			node->__points = createLines (node->startAngle,
				node->endAngle, node->radius, PIE, &node->__numPoints);
		} else if (strncmp(ct,"CHORD",xx) == 0) {
			node->__points = createLines (node->startAngle,
				node->endAngle, node->radius, CHORD, &node->__numPoints);
		} else {
			printf ("ArcClose2D, closureType %s invalid\n",node->closureType);
		}
	}


	if (node->__numPoints>0) {	
        	LIGHTING_OFF
        	DISABLE_CULL_FACE
		glColor3f (1.0, 1.0, 1.0);

		glDisableClientState (GL_NORMAL_ARRAY);
		glVertexPointer (2,GL_FLOAT,0,(GLfloat *)node->__points);
        	glDrawArrays (GL_LINE_STRIP, 0, node->__numPoints);
		glEnableClientState (GL_NORMAL_ARRAY);
	}
}

void render_Circle2D (struct X3D_Circle2D *node) {
        if (node->_ichange != node->_change) {
                /*  have to regen the shape*/
                node->_ichange = node->_change;
		
		FREE_IF_NZ (node->__points);
		node->__numPoints = 0;
		node->__points = createLines (0.0, 0.0,
			node->radius, NONE, &node->__numPoints);
	}

	if (node->__numPoints>0) {	
	        LIGHTING_OFF
	        DISABLE_CULL_FACE
		glColor3f (1.0, 1.0, 1.0);

		glDisableClientState (GL_NORMAL_ARRAY);
		glVertexPointer (2,GL_FLOAT,0,(GLfloat *)node->__points);
        	glDrawArrays (GL_LINE_STRIP, 0, node->__numPoints);
		glEnableClientState (GL_NORMAL_ARRAY);
	}
}

void render_Polyline2D (struct X3D_Polyline2D *node){
	if (node->lineSegments.n>0) {
	        LIGHTING_OFF
	        DISABLE_CULL_FACE
		glColor3f (1.0, 1.0, 1.0);

		glDisableClientState (GL_NORMAL_ARRAY);
		glVertexPointer (2,GL_FLOAT,0,(GLfloat *)node->lineSegments.p);
        	glDrawArrays (GL_LINE_STRIP, 0, node->lineSegments.n);
		glEnableClientState (GL_NORMAL_ARRAY);
	}
}

void render_Polypoint2D (struct X3D_Polypoint2D *node){
	if (node->point.n>0) {
	        LIGHTING_OFF
	        DISABLE_CULL_FACE
		glColor3f (1.0, 1.0, 1.0);

		glDisableClientState (GL_NORMAL_ARRAY);
		glVertexPointer (2,GL_FLOAT,0,(GLfloat *)node->point.p);
        	glDrawArrays (GL_POINTS, 0, node->point.n);
		glEnableClientState (GL_NORMAL_ARRAY);
	}
}

void render_Disk2D (struct X3D_Disk2D *node){
        if (node->_ichange != node->_change) {
                /*  have to regen the shape*/
                node->_ichange = node->_change;
		createDisk2D (node);
	}

	if (node->__numPoints>0) {	
		CULL_FACE(node->solid)

		textureDraw_start(NULL,(GLfloat *)node->__texCoords);
		glVertexPointer (2,GL_FLOAT,0,(GLfloat *)node->__points);
		glDisableClientState (GL_NORMAL_ARRAY);
		glNormal3f (0.0, 0.0, 1.0);

		/* do the array drawing; sides are simple 0-1-2-3, 4-5-6-7, etc quads */
		if (node->__simpleDisk) glDrawArrays (GL_TRIANGLE_FAN, 0, node->__numPoints);
		else 			glDrawArrays (GL_QUAD_STRIP, 0, node->__numPoints);

		textureDraw_end();
		glEnableClientState (GL_NORMAL_ARRAY);
	}
}

void render_TriangleSet2D (struct X3D_TriangleSet2D *node){
        if (node->_ichange != node->_change) {
                /*  have to regen the shape*/
                node->_ichange = node->_change;
		createTriangleSet2D (node);
	}

	if (node->vertices.n>0) {	
		CULL_FACE(node->solid)

		textureDraw_start(NULL,(GLfloat *)node->__texCoords);
		glVertexPointer (2,GL_FLOAT,0,(GLfloat *)node->vertices.p);
		glDisableClientState (GL_NORMAL_ARRAY);
		glNormal3f (0.0, 0.0, 1.0);

		glDrawArrays (GL_TRIANGLES, 0, node->vertices.n);

		textureDraw_end();
		glEnableClientState (GL_NORMAL_ARRAY);
	}
}

/* this code is remarkably like Box, but with a zero z axis. */
void render_Rectangle2D (struct X3D_Rectangle2D *node) {
	extern GLfloat boxtex[];		/*  in CFuncs/statics.c*/
	extern GLfloat boxnorms[];		/*  in CFuncs/statics.c*/
	float *pt;
	float x = ((node->size).c[0])/2;
	float y = ((node->size).c[1])/2;

	/* test for <0 of sides */
	if ((x < 0) || (y < 0)) return;

	/* for BoundingBox calculations */
	setExtent(x,y,0.0,(struct X3D_Box *)node);


	if (node->_ichange != node->_change) {
		/*  have to regen the shape*/

		node->_ichange = node->_change;

		/*  malloc memory (if possible)*/
		if (!node->__points) node->__points = malloc (sizeof(struct SFColor)*(4));
		if (!node->__points) {
			printf ("can not malloc memory for Rectangle2D points\n");
			return;
		}

		/*  now, create points; 4 points per face.*/
		pt = (float *) node->__points;
		/*  front*/
		*pt++ =  x; *pt++ =  y; *pt++ =  0.0; *pt++ = -x; *pt++ =  y; *pt++ =  0.0;
		*pt++ = -x; *pt++ = -y; *pt++ =  0.0; *pt++ =  x; *pt++ = -y; *pt++ =  0.0;
	}


	CULL_FACE(node->solid)

	/*  Draw it; assume VERTEX and NORMALS already defined.*/
	textureDraw_start(NULL,boxtex);
	glVertexPointer (3,GL_FLOAT,0,(GLfloat *)node->__points);
	glDisableClientState (GL_NORMAL_ARRAY);
	glNormal3f (0.0, 0.0, 1.0);

	/* do the array drawing; sides are simple 0-1-2-3, 4-5-6-7, etc quads */
	glDrawArrays (GL_QUADS, 0, 4);
	textureDraw_end();
	glEnableClientState (GL_NORMAL_ARRAY);
}


void *createLines (float start, float end, float radius, int closed, int *size) {
	int i;
	int isCircle;
	int numPoints;
	GLfloat tmp;
	GLfloat *points;
	GLfloat *fp;
	int arcpoints;

	*size = 0;

	/* is this a circle? */
	isCircle =  APPROX(start,end);

	/* bounds check, and sort values */
	if ((start < PI*2.0) || (start > PI*2.0)) start = 0;
	if ((end < PI*2.0) || (end > PI*2.0)) end = PI/2;
	if (radius<0.0) radius = 1.0;

	if (end > start) {
		tmp = start;
		start = end;
		end = tmp;
	}
		

	if (isCircle) {
		numPoints = SEGMENTS_PER_CIRCLE;
		closed = NONE; /* this is a circle, CHORD, PIE dont mean anything now */
	} else {
		numPoints = ((float) SEGMENTS_PER_CIRCLE * (start-end)/(PI*2.0));
		if (numPoints>SEGMENTS_PER_CIRCLE) numPoints=SEGMENTS_PER_CIRCLE;
	}

	/* we always have to draw the line - we have a line strip, and we calculate
	   the beginning points; we have also to calculate the ending point. */
	numPoints++;
	arcpoints = numPoints;

	/* closure type */
	if (closed == CHORD) numPoints++;
	if (closed == PIE) numPoints+=2;

	points = malloc (sizeof(float)*numPoints*2);
	fp = points;

	for (i=0; i<arcpoints; i++) {
		*fp = -radius * sinf((PI * 2.0 * (float)i)/((float)SEGMENTS_PER_CIRCLE));	
		fp++;
		*fp = radius * cosf((PI * 2.0 * (float)i)/((float)SEGMENTS_PER_CIRCLE));	
		fp++;
	}

	/* do we have to draw any pies, cords, etc, etc? */
	if (closed == CHORD) {
		/* loop back to origin */
		*fp = -radius * sinf(0.0/((float)SEGMENTS_PER_CIRCLE));	
		fp++;
		*fp = radius * cosf(0.0/((float)SEGMENTS_PER_CIRCLE));	
		fp++;
	} else if (closed == PIE) {
		/* go to origin */
		*fp = 0.0; fp++; *fp=0.0; fp++; 
		*fp = -radius * sinf(0.0/((float)SEGMENTS_PER_CIRCLE));	
		fp++;
		*fp = radius * cosf(0.0/((float)SEGMENTS_PER_CIRCLE));	
		fp++;
	}

		
	*size = numPoints;
	return (void *)points;
}


void createDisk2D (struct X3D_Disk2D *node) {
	float rad;
	GLfloat *fp;
	GLfloat *tp;
	int i;
	GLfloat id;
	GLfloat od;

	FREE_IF_NZ (node->__points);
	FREE_IF_NZ (node->__texCoords);
	node->__numPoints = 0;

	/* bounds checking */
	node-> __simpleDisk = FALSE;
	if (node->innerRadius<0) return;
	if (node->outerRadius<0) return;

	/* is this a simple disc ? */
	if ((APPROX (node->innerRadius, 0.0)) || (APPROX(node->innerRadius,node->outerRadius))) {
		node->__simpleDisk = TRUE;
	}

	/* is this a simple disk, or one with an inner circle cut out? */
	if (node->__simpleDisk) {
		node->__numPoints = SEGMENTS_PER_CIRCLE+2;
		fp = node->__points = malloc (sizeof(GLfloat) * 2 * (node->__numPoints));
		tp = node->__texCoords = malloc (sizeof(GLfloat) * 2 * (node->__numPoints));

		/* initial TriangleFan point */
		*fp = 0.0; fp++; *fp = 0.0; fp++;
		*tp = 0.5; tp++; *tp = 0.5; tp++;
		id = 2.0;

		for (i=SEGMENTS_PER_CIRCLE; i >= 0; i--) {
			*fp = node->outerRadius * sinf((PI * 2.0 * (float)i)/((float)SEGMENTS_PER_CIRCLE));	fp++;
			*fp = node->outerRadius * cosf((PI * 2.0 * (float)i)/((float)SEGMENTS_PER_CIRCLE));	fp++;
			*tp = 0.5 + (sinf((PI * 2.0 * (float)i)/((float)SEGMENTS_PER_CIRCLE))/id);	tp++;
			*tp = 0.5 + (cosf((PI * 2.0 * (float)i)/((float)SEGMENTS_PER_CIRCLE))/id);	tp++;
		}
	} else {
		node->__numPoints = (SEGMENTS_PER_CIRCLE+1) * 2;
		fp = node->__points = malloc (sizeof(GLfloat) * 2 * node->__numPoints);
		tp = node->__texCoords = malloc (sizeof(GLfloat) * 2 * node->__numPoints);


		/* texture scaling params */
		od = 2.0;
		id = node->outerRadius * 2.0 / node->innerRadius;

		for (i=SEGMENTS_PER_CIRCLE; i >= 0; i--) {
			*fp = node->innerRadius * sinf((PI * 2.0 * (float)i)/((float)SEGMENTS_PER_CIRCLE));	fp++;
			*fp = node->innerRadius * cosf((PI * 2.0 * (float)i)/((float)SEGMENTS_PER_CIRCLE));	fp++;
			*fp = node->outerRadius * sinf((PI * 2.0 * (float)i)/((float)SEGMENTS_PER_CIRCLE));	fp++;
			*fp = node->outerRadius * cosf((PI * 2.0 * (float)i)/((float)SEGMENTS_PER_CIRCLE));	fp++;
			*tp = 0.5 + (sinf((PI * 2.0 * (float)i)/((float)SEGMENTS_PER_CIRCLE))/id);	tp++;
			*tp = 0.5 + (cosf((PI * 2.0 * (float)i)/((float)SEGMENTS_PER_CIRCLE))/id);	tp++;
			*tp = 0.5 + (sinf((PI * 2.0 * (float)i)/((float)SEGMENTS_PER_CIRCLE))/od);	tp++;
			*tp = 0.5 + (cosf((PI * 2.0 * (float)i)/((float)SEGMENTS_PER_CIRCLE))/od);	tp++;
		}
	}
}

void createTriangleSet2D (struct X3D_TriangleSet2D *node) {
	GLfloat maxX, minX;
	GLfloat maxY, minY;
	GLfloat Ssize, Tsize;
	int i;
	GLfloat *fp;


	/* do we have vertex counts in sets of 3? */
	if ((node->vertices.n %3) != 0) {
		printf ("TriangleSet2D, have incorrect vertex count, %d\n",node->vertices.n);
		node->vertices.n -= node->vertices.n % 3;
	}
	FREE_IF_NZ (node->__texCoords);
	node->__texCoords = fp = malloc (sizeof (GLfloat) * node->vertices.n * 2);

	/* find min/max values for X and Y axes */
	minY = minX = 99999999.0;
	maxY = maxX = -9999999.0;
	for (i=0; i<node->vertices.n; i++) {
		if (node->vertices.p[i].c[0] < minX) minX = node->vertices.p[i].c[0];
		if (node->vertices.p[i].c[1] < minY) minY = node->vertices.p[i].c[1];
		if (node->vertices.p[i].c[0] > maxX) maxX = node->vertices.p[i].c[0];
		if (node->vertices.p[i].c[1] > maxY) maxY = node->vertices.p[i].c[1];
	}
	/* printf ("minX %f maxX %f minY %f maxY %f\n",minX, maxX, minY, maxY); */
	Ssize = maxX - minX;
	Tsize = maxY - minY;
	/* printf ("ssize %f tsize %f\n",Ssize, Tsize); */

	for (i=0; i<node->vertices.n; i++) {
		*fp = (node->vertices.p[i].c[0] - minX) / Ssize; fp++;
		*fp = (node->vertices.p[i].c[1] - minY) / Tsize; fp++;
	}
}

void collide_TriangleSet2D (struct X3D_TriangleSet2D *node) {
	UNUSED (node);
}

void collide_Disk2D (struct X3D_Disk2D *node) {
	UNUSED (node);
}

void collide_Rectangle2D (struct X3D_Rectangle2D *node) {
		/* Modified Box code. */

	       /*easy access, naviinfo.step unused for sphere collisions */
	       GLdouble awidth = naviinfo.width; /*avatar width*/
	       GLdouble atop = naviinfo.width; /*top of avatar (relative to eyepoint)*/
	       GLdouble abottom = -naviinfo.height; /*bottom of avatar (relative to eyepoint)*/
	       GLdouble astep = -naviinfo.height+naviinfo.step;

	       GLdouble modelMatrix[16];
	       GLdouble upvecmat[16];
	       struct pt iv = {0,0,0};
	       struct pt jv = {0,0,0};
	       struct pt kv = {0,0,0};
	       struct pt ov = {0,0,0};

	       struct pt t_orig = {0,0,0};
	       GLdouble scale; /* FIXME: won''t work for non-uniform scales. */

	       struct pt delta;
	       struct pt tupv = {0,1,0};

		iv.x = node->size.c[0];
		jv.y = node->size.c[1]; 
		kv.z = 0.0;
		ov.x = -((node->size).c[0])/2; ov.y = -((node->size).c[1])/2; ov.z = 0.0;

	       /* get the transformed position of the Box, and the scale-corrected radius. */
	       fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);

	       transform3x3(&tupv,&tupv,modelMatrix);
	       matrotate2v(upvecmat,ViewerUpvector,tupv);
	       matmultiply(modelMatrix,upvecmat,modelMatrix);
	       matinverse(upvecmat,upvecmat);

	       /* values for rapid test */
	       t_orig.x = modelMatrix[12];
	       t_orig.y = modelMatrix[13];
	       t_orig.z = modelMatrix[14];
	       scale = pow(det3x3(modelMatrix),1./3.);
	       if(!fast_ycylinder_box_intersect(abottom,atop,awidth,t_orig,scale*node->size.c[0],
			scale*node->size.c[1],0.0)) return;

	       /* get transformed box edges and position */
	       transform(&ov,&ov,modelMatrix);
	       transform3x3(&iv,&iv,modelMatrix);
	       transform3x3(&jv,&jv,modelMatrix);
	       transform3x3(&kv,&kv,modelMatrix);


	       delta = box_disp(abottom,atop,astep,awidth,ov,iv,jv,kv);

	       vecscale(&delta,&delta,-1);
	       transform3x3(&delta,&delta,upvecmat);

	       accumulate_disp(&CollisionInfo,delta);


		#ifdef COLLISIONVERBOSE
	       if((fabs(delta.x) != 0. || fabs(delta.y) != 0. || fabs(delta.z) != 0.))
	           printf("COLLISION_BOX: (%f %f %f) (%f %f %f)\n",
			  ov.x, ov.y, ov.z,
			  delta.x, delta.y, delta.z
			  );
	       if((fabs(delta.x != 0.) || fabs(delta.y != 0.) || fabs(delta.z) != 0.))
	           printf("iv=(%f %f %f) jv=(%f %f %f) kv=(%f %f %f)\n",
			  iv.x, iv.y, iv.z,
			  jv.x, jv.y, jv.z,
			  kv.x, kv.y, kv.z
			  );
		#endif
}
