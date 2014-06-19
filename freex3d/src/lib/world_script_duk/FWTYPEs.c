/*
=INSERT_TEMPLATE_HERE=

$Id: jsVRMLClasses.c,v 1.35 2012/03/18 15:41:35 dug9 Exp $

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
#include <system_threads.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>


#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../main/Snapshot.h"
#include "../scenegraph/Collision.h"
#include "../scenegraph/quaternion.h"
#include "../scenegraph/Viewer.h"
#include "../input/SensInterps.h"
#include "../x3d_parser/Bindable.h"

//NO JS ENGINE-SPECIFIC HEADERS ALLOWED IN THIS SOUCRCE FILE
//#include "JScript.h"
//#include "CScripts.h"
//#include "jsUtils.h"
//#include "jsNative.h"
//#include "jsVRMLClasses.h"
#include "FWTYPE.h"
#ifdef HAVE_JAVASCRIPT


/*
	We'll create the fwtypes for FIELDS here
*/







//declarative boilerplate struct in generic C, 
//for generating javascript Classes using any javascript engine

FWNative *FWNativeNew(){
	void *ptr = malloc(sizeof(struct FWNATIVE));
	memset(ptr,0,sizeof(struct FWNATIVE));
	return ptr;
}
FWval FWvalsNew(int n){
	void *ptr = malloc(sizeof(struct FWVAL)*n);
	memset(ptr,0,sizeof(struct FWVAL)*n);
	return ptr;
}



int fwSFColorConstr(FWval fwvals, int arg_scheme, int argc, FWNative ptr){
 	struct SFColor * p = (struct SFColor*)malloc(sizeof(struct SFColor));
	if (argc == 0) {
		p->c[0] = (float) 0.0;
		p->c[1] = (float) 0.0;
		p->c[2] = (float) 0.0;
	} else if (argc == 3) {
		p->c[0] = (float) fwvals[0].dval;
		p->c[1] = (float) fwvals[1].dval;
		p->c[2] = (float) fwvals[2].dval;
	}
	ptr->native = p;
	ptr->wasMalloced = true;
	return TRUE;
}

//getter
int fwSFColorGetter(int index, FWNative fwn, FWval fwval){
	struct SFColor *ptr = (struct SFColor*)fwn->native;
	fwval->itype = 1; //0 = ival, 1=dval, 2=cval, 3=fwnval 4=vrmlval
	switch (index) {
		case 0: //r
		case 1: //g
		case 2: //b
			fwval->dval = ptr->c[index];
			break;
		default:
			return FALSE;
		}
	return TRUE;
}

//setter
int fwSFColorSetter(int index, FWNative fwn, FWval fwval){
	struct SFColor *ptr = (struct SFColor*)fwn->native;
	switch (index) {
		case 0:
		case 1:
		case 2:
			ptr->c[index] = fwval->dval;
			break;
		default:
			return FALSE;
	}
	return TRUE;
}

int fwSFColorGetHSV(FWType fwtype, FWNative fwn, int argc, FWval *fwpars, FWval *fwretval)
{
	double xp[3];
	int i;
	struct SFColor *ptr = fwn->native;
	//if(ptr == NULL) return FALSE;

	/* convert rgb to hsv */
	convertRGBtoHSV(ptr->c[0], ptr->c[1], ptr->c[2],&xp[0],&xp[1],&xp[2]);

	#ifdef JSVRMLCLASSESVERBOSE
        printf("hsv code, orig rgb is %.9g %.9g %.9g\n", ptr->c[0], ptr->c[1], ptr->c[2]);
	printf ("hsv conversion is %lf %lf %lf\n",xp[0],xp[1],xp[2]);
	#endif

	fwretval = malloc(sizeof(struct FWVAL)*3); //JS_NewArrayObject(cx, 3, NULL); 
    for(i=0; i<3; i++) { 
		fwretval[i]->itype = 1;
		fwretval[i]->dval = xp[i];
	}
	return TRUE;
}

