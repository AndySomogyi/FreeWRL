/*
=INSERT_TEMPLATE_HERE=

$Id$

Proximity sensor macro.

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


#ifndef __FREEWRL_SCENEGRAPH_RENDERFUNCS_H__
#define __FREEWRL_SCENEGRAPH_RENDERFUNCS_H__

/* trat: test if a ratio is reasonable */
#define TRAT(a) ((a) > 0 && ((a) < hitPointDist || hitPointDist < 0))

/* structure for rayhits */
struct currayhit {
	struct X3D_Node *hitNode; /* What node hit at that distance? */
	GLDOUBLE modelMatrix[16]; /* What the matrices were at that node */
	GLDOUBLE projMatrix[16];
};

extern struct currayhit rayHit,rayph,rayHitHyper;
extern double hitPointDist;                   /* in VRMLC.pm */
extern struct point_XYZ hp;                     /* in VRMLC.pm */
extern void *hypersensitive;            /* in VRMLC.pm */
extern int hyperhit;                    /* in VRMLC.pm */
extern struct point_XYZ r1, r2;         /* in VRMLC.pm */


/* function protos */
int nextlight(void);
void render_node(struct X3D_Node *node);

extern int BrowserAction;
extern struct X3D_Anchor *AnchorsAnchor;
extern char *OSX_replace_world_from_console;


void lightState(GLint light, int status);
void saveLightState(int *ls);
void restoreLightState(int *ls);
void fwglLightfv (int light, int pname, GLfloat *params);
void fwglLightf (int light, int pname, GLfloat param);
void initializeLightTables(void);
void chooseAppearanceShader(struct X3D_Material *material_oneSided, struct X3D_TwoSidedMaterial *material_twoSided);
void sendAttribToGPU(int myType, int mySize, int  xtype, int normalized, int stride, float *pointer);
void sendClientStateToGPU(int enable, int cap);
void sendArraysToGPU (int mode, int first, int count);
void sendElementsToGPU (int mode, int count, int type, int *indices);


#endif /* __FREEWRL_SCENEGRAPH_RENDERFUNCS_H__ */
