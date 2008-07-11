/*******************************************************************
 Copyright (C) 2005, 2006 John Stewart, Ayla Khan, Sarah Dumoulin, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*******************************************************************

	X3D Shape Component

*********************************************************************/

#include <math.h>
#include "headers.h"
#include "installdir.h"

void render_LineProperties (struct X3D_LineProperties *node) {
	GLint	factor;
	GLushort pat;

	if (node->applied) {
		global_lineProperties=TRUE;
		if (node->linewidthScaleFactor > 1.0) {
			glLineWidth(node->linewidthScaleFactor);
			glPointSize(node->linewidthScaleFactor);
		}
			
		if (node->linetype > 0) {
			factor = 1;
			pat = 0xffff; /* can not support fancy line types - this is the default */
			switch (node->linetype) {
				case 2: pat = 0xaaaa; break;
				case 3: pat = 0x4444; break;
				case 4: pat = 0xa4a4; break;
				case 5: pat = 0xaa44; break;
				case 6: pat = 0x0100; break;
				case 7: pat = 0x0100; break;
				case 10: pat = 0xaaaa; break;
				case 11: pat = 0x0170; break;
				case 12: pat = 0x0000; break;
				case 13: pat = 0x0000; break;
				default: {}
			}
			glLineStipple (factor,pat);
			glEnable(GL_LINE_STIPPLE);
		}
	}

}

void render_FillProperties (struct X3D_FillProperties *node) {
		GLubyte halftone[] = {
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55,
		0xAA, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55};

		global_fillProperties=TRUE;

		if (!node->filled) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (node->hatched) {
			glColor3f (node->hatchColor.c[0], node->hatchColor.c[1],node->hatchColor.c[2]);
			glPolygonStipple(halftone);			
			glEnable (GL_POLYGON_STIPPLE);
		}
}

void render_Material (struct X3D_Material *node) {
		int i;
		float dcol[4];
		float ecol[4];
		float scol[4];
		float shin;
		float amb;
		float trans;

		/* set the diffuseColor; we will reset this later if the
		   texture depth is 3 (RGB texture) */

		for (i=0; i<3;i++){ dcol[i] = node->diffuseColor.c[i]; }

		/* set the transparency here for the material */
		trans = 1.0 - node->transparency;
		if (trans<0.0) trans = 0.0;
		if (trans>=0.99) trans = 0.99;

		/* record this for possible texture filtering later */
		last_transparency = trans;

		dcol[3] = trans;
		scol[3] = trans;
		ecol[3] = trans;

		/* the diffuseColor might change, depending on the texture depth - that we do not have yet */
		do_glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, dcol);

		/* do the ambientIntensity; this will allow lights with ambientIntensity to
		   illuminate it as per the spec. Note that lights have the ambientIntensity
		   set to 0.0 by default; this should make ambientIntensity lighting be zero
		   via OpenGL lighting equations. */
		amb = node->ambientIntensity;

 		for(i=0; i<3; i++) { dcol[i] *= amb; } 
		do_glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, dcol);

		for (i=0; i<3;i++){ scol[i] = node->specularColor.c[i]; }
		do_glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, scol);

		for (i=0; i<3;i++){ ecol[i] = node->emissiveColor.c[i]; }
		do_glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, ecol);
		glColor3f(ecol[0],ecol[1],ecol[2]);

		shin = 128.0* node->shininess;
		do_shininess(shin);
}


void child_Shape (struct X3D_Shape *node) {
		int count;
		void *tmpN;

		if(!(node->geometry)) { return; }

		/* mark that this shape was "done" for OcclusionTests */
		if (node->__OccludeNumber < QueryCount) OccNodeRendered[node->__OccludeNumber] = TRUE;

		RECORD_DISTANCE

		if((render_collision) || (render_sensitive)) {
			/* only need to forward the call to the child */
			POSSIBLE_PROTO_EXPANSION(node->geometry,tmpN)
			render_node(tmpN);
			return;
		}

		/* reset textureTransform pointer */
		this_textureTransform = 0;
		global_lineProperties=FALSE;
		global_fillProperties=FALSE;



		/* JAS - if not collision, and render_geom is not set, no need to go further */
		/* printf ("render_Shape vp %d geom %d light %d sens %d blend %d prox %d col %d\n",*/
		/* render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision);*/

		/* a texture and a transparency flag... */
		texture_count = 0; /* will be >=1 if textures found */
		have_texture = FALSE;

                /* assume that lighting is enabled. Absence of Material or Appearance
                   node will turn lighting off; in this case, at the end of Shape, we
                   have to turn lighting back on again. */
                LIGHTING_ON
		COLOR_MATERIAL_OFF
		
		/* is there an associated appearance node? */
       	        if(node->appearance) {
			POSSIBLE_PROTO_EXPANSION(node->appearance,tmpN)
                        render_node(tmpN);
       	        } else {
                        /* no material, so just colour the following shape */
                       	/* Spec says to disable lighting and set coloUr to 1,1,1 */
                       	LIGHTING_OFF
       	                glColor3f(1.0,1.0,1.0);

			/* tell the rendering passes that this is just "normal" */
			last_texture_type = NOTEXTURE;
			last_transparency = 1.0;
                }

		/* now, are we rendering blended nodes or normal nodes?*/
		if (render_blend == (node->_renderFlags & VF_Blend)) {

                        #ifdef SHAPEOCCLUSION
			BEGINOCCLUSIONQUERY
                        #endif

			POSSIBLE_PROTO_EXPANSION(node->geometry,tmpN)
			render_node(tmpN);

                        #ifdef SHAPEOCCLUSION
			ENDOCCLUSIONQUERY
                        #endif

		}

               /* did the lack of an Appearance or Material node turn lighting off? */
		LIGHTING_ON

		/* any line properties to reset? */
		if (global_lineProperties) {
			glDisable (GL_POLYGON_STIPPLE);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		if (global_fillProperties) {
			glDisable (GL_LINE_STIPPLE);
			glLineWidth(1.0);
			glPointSize(1.0);
		}
}


void child_Appearance (struct X3D_Appearance *node) {
                last_texture_type = NOTEXTURE;
                last_transparency = 1.0;
		void *tmpN;

        /* printf ("in Appearance, this %d, nodeType %d\n",node, node->_nodeType);
         printf (" vp %d geom %d light %d sens %d blend %d prox %d col %d\n",
         render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision); */

                if(node->material) {
			POSSIBLE_PROTO_EXPANSION(node->material,tmpN)
                        render_node(tmpN);
                } else {
                        /* no material, so just colour the following shape */
                        /* Spec says to disable lighting and set coloUr to 1,1,1 */
                        LIGHTING_OFF
                        glColor3f(1.0,1.0,1.0);
                }

                if (node->fillProperties) {
			POSSIBLE_PROTO_EXPANSION(node->fillProperties,tmpN)
                        render_node(tmpN);
                }

                /* set line widths - if we have line a lineProperties node */
                if (node->lineProperties) {
			POSSIBLE_PROTO_EXPANSION(node->lineProperties,tmpN)
                        render_node(tmpN);
                }

                if(node->texture) {
                        /* we have to do a glPush, then restore, later */
                        have_texture=TRUE;
                        /* glPushAttrib(GL_ENABLE_BIT); */

                        /* is there a TextureTransform? if no texture, fugutaboutit */
			POSSIBLE_PROTO_EXPANSION(node->textureTransform,this_textureTransform)
                        /* this_textureTransform = node->textureTransform; */

                        /* now, render the texture */
			POSSIBLE_PROTO_EXPANSION(node->texture,tmpN)
                        render_node(tmpN);
                }
}
