/*******************************************************************
 Copyright (C) 2005, 2006 John Stewart, Ayla Khan, Sarah Dumoulin, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*******************************************************************

	X3D H-Anim Component

*********************************************************************/

#include <math.h>
#include "headers.h"
#include "installdir.h"
#include "OpenGL_Utils.h"

/* last HAnimHumanoid skinCoord and skinNormals */
void *HANimSkinCoord = 0;
void *HAnimSkinNormal = 0;

void prep_HAnimJoint (struct X3D_HAnimJoint *node) {
/*
	GLfloat my_rotation;
	GLfloat my_scaleO=0;
	int	recalculate_dist;
*/
return;
#ifdef HANIMHANIM
        /* rendering the viewpoint means doing the inverse transformations in reverse order (while poping stack),
         * so we do nothing here in that case -ncoder */

	 /* we recalculate distance on last pass, or close to it, and only
	 once per event-loop tick. we can do it on the last pass - the
	 render_sensitive pass, but when mouse is clicked (eg, moving in
	 examine mode, sensitive node code is not rendered. So, we choose
	 the second-last pass. ;-) */
	recalculate_dist = render_light;

	/* printf ("render_hier vp %d geom %d light %d sens %d blend %d prox %d col %d\n",*/
	/* render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision);*/

	if(!render_vp) {
                /* glPushMatrix();*/
		fwXformPush(node);

		/* might we have had a change to a previously ignored value? */
		if (node->_change != node->_dlchange) {
			/* printf ("re-rendering for %d\n",node);*/
			node->__do_center = verify_translate ((GLfloat *)node->center.c);
			node->__do_trans = verify_translate ((GLfloat *)node->translation.c);
			node->__do_scale = verify_scale ((GLfloat *)node->scale.c);
			node->__do_rotation = verify_rotate ((GLfloat *)node->rotation.r);
			node->__do_scaleO = verify_rotate ((GLfloat *)node->scaleOrientation.r);
			node->_dlchange = node->_change;
		}



		/* TRANSLATION */
		if (node->__do_trans)
			glTranslatef(node->translation.c[0],node->translation.c[1],node->translation.c[2]);

		/* CENTER */
		if (node->__do_center)
			glTranslatef(node->center.c[0],node->center.c[1],node->center.c[2]);

		/* ROTATION */
		if (node->__do_rotation) {
			my_rotation = node->rotation.r[3]/3.1415926536*180;
			glRotatef(my_rotation,
				node->rotation.r[0],node->rotation.r[1],node->rotation.r[2]);
		}

		/* SCALEORIENTATION */
		if (node->__do_scaleO) {
			my_scaleO = node->scaleOrientation.r[3]/3.1415926536*180;
			glRotatef(my_scaleO, node->scaleOrientation.r[0],
				node->scaleOrientation.r[1],node->scaleOrientation.r[2]);
		}


		/* SCALE */
		if (node->__do_scale)
			glScalef(node->scale.c[0],node->scale.c[1],node->scale.c[2]);

		/* REVERSE SCALE ORIENTATION */
		if (node->__do_scaleO)
			glRotatef(-my_scaleO, node->scaleOrientation.r[0],
				node->scaleOrientation.r[1],node->scaleOrientation.r[2]);

		/* REVERSE CENTER */
		if (node->__do_center)
			glTranslatef(-node->center.c[0],-node->center.c[1],-node->center.c[2]);

		/* did either we or the Viewpoint move since last time? */
		if (recalculate_dist) {
			/* printf ("calling recordDistance for %d\n",node);*/
			recordDistance(node);
			/* printf ("ppv %d\n");*/

	       }
        }
#endif
}


