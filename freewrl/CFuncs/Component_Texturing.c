/*******************************************************************
 Copyright (C) 2005, 2006 John Stewart, Ayla Khan, Sarah Dumoulin, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*******************************************************************

	X3D Texturing Component

*********************************************************************/

#include <math.h>
#include "headers.h"
#include "installdir.h"

/* verify the TextureCoordinateGenerator node - if the params are ok, then the internal
   __compiledmode is NOT zero. If there are problems, the __compiledmode IS zero */

void render_TextureCoordinateGenerator(struct X3D_TextureCoordinateGenerator *node) {
	char *modeptr;

	if (node->_ichange != node->_change) {
		MARK_NODE_COMPILED

		modeptr = node->mode->strptr;

		/* make the __compiledmode reflect actual OpenGL parameters */
		if(strcmp("SPHERE-REFLECT-LOCAL",modeptr)==0) {
			node->__compiledmode = GL_SPHERE_MAP;
		} else if(strcmp("SPHERE-REFLECT",modeptr)==0) {
			node->__compiledmode = GL_SPHERE_MAP;
		} else if(strcmp("SPHERE-LOCAL",modeptr)==0) {
			node->__compiledmode = GL_SPHERE_MAP;
		} else if(strcmp("SPHERE",modeptr)==0) {
			node->__compiledmode = GL_SPHERE_MAP;
		} else if(strcmp("CAMERASPACENORMAL",modeptr)==0) {
			node->__compiledmode = GL_NORMAL_MAP;
		} else if(strcmp("CAMERASPACEPOSITION",modeptr)==0) {
			node->__compiledmode = GL_OBJECT_LINEAR;
		} else if(strcmp("CAMERASPACEREFLECTION",modeptr)==0) {
			node->__compiledmode = GL_REFLECTION_MAP;
		} else if(strcmp("COORD-EYE",modeptr)==0) {
			node->__compiledmode = GL_EYE_LINEAR;
		} else if(strcmp("COORD",modeptr)==0) {
			node->__compiledmode = GL_EYE_LINEAR;
		} else if(strcmp("NOISE-EYE",modeptr)==0) {
			node->__compiledmode = GL_EYE_LINEAR;
		} else if(strcmp("NOISE",modeptr)==0) {
			node->__compiledmode = GL_EYE_LINEAR;
		} else {
			printf ("TextureCoordinateGenerator - error - %s invalid as a mode\n",modeptr);
		}
	}
}

void render_TextureCoordinate(struct X3D_TextureCoordinate *node) {
	int i;
	int op;
	struct SFVec2f oFp;

	#ifdef TEXVERBOSE
	struct SFVec2f nFp;
	#endif

	float *fptr;


	#ifdef TEXVERBOSE
	printf ("rendering TextureCoordinate node __compiledpoint %d\n",node->__compiledpoint);
	printf ("tcin %d tcin_count %d oldpoint.n %d\n",global_tcin, global_tcin_count, node->point.n);
	#endif

	/* is node the statusbar? we should *always* have a global_tcin textureIndex */
	if (global_tcin == 0) return;

	/* note: IF this TextureCoordinate is USEd in more than one node, WE CAN NOT "pre-compile" the
	   coordinates, because, potentially the order of texture coordinate usage would be different. 
	   So, in the case where there is more than 1 parent, we have to re-calculate this ordering 
	   every time a texture is displayed */

	if ((node->_ichange != node->_change) || (node->__lastParent != global_tcin_lastParent)) {

		MARK_NODE_COMPILED

		if (node->__compiledpoint.n == 0) {
			node->__compiledpoint.n = global_tcin_count;
			node->__compiledpoint.p = (struct SFVec2f *) MALLOC (sizeof(float) *2 * global_tcin_count);
		} 
	
		fptr = (float *) node->__compiledpoint.p;
		
		/* ok, we have a bunch of triangles, loop through and stream the texture coords
		   into a format that matches 1 for 1, the coordinates */
	
		for (i=0; i<global_tcin_count; i++) {
			op = global_tcin[i];
	
			/* bounds check - is the tex coord greater than the number of points? 	*/
			/* node should have been checked before hand...				*/
			if (op >= node->point.n) {
				#ifdef TEXVERBOSE
				printf ("renderTextureCoord - op %d npoints %d\n",op,node->point.n);
				#endif
				*fptr = 0.0; fptr++; 
				*fptr = 0.0; fptr++; 
			} else {
				oFp = node->point.p[op];
	
				#ifdef TEXVERBOSE
				printf ("TextureCoordinate copying %d to %d\n",op,i);	
				printf ("	op %f %f\n",oFp.c[0], oFp.c[1]);
				#endif
	
				*fptr = oFp.c[0]; fptr++; *fptr = oFp.c[1]; fptr++;
			}
		}
		
			
		#ifdef TEXVERBOSE
		for (i=0; i<global_tcin_count; i++) {
			nFp = node->__compiledpoint.p[i];
			printf ("checking... %d %f %f\n",i,nFp.c[0], nFp.c[1]);
		}
		#endif
	}

	/* keep this last parent around, so that MAYBE we do not have to re-do the compiled node */
	node->__lastParent = global_tcin_lastParent;

	if (node->__compiledpoint.n < global_tcin_count) {
		printf ("TextureCoordinate - problem %d < %d\n",node->__compiledpoint.n,global_tcin_count);
	}

}

void render_PixelTexture (struct X3D_PixelTexture *node) {
	loadTextureNode(X3D_NODE(node),NULL);
	texture_count=1; /* not multitexture - should have saved to bound_textures[0] */
}

void render_ImageTexture (struct X3D_ImageTexture *node) {
	loadTextureNode(X3D_NODE(node),NULL);
	texture_count=1; /* not multitexture - should have saved to bound_textures[0] */
}

void render_MultiTexture (struct X3D_MultiTexture *node) {
	loadMultiTexture(node);
}

void render_MovieTexture (struct X3D_MovieTexture *node) {
	/* really simple, the texture number is calculated, then simply sent here.
	   The bound_textures field is sent, and, made current */

	/*  if this is attached to a Sound node, tell it...*/
	sound_from_audioclip = FALSE;

	loadTextureNode(X3D_NODE(node),NULL);
	bound_textures[texture_count] = node->__ctex;
	/* not multitexture, should have saved to bound_textures[0] */
	
	texture_count=1;
}