int fwSFColorSetHSV(FWType fwtype, FWNative fwn, int argc, FWval *fwpars, FWval *fwretval)
{
	double hue, saturation, value;
	double red,green,blue;
	int i;
	struct SFColor *ptr = fwn->native;

	hue = fwpars[0]->dval;
	saturation = fwpars[1]->dval;
	value = fwpars[2]->dval;

	/* do conversion here!!! */
	#ifdef JSCLASSESVERBOSE
        printf("hsv code, orig rgb is %.9g %.9g %.9g\n", (ptr->v).c[0], (ptr->v).c[1], (ptr->v).c[2]);
	printf ("SFColorSetHSV, have %lf %lf %lf\n",hue,saturation,value);
	#endif
	convertHSVtoRGB(hue,saturation,value, &red, &green, &blue);
	ptr->c[0] = (float) red;
	ptr->c[1] = (float) green;
	ptr->c[2] = (float) blue;
	fwn->valueChanged ++;
	#ifdef JSCLASSESVERBOSE
        printf("hsv code, now rgb is %.9g %.9g %.9g\n", (ptr->v).c[0], (ptr->v).c[1], (ptr->v).c[2]);
	#endif
    return JS_TRUE;
}

int fwSFColorToString(FWType fwtype, FWNative fwn, int argc, FWval *fwpars, FWval *fwretval)
{
	struct SFColor *ptr = fwn->native;
	char _buff[STRING];
	memset(_buff, 0, STRING);
	sprintf(_buff, "%.9g %.9g %.9g",
			ptr->c[0], ptr->c[1], ptr->c[2]);
	fwretval = malloc(sizeof(struct FWVAL));
	fwretval[0]->cval = strdup(_buff);
    return TRUE;
}

int fwSFColorAssign(FWType fwtype, FWNative fwn, int argc, FWval *fwpars, FWval *fwretval)
{
	FWNative from;
	from = fwpars[0]->fwnval;
	fwn->valueChanged++;
	fwn->native = malloc(sizeof(struct SFColor));
	memcpy(fwn->native,from->native,sizeof(struct SFColor));
	fwn->wasMalloced = TRUE; //is the above on the heap?
    return JS_TRUE;
}



FWFunctionSpec (FWFuncSFColor)[] = {
	{"getHSV", fwSFColorGetHSV, 7,0,0},
	{"setHSV", fwSFColorSetHSV, 0,3,1},
	{"toString", fwSFColorToString, 2,0,0},
	{"assign", fwSFColorAssign, 5,1,5},
	{0},
};

FWPropertySpec (FWPropSFColor)[] = {
	{"r", 0, 1},
	{"g", 1, 1},
	{"b", 2, 1},
	{0},
};

FWConstructorSpec (FWConSFColor)[] = {
	{0,0},
	{1,5},
	{3,1,1,1},
};

//JSClass MFVec2fClass = {
FWTYPE SFColorType = {
	"SFColor",
	0, //assigned later
	FWPropSFColor,
	3,
	FWFuncSFColor,
	4,
	fwSFColorConstr,
	FWConSFColor,
	3,
	fwSFColorGetter,
	fwSFColorSetter,
};
//char SFColorType [] = { "SFColor" };
//FWTYPE SFColorType [] = { "SFColor" };

struct JSLoadPropElement {
	JSClass *class;
	void *constr;
	void *Functions;
	char *id;
};

FWTYPE (FWTYPos) [] = {
	SFColorType,
        0,
};

/* load the FreeWRL classes into the context etc in opaque*/
int fwLoadVrmlClasses(void* opaque) {
	int i;
	i=0;
	while (FWTYPEs[i] != NULL) {
		if(!initializeClassInContext(i, opaque, FWTYPEs[i]) {
			return FALSE;
		}
		i++;
	}
	return TRUE;
}

#endif /* HAVE_JAVASCRIPT */