void prep_HAnimSite (struct X3D_HAnimSite *node) {

	/*
	GLfloat my_rotation;
	GLfloat my_scaleO=0;
	int	recalculate_dist;
	*/
return;
#ifdef HANIMHANIM

        /* rendering the viewpoint means doing the inverse transformations in reverse order (while poping stack),
         * so we do nothing here in that case -ncoder */

	 /* we recalculate distance on last pass, or close to it, and only
	 once per event-loop tick. we can do it on the last pass - the
	 render_sensitive pass, but when mouse is clicked (eg, moving in
	 examine mode, sensitive node code is not rendered. So, we choose
	 the second-last pass. ;-) */
	recalculate_dist = render_light;

	/* printf ("render_hier vp %d geom %d light %d sens %d blend %d prox %d col %d\n",*/
	/* render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision);*/

	if(!render_vp) {
                /* glPushMatrix();*/
		fwXformPush(node);

		/* might we have had a change to a previously ignored value? */
		if (node->_change != node->_dlchange) {
			/* printf ("re-rendering for %d\n",node);*/
			node->__do_center = verify_translate ((GLfloat *)node->center.c);
			node->__do_trans = verify_translate ((GLfloat *)node->translation.c);
			node->__do_scale = verify_scale ((GLfloat *)node->scale.c);
			node->__do_rotation = verify_rotate ((GLfloat *)node->rotation.r);
			node->__do_scaleO = verify_rotate ((GLfloat *)node->scaleOrientation.r);
			node->_dlchange = node->_change;
		}



		/* TRANSLATION */
		if (node->__do_trans)
			glTranslatef(node->translation.c[0],node->translation.c[1],node->translation.c[2]);

		/* CENTER */
		if (node->__do_center)
			glTranslatef(node->center.c[0],node->center.c[1],node->center.c[2]);

		/* ROTATION */
		if (node->__do_rotation) {
			my_rotation = node->rotation.r[3]/3.1415926536*180;
			glRotatef(my_rotation,
				node->rotation.r[0],node->rotation.r[1],node->rotation.r[2]);
		}

		/* SCALEORIENTATION */
		if (node->__do_scaleO) {
			my_scaleO = node->scaleOrientation.r[3]/3.1415926536*180;
			glRotatef(my_scaleO, node->scaleOrientation.r[0],
				node->scaleOrientation.r[1],node->scaleOrientation.r[2]);
		}


		/* SCALE */
		if (node->__do_scale)
			glScalef(node->scale.c[0],node->scale.c[1],node->scale.c[2]);

		/* REVERSE SCALE ORIENTATION */
		if (node->__do_scaleO)
			glRotatef(-my_scaleO, node->scaleOrientation.r[0],
				node->scaleOrientation.r[1],node->scaleOrientation.r[2]);

		/* REVERSE CENTER */
		if (node->__do_center)
			glTranslatef(-node->center.c[0],-node->center.c[1],-node->center.c[2]);

		/* did either we or the Viewpoint move since last time? */
		if (recalculate_dist) {
			/* printf ("calling recordDistance for %d\n",node);*/
			recordDistance(node);
			/* printf ("ppv %d\n");*/

	       }
        }
#endif
}

void render_HAnimHumanoid (struct X3D_HAnimHumanoid *node) {
	/* save the skinCoords and skinNormals for use in following HAnimJoints */
	HANimSkinCoord = node->skinCoord;
	HAnimSkinNormal = node->skinNormal;

	/* printf ("rendering HAnimHumanoid\n"); */
}

void render_HAnimJoint (struct X3D_HAnimJoint * node) {
return;
	/* printf ("rendering HAnimJoint %d\n",node); */

}

void child_HAnimHumanoid(struct X3D_HAnimHumanoid *node) {
	int nc;
	DIRECTIONAL_LIGHT_SAVE

	/* any segments at all? */
/*
printf ("hanimHumanoid, segment coutns %d %d %d %d %d %d\n",
		node->joints.n,
		node->segments.n,
		node->sites.n,
		node->skeleton.n,
		node->skin.n,
		node->viewpoints.n);
*/

	nc = node->joints.n + node->segments.n + node->viewpoints.n + node->sites.n +
		node->skeleton.n + node->skin.n;
	if (nc==0) return;

	

	/* should we go down here? */
	/* printf ("HANIMX, rb %x VF_B %x, rg  %x VF_G %x\n",render_blend, VF_Blend, render_geom, VF_Geom);  */
	if (render_blend == VF_Blend)
		if ((node->_renderFlags & VF_Blend) != VF_Blend) {
			return;
		}
	if (render_proximity == VF_Proximity)
		if ((node->_renderFlags & VF_Proximity) != VF_Proximity)  {
			return;
		}

	/* Lets do segments first */
	/* do we have to sort this node? */
	if ((node->segments.n > 1)  && !render_blend) sortChildren(node->segments);
	/* now, just render the non-directionalLight segments */
	normalChildren(node->segments);


	/* Lets do joints second */
	/* do we have to sort this node? */
	if ((node->joints.n > 1)  && !render_blend) sortChildren(node->joints);
	/* now, just render the non-directionalLight joints */
	normalChildren(node->joints);


	/* Lets do sites third */
	/* do we have to sort this node? */
	if ((node->sites.n > 1)  && !render_blend) sortChildren(node->sites);
	/* do we have a DirectionalLight for a child? */
	if(node->has_light) dirlightChildren(node->sites);
	/* now, just render the non-directionalLight sites */
	normalChildren(node->sites);

	/* Lets do skeleton fourth */
	/* do we have to sort this node? */
	if ((node->skeleton.n > 1)  && !render_blend) sortChildren(node->skeleton);
	/* now, just render the non-directionalLight skeleton */
	normalChildren(node->skeleton);

	/* Lets do skin fifth */
	/* do we have to sort this node? */
	if ((node->skin.n > 1)  && !render_blend) sortChildren(node->skin);
	/* do we have a DirectionalLight for a child? */
	if(node->has_light) dirlightChildren(node->skin);
	/* now, just render the non-directionalLight skin */
	normalChildren(node->skin);


	/* Lets do viewpoints last */
	normalChildren(node->segments);


	/* BoundingBox/Frustum stuff */
	if (render_geom && (!render_blend)) {
		EXTENTTOBBOX
		/* pass the bounding box calculations on up the chain */
		propagateExtent((struct X3D_Box *)node);
		BOUNDINGBOX
	}

	/* did we have that directionalLight? */

	DIRECTIONAL_LIGHT_OFF
}


void child_HAnimJoint(struct X3D_HAnimJoint *node) {
	/* int nc = ((node->children).n); */
return;
#ifdef HANIMHANIM
	/* any children at all? */
	if (nc==0) return;

	/* should we go down here? */
	/* printf ("HANIMX, rb %x VF_B %x, rg  %x VF_G %x\n",render_blend, VF_Blend, render_geom, VF_Geom);  */
	if (render_blend == VF_Blend)
		if ((node->_renderFlags & VF_Blend) != VF_Blend) {
			return;
		}
	if (render_proximity == VF_Proximity)
		if ((node->_renderFlags & VF_Proximity) != VF_Proximity)  {
			return;
		}

	/* do we have to sort this node? */
	if ((nc > 1)  && !render_blend) sortChildren(node->children);

	/* just render the non-directionalLight children */
	normalChildren(node->children);


	/* BoundingBox/Frustum stuff */
	if (render_geom && (!render_blend)) {
		EXTENTTOBBOX

		/* pass the bounding box calculations on up the chain */
		propagateExtent((struct X3D_Box *)node);
		BOUNDINGBOX
	}
#endif
}

void child_HAnimSegment(struct X3D_HAnimSegment *node) {
	/* int nc = ((node->children).n); */
return;
#ifdef HANIMHANIM
	/* any children at all? */
	if (nc==0) return;

	/* should we go down here? */
	/* printf ("HANIMX, rb %x VF_B %x, rg  %x VF_G %x\n",render_blend, VF_Blend, render_geom, VF_Geom);  */
	if (render_blend == VF_Blend)
		if ((node->_renderFlags & VF_Blend) != VF_Blend) {
			return;
		}
	if (render_proximity == VF_Proximity)
		if ((node->_renderFlags & VF_Proximity) != VF_Proximity)  {
			return;
		}

	/* do we have to sort this node? Only if not a proto - only first node has visible children. */
	if ((nc > 1)  && !render_blend) sortChildren(node->children);

	/* now, just render the non-directionalLight children */
	normalChildren(node->children);


	/* BoundingBox/Frustum stuff */
	if (render_geom && (!render_blend)) {
		EXTENTTOBBOX

		/* pass the bounding box calculations on up the chain */
		propagateExtent((struct X3D_Box *)node);
		BOUNDINGBOX
	}
#endif
}


void child_HAnimSite(struct X3D_HAnimSite *node) {
	/* int nc = ((node->children).n); */
	DIRECTIONAL_LIGHT_SAVE
return;
#ifdef HANIMHANIM
	/* any children at all? */
	if (nc==0) return;

	/* should we go down here? */
	/* printf ("HANIMX, rb %x VF_B %x, rg  %x VF_G %x\n",render_blend, VF_Blend, render_geom, VF_Geom);  */
	if (render_blend == VF_Blend)
		if ((node->_renderFlags & VF_Blend) != VF_Blend) {
			return;
		}
	if (render_proximity == VF_Proximity)
		if ((node->_renderFlags & VF_Proximity) != VF_Proximity)  {
			return;
		}

	/* do we have to sort this node? */
	if ((nc > 1)  && !render_blend) sortChildren(node->children);

	/* do we have a DirectionalLight for a child? */
	if(node->has_light) dirlightChildren(node->children);

	/* now, just render the non-directionalLight children */
	normalChildren(node->children);


	/* BoundingBox/Frustum stuff */
	if (render_geom && (!render_blend)) {
		EXTENTTOBBOX

		/* pass the bounding box calculations on up the chain */
		propagateExtent((struct X3D_Box *)node);
		BOUNDINGBOX
	}

	DIRECTIONAL_LIGHT_OFF
#endif
}

void fin_HAnimSite (struct X3D_HAnimSite * node) {
        if(!render_vp) {
	/* VERIFY THIS CAST */
            fwXformPop((struct X3D_Transform *)node);
        } else {
           /*Rendering the viewpoint only means finding it, and calculating the reverse WorldView matrix.*/
            if(found_vp) {
                glTranslatef(((node->center).c[0]),((node->center).c[1]),((node->center).c[2])
                );
                glRotatef(((node->scaleOrientation).r[3])/3.1415926536*180,((node->scaleOrientation).r[0]),((node->scaleOrientation).r[1]),((node->scaleOrientation).r[2])
                );
                glScalef(1.0/(((node->scale).c[0])),1.0/(((node->scale).c[1])),1.0/(((node->scale).c[2]))
                );
                glRotatef(-(((node->scaleOrientation).r[3])/3.1415926536*180),((node->scaleOrientation).r[0]),((node->scaleOrientation).r[1]),((node->scaleOrientation).r[2])
                );
                glRotatef(-(((node->rotation).r[3]))/3.1415926536*180,((node->rotation).r[0]),((node->rotation).r[1]),((node->rotation).r[2])
                );
                glTranslatef(-(((node->center).c[0])),-(((node->center).c[1])),-(((node->center).c[2]))
                );
                glTranslatef(-(((node->translation).c[0])),-(((node->translation).c[1])),-(((node->translation).c[2]))
                );
            }
        }
}

void fin_HAnimJoint (struct X3D_HAnimJoint * node) {
        if(!render_vp) {
	/* VERIFY THIS CAST */
            fwXformPop((struct X3D_Transform *)node);
        } else {
           /*Rendering the viewpoint only means finding it, and calculating the reverse WorldView matrix.*/
            if(found_vp) {
                glTranslatef(((node->center).c[0]),((node->center).c[1]),((node->center).c[2])
                );
                glRotatef(((node->scaleOrientation).r[3])/3.1415926536*180,((node->scaleOrientation).r[0]),((node->scaleOrientation).r[1]),((node->scaleOrientation).r[2])
                );
                glScalef(1.0/(((node->scale).c[0])),1.0/(((node->scale).c[1])),1.0/(((node->scale).c[2]))
                );
                glRotatef(-(((node->scaleOrientation).r[3])/3.1415926536*180),((node->scaleOrientation).r[0]),((node->scaleOrientation).r[1]),((node->scaleOrientation).r[2])
                );
                glRotatef(-(((node->rotation).r[3]))/3.1415926536*180,((node->rotation).r[0]),((node->rotation).r[1]),((node->rotation).r[2])
                );
                glTranslatef(-(((node->center).c[0])),-(((node->center).c[1])),-(((node->center).c[2]))
                );
                glTranslatef(-(((node->translation).c[0])),-(((node->translation).c[1])),-(((node->translation).c[2]))
                );
            }
        }
}

void changed_HAnimSite (struct X3D_HAnimSite *node) {
               int i;
                int nc = ((node->children).n);
                struct X3D_Box *p;

		DIRECTIONAL_LIGHT_FIND
		INITIALIZE_EXTENT
}
