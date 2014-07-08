/*

  A substantial amount of code has been adapted from js/src/js.c,
  which is the sample application included with the javascript engine.

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
#include <config.h>
#if defined(JAVASCRIPT_DUK)
#include <system.h>
#include <system_threads.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>
#include <list.h>

#include <io_files.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../main/Snapshot.h"
#include "../scenegraph/LinearAlgebra.h"
#include "../scenegraph/Collision.h"
#include "../scenegraph/quaternion.h"
#include "../scenegraph/Viewer.h"
#include "../input/SensInterps.h"
#include "../x3d_parser/Bindable.h"
#include "../input/InputFunctions.h"

#include "FWTYPE.h"
#include "JScript.h"
#include "CScripts.h"


/********************************************************/
/*							*/
/* Second part - SF classes				*/
/*							*/
/********************************************************/

/*
Notes for the virtual proxy approach used here:
The field types represented by primitives in ecmascript have no constructor,
 gc=0 on their pointer, and getter/setter convert to/from ecma primitives:
 SFBool		boolean
 SFInt32	numeric
 SFFloat	numeric
 SFTime		numeric
 SFDouble	numeric
 SFString	string
For Script node fields they show up as all other field types as properties on the js context global object.
But unlike other field types, there's no new SFBool(). Getters and setters convert to/from ecma primitives
and set the valueChanged flag when set. Similarly other fieldtype functions and getter/setters convert to/from
ecma primitive instead of one of the above, and never generate a new one of these.
*/



/* from http://www.cs.rit.edu/~ncs/color/t_convert.html */
double MIN(double a, double b, double c) {
	double min;
	if((a<b)&&(a<c))min=a; else if((b<a)&&(b<c))min=b; else min=c; return min;
}

double MAX(double a, double b, double c) {
	double max;
	if((a>b)&&(a>c))max=a; else if((b>a)&&(b>c))max=b; else max=c; return max;
}

void convertRGBtoHSV(double r, double g, double b, double *h, double *s, double *v) {
	double my_min, my_max, delta;

	my_min = MIN( r, g, b );
	my_max = MAX( r, g, b );
	*v = my_max;				/* v */
	delta = my_max - my_min;
	if( my_max != 0 )
		*s = delta / my_max;		/* s */
	else {
		/* r = g = b = 0 */		/* s = 0, v is undefined */
		*s = 0;
		*h = -1;
		return;
	}
	if( r == my_max )
		*h = ( g - b ) / delta;		/* between yellow & magenta */
	else if( g == my_max )
		*h = 2 + ( b - r ) / delta;	/* between cyan & yellow */
	else
		*h = 4 + ( r - g ) / delta;	/* between magenta & cyan */
	*h *= 60;				/* degrees */
	if( *h < 0 )
		*h += 360;
}
void convertHSVtoRGB( double h, double s, double v ,double *r, double *g, double *b)
{
	int i;
	double f, p, q, t;
	if( s == 0 ) {
		/* achromatic (grey) */
		*r = *g = *b = v;
		return;
	}
	h /= 60;			/* sector 0 to 5 */
	i = (int) floor( h );
	f = h - i;			/* factorial part of h */
	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );
	switch( i ) {
		case 0: *r = v; *g = t; *b = p; break;
		case 1: *r = q; *g = v; *b = p; break;
		case 2: *r = p; *g = v; *b = t; break;
		case 3: *r = p; *g = q; *b = v; break;
		case 4: *r = t; *g = p; *b = v; break;
		default: *r = v; *g = p; *b = q; break;
	}
}


int SFColor_getHSV(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	//argc should == 0 for getHSV
	struct SFColor *ptr = (struct SFColor *)fwn;
	double xp[3];
	/* convert rgb to hsv */
	convertRGBtoHSV((double)ptr->c[0], (double)ptr->c[1], (double)ptr->c[2],&xp[0],&xp[1],&xp[2]);
	//supposed to return numeric[3] - don't have that set up so sfvec3d
	union anyVrml *any = malloc(sizeof(union anyVrml)); //garbage collector please
	memcpy(any->sfvec3d.c,xp,sizeof(double)*3);
	fwretval->_web3dval.native = any; 
	fwretval->_web3dval.fieldType = FIELDTYPE_SFVec3d;
	fwretval->_web3dval.gc = 'T'; //garbage collect .native (with C free(.native)) when proxy obj is gc'd.
	fwretval->itype = 'W';
	return 1;
}


//#define FIELDTYPE_SFFloat	0
FWTYPE SFFloatType = {
	FIELDTYPE_SFFloat,
	"SFFloat",
	sizeof(float), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFFloat	1
FWTYPE MFFloatType = {
	FIELDTYPE_MFFloat,
	"MFFloat",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFRotation	2
FWTYPE SFRotationType = {
	FIELDTYPE_SFRotation,
	"SFRotation",
	sizeof(struct SFRotation), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFRotation	3
FWTYPE MFRotationType = {
	FIELDTYPE_MFRotation,
	"MFRotation",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFVec3f	4
FWTYPE SFVec3fType = {
	FIELDTYPE_SFVec3f,
	"SFVec3f",
	sizeof(struct SFVec3f), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFVec3f	5
FWTYPE MFVec3fType = {
	FIELDTYPE_MFVec3f,
	"MFVec3f",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFBool	6
FWTYPE SFBoolType = {
	FIELDTYPE_SFBool,
	"SFBool",
	sizeof(int), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFBool	7
FWTYPE MFBoolType = {
	FIELDTYPE_MFBool,
	"MFBool",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFInt32	8
FWTYPE SFInt32Type = {
	FIELDTYPE_SFInt32,
	"SFInt32",
	sizeof(int), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFInt32	9
FWTYPE MFInt32Type = {
	FIELDTYPE_MFInt32,
	"MFInt32",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

int getFieldFromNodeAndIndex(struct X3D_Node* node, int iifield, const char **fieldname, int *type, int *kind, union anyVrml **value);
int SFNode_Iterator(int index, FWTYPE *fwt, FWPointer *pointer, char **name, int *lastProp, int *jndex, char *type, char *readOnly){
	struct X3D_Node *node = ((union anyVrml*)pointer)->sfnode;
	int ftype, kind, ihave;
	union anyVrml *value;
	index ++;
	(*jndex) = 0;
	int iifield = index;
	ihave = getFieldFromNodeAndIndex(node, index, name, &ftype, &kind, &value);
	if(ihave){
		(*jndex) = index;
		(*lastProp) = index;
		(*type) = ftype;
		(*readOnly) = kind == PKW_inputOnly ? 'T' : 0;
		return index;
	}
	return -1;
}
int SFNode_Getter(int index, void * fwn, FWval fwretval){
	struct X3D_Node *node = (struct X3D_Node*)fwn; //may be wrong
	int ftype, kind, ihave, nr;
	const char *name;
	union anyVrml *value;
	nr = 0;
	ihave = getFieldFromNodeAndIndex(node, index, &name, &ftype, &kind, &value);
	if(ihave){
		fwretval->itype = 'W';
		fwretval->_web3dval.fieldType = ftype;
		fwretval->_web3dval.native = value; //Q. am I supposed to deep copy here? I'm not. So I don't set gc.
		nr = 1;
	}
	return nr;
}
void * SFNode_Constructor(FWType fwtype, int nargs, FWval fwpars){
	struct X3D_Node *ptr = NULL; // = malloc(fwtype->size_of); //garbage collector please
	if(nargs == 1){
		if(fwpars[0].itype == 'S'){
			/* for the return of the nodes */
			struct X3D_Group *retGroup;
			char *xstr; 
			char *tmpstr;
			char *separator;
			int ra;
			int count;
			int wantedsize;
			int MallocdSize;
			ttglobal tg = gglobal();
			struct VRMLParser *globalParser = (struct VRMLParser *)tg->CParse.globalParser;
			const char *_c = fwpars[0]._string;

			/* do the call to make the VRML code  - create a new browser just for this string */
			gglobal()->ProdCon.savedParser = (void *)globalParser; globalParser = NULL;
			retGroup = createNewX3DNode(NODE_Group);
			ra = EAI_CreateVrml("String",_c,retGroup);
			globalParser = (struct VRMLParser*)gglobal()->ProdCon.savedParser; /* restore it */
			if(retGroup->children.n < 1) return 0;
			ptr = (struct X3D_Node *)&retGroup->children.p[0];
		}else if(fwpars->itype = 'W'){
			if(fwpars->_web3dval.fieldType == FIELDTYPE_SFNode)
				ptr = fwpars[0]._web3dval.native; //don't gc
			
		}
	}
	return ptr;
}
ArgListType (SFNode_ConstructorArgs)[] = {
		{1,-1,'F',"S"},
		{1,-1,'F',"W"},
		{-1,0,0,NULL},
};
//#define FIELDTYPE_SFNode	10
FWTYPE SFNodeType = {
	FIELDTYPE_SFNode,
	"SFNode",
	sizeof(void*), //sizeof(struct ), 
	SFNode_Constructor, //constructor
	SFNode_ConstructorArgs, //constructor args
	NULL, //Properties,
	SFNode_Iterator, //special iterator
	SFNode_Getter, //Getter,
	NULL, //Setter,
	0,0, //index prop type,readonly
	NULL, //functions
};



//MFW for MF types that take web3d (non-ecma-primitive) types ie new MFColor( new SFColor(0,0,0), new SFColor(.1,.2,.3), ...) 
ArgListType (MFW_ConstructorArgs)[] = {
		{0,0,'F',"W"},
		{-1,0,0,NULL},
};
int sizeofSF(int itype); //thunks MF to SF (usually itype-1) and gets sizeof SF
void * MFW_Constructor(FWType fwtype, int argc, FWval fwpars){
	int lenSF;
	struct Multi_Any *ptr = malloc(sizeof(struct Multi_Any));  ///malloc in 2 parts for MF
	lenSF = sizeofSF(fwtype->itype); 
	ptr->n = argc;
	ptr->p = NULL;
	if(ptr->n)
		ptr->p = malloc(ptr->n * lenSF); // This second part is resizable ie MF[i] = new SF() if i >= (.length), .length is expanded to accomodate
	char *p = ptr->p;
	for(int i=0;i<ptr->n;i++){
		memcpy(p,fwpars[i]._web3dval.native,lenSF);
		p += lenSF;
	}
	return (void *)ptr;
}
FWPropertySpec (MFW_Properties)[] = {
	{"length", -1, 'N', 'F'},
	{NULL,0,0,0},
};
int MFNode_Getter(int index, void * fwn, FWval fwretval){
	struct Multi_Any *ptr = (struct Multi_Any *)fwn;
	int nr = 0;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index == -1){
		//length
		fwretval->_integer = ptr->n;
		fwretval->itype = 'I';
		nr = 1;
	}else if(index > -1 && index < ptr->n){
		int elen = sizeofSF(FIELDTYPE_SFNode);
		fwretval->_web3dval.native = (void *)(ptr->p + index*elen);
		fwretval->_web3dval.fieldType = FIELDTYPE_SFNode;
		fwretval->itype = 'W';
		nr = 1;
	}
	return nr;
}
int MFNode_Setter(int index, void * fwn, FWval fwval){
	struct Multi_Any *ptr = (struct Multi_Any *)fwn;
	int nelen, nold, nr = FALSE;
	int elen = sizeofSF(FIELDTYPE_SFNode);
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index == -1){
		//length
		nold = ptr->n;
		ptr->n = fwval->_integer;
		if(ptr->n > nold){
			//nelen = (int) upper_power_of_two(fwval->_integer);
			ptr->p = realloc(ptr->p,ptr->n * elen);
		}
		nr = TRUE;
	}else if(index > -1){
		if(index >= ptr->n){
			//nold = ptr->n;
			ptr->n = index+1;
			//nelen = (int) upper_power_of_two(ptr->n);
			ptr->p = realloc(ptr->p,ptr->n *elen); //need power of 2 if SFNode children
		}
		memcpy(ptr->p + index*elen,fwval->_web3dval.native, elen);
		nr = TRUE;
	}
	return nr;
}

//#define FIELDTYPE_MFNode	11
FWTYPE MFNodeType = {
	FIELDTYPE_MFNode,
	"MFNode",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	MFW_Constructor, //constructor
	MFW_ConstructorArgs, //constructor args
	MFW_Properties, //Properties,
	NULL, //special iterator
	MFNode_Getter, //Getter,
	MFNode_Setter, //Setter,
	'W',0, //index prop type,readonly
	NULL, //functions
};


int SFColor_Getter(int index, void * fwn, FWval fwretval){
	struct SFColor *ptr = (struct SFColor *)fwn;
	int nr = 0;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index > -1 && index < 3){
		nr = 1;
		switch(index){
		case 0: //r
		case 1: //g
		case 2: //b
			fwretval->_numeric =  ptr->c[index];
			break;
		default:
			nr = 0;
		}
	}
	fwretval->itype = 'N';
	return nr;
}
int SFColor_Setter(int index, void * fwn, FWval fwval){
	struct SFColor *ptr = (struct SFColor *)fwn;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index > -1 && index < 3){
		switch(index){
		case 0: //r
		case 1: //g
		case 2: //b
			ptr->c[index] = fwval->_numeric;
			break;
		}
		return TRUE;
	}
	return FALSE;
}
//typedef int (* FWConstructor)(FWType fwtype, int argc, FWval fwpars);
void * SFColor_Constructor(FWType fwtype, int ic, FWval fwpars){
	struct SFColor *ptr = malloc(fwtype->size_of); //garbage collector please
	if(fwtype->ConstructorArgs[0].nfixedArg == 3)
	for(int i=0;i<3;i++)
		ptr->c[i] = fwpars[i]._numeric;
	return (void *)ptr;
}

FWPropertySpec (SFColor_Properties)[] = {
	{"r", 0, 'N', 'F'},
	{"g", 1, 'N', 'F'},
	{"b", 2, 'N', 'F'},
	{NULL,0,0,0},
};

//typedef struct ArgListType {
//	char nfixedArg;
//	char iVarArgStartsAt; //-1 no varargs
//	char fillMissingFixedWithZero; //T/F if F then complain if short
//	char *argtypes; //if varargs, then argtypes[nfixedArg] == type of varArg, and all varargs are assumed the same type
//} ArgListType;
ArgListType (SFColor_ConstructorArgs)[] = {
		{3,0,'T',"NNN"},
		{-1,0,0,NULL},
};
//#define FIELDTYPE_SFColor	12
FWTYPE SFColorType = {
	FIELDTYPE_SFColor,
	"SFColor",
	sizeof(struct SFColor), //sizeof(struct ), 
	SFColor_Constructor, //constructor
	SFColor_ConstructorArgs, //constructor args
	SFColor_Properties, //Properties,
	NULL, //special iterator
	SFColor_Getter, //Getter,
	SFColor_Setter, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
int MFColor_Getter(int index, void * fwn, FWval fwretval){
	struct Multi_Any *ptr = (struct Multi_Any *)fwn;
	int nr = 0;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index == -1){
		//length
		fwretval->_integer = ptr->n;
		fwretval->itype = 'I';
		nr = 1;
	}else if(index > -1 && index < ptr->n){
		int elen = sizeofSF(FIELDTYPE_SFColor);
		fwretval->_web3dval.native = (void *)(ptr->p + index*elen);
		fwretval->_web3dval.fieldType = FIELDTYPE_SFColor;
		fwretval->itype = 'W';
		nr = 1;
	}
	return nr;
}
int MFColor_Setter(int index, void * fwn, FWval fwval){
	struct Multi_Any *ptr = (struct Multi_Any *)fwn;
	int nr = FALSE;
	int elen = sizeofSF(FIELDTYPE_SFColor);
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index == -1){
		//length
		ptr->n = fwval->_integer;
		ptr->p = realloc(ptr->p,ptr->n * elen);
		nr = TRUE;
	}else if(index > -1){
		if(index >= ptr->n){
			ptr->n = index +1;
			ptr->p = realloc(ptr->p,ptr->n *elen); //need power of 2 if SFNode
		}
		memcpy(ptr->p + index*elen,fwval->_web3dval.native, elen);
		nr = TRUE;
	}
	return nr;
}
//#define FIELDTYPE_MFColor	13
FWTYPE MFColorType = {
	FIELDTYPE_MFColor,
	"MFColor",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	MFW_Constructor, //constructor
	MFW_ConstructorArgs, //constructor args
	MFW_Properties, //Properties,
	NULL, //special iterator
	MFColor_Getter, //Getter,
	MFColor_Setter, //Setter,
	'W',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFColorRGBA	14
FWTYPE SFColorRGBAType = {
	FIELDTYPE_SFColorRGBA,
	"SFColorRGBA",
	sizeof(struct SFColorRGBA), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFColorRGBA	15
FWTYPE MFColorRGBAType = {
	FIELDTYPE_MFColorRGBA,
	"MFColorRGBA",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFTime	16
FWTYPE SFTimeType = {
	FIELDTYPE_SFTime,
	"SFTime",
	sizeof(double), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFTime	17
FWTYPE MFTimeType = {
	FIELDTYPE_MFTime,
	"MFTime",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFString	18
FWTYPE SFStringType = {
	FIELDTYPE_SFString,
	"SFString",
	sizeof(void *), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFString	19
FWTYPE MFStringType = {
	FIELDTYPE_MFString,
	"MFString",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFVec2f	20
FWTYPE SFVec2fType = {
	FIELDTYPE_SFVec2f,
	"SFVec2f",
	sizeof(struct SFVec2f), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_MFVec2f	21
FWTYPE MFVec2fType = {
	FIELDTYPE_MFVec2f,
	"MFVec2f",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFImage	22
FWTYPE SFImageType = {
	FIELDTYPE_SFImage,
	"SFImage",
	sizeof(void *), //sizeof(struct ),  //----------------------unknown struct but it looks like an SFNode with node-specific fields
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_FreeWRLPTR	23
//#define FIELDTYPE_FreeWRLThread	24
//#define FIELDTYPE_SFVec3d	25
FWTYPE SFVec3dType = {
	FIELDTYPE_SFVec3d,
	"SFVec3d",
	sizeof(struct SFVec3d), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_MFVec3d	26
FWTYPE MFVec3dType = {
	FIELDTYPE_MFVec3d,
	"MFVec3d",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_SFDouble	27
FWTYPE SFDoubleType = {
	FIELDTYPE_SFDouble,
	"SFDouble",
	sizeof(double), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_MFDouble	28
FWTYPE MFDoubleType = {
	FIELDTYPE_MFDouble,
	"MFDouble",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_SFMatrix3f	29
FWTYPE SFMatrix3fType = {
	FIELDTYPE_SFMatrix3f,
	"SFMatrix3f",
	sizeof(struct SFMatrix3f), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_MFMatrix3f	30
FWTYPE MFMatrix3fType = {
	FIELDTYPE_MFMatrix3f,
	"MFMatrix3f",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFMatrix3d	31
FWTYPE SFMatrix3dType = {
	FIELDTYPE_SFMatrix3d,
	"SFMatrix3d",
	sizeof(struct SFMatrix3d), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFMatrix3d	32
FWTYPE MFMatrix3dType = {
	FIELDTYPE_MFMatrix3d,
	"MFMatrix3d",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_SFMatrix4f	33
FWTYPE SFMatrix4fType = {
	FIELDTYPE_SFMatrix4f,
	"SFMatrix4f",
	sizeof(struct SFMatrix4f), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};
//#define FIELDTYPE_MFMatrix4f	34
FWTYPE MFMatrix4fType = {
	FIELDTYPE_MFMatrix4f,
	"MFMatrix4f",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_SFMatrix4d	35
FWTYPE SFMatrix4dType = {
	FIELDTYPE_SFMatrix4d,
	"SFMatrix4d",
	sizeof(struct SFMatrix4d), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_MFMatrix4d	36
FWTYPE MFMatrix4dType = {
	FIELDTYPE_MFMatrix4d,
	"MFMatrix4d",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_SFVec2d	37
FWTYPE SFVec2dType = {
	FIELDTYPE_SFVec2d,
	"SFVec2d",
	sizeof(struct SFVec2d), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_MFVec2d	38
FWTYPE MFVec2dType = {
	FIELDTYPE_MFVec2d,
	"MFVec2d",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_SFVec4f	39
FWTYPE SFVec4fType = {
	FIELDTYPE_SFVec4f,
	"SFVec4f",
	sizeof(struct SFVec4f), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_MFVec4f	40
FWTYPE MFVec4fType = {
	FIELDTYPE_MFVec4f,
	"MFVec4f",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_SFVec4d	41
FWTYPE SFVec4dType = {
	FIELDTYPE_SFVec4d,
	"SFVec4d",
	sizeof(struct SFVec4d), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

//#define FIELDTYPE_MFVec4d	42
FWTYPE MFVec4dType = {
	FIELDTYPE_MFVec4d,
	"MFVec4d",
	sizeof(struct Multi_Any), //sizeof(struct ), 
	NULL, //constructor
	NULL, //constructor args
	NULL, //Properties,
	NULL, //special iterator
	NULL, //Getter,
	NULL, //Setter,
	'N',0, //index prop type,readonly
	NULL, //functions
};

void initVRMLFields(FWTYPE** typeArray, int *n){
	typeArray[*n] = &SFFloatType; (*n)++;
	typeArray[*n] = &MFFloatType; (*n)++;
	typeArray[*n] = &SFRotationType; (*n)++;
	typeArray[*n] = &MFRotationType; (*n)++;
	typeArray[*n] = &SFVec3fType; (*n)++;
	typeArray[*n] = &MFVec3fType; (*n)++;
	typeArray[*n] = &SFBoolType; (*n)++;
	typeArray[*n] = &MFBoolType; (*n)++;
	typeArray[*n] = &SFInt32Type; (*n)++;
	typeArray[*n] = &MFInt32Type; (*n)++;
	typeArray[*n] = &SFNodeType; (*n)++;
	typeArray[*n] = &MFNodeType; (*n)++;
	typeArray[*n] = &SFColorType; (*n)++;
	typeArray[*n] = &MFColorType; (*n)++;
	typeArray[*n] = &SFColorRGBAType; (*n)++;
	typeArray[*n] = &MFColorRGBAType; (*n)++;
	typeArray[*n] = &SFTimeType; (*n)++;
	typeArray[*n] = &MFTimeType; (*n)++;
	typeArray[*n] = &SFStringType; (*n)++;
	typeArray[*n] = &MFStringType; (*n)++;
	typeArray[*n] = &SFVec2fType; (*n)++;
	typeArray[*n] = &MFVec2fType; (*n)++;
	typeArray[*n] = &SFImageType; (*n)++;
	//typeArray[*n] = &FreeWRLPTRType; (*n)++;
	//typeArray[*n] = &FreeWRLThreadType; (*n)++;
	typeArray[*n] = &SFVec3dType; (*n)++;
	typeArray[*n] = &MFVec3dType; (*n)++;
	typeArray[*n] = &SFDoubleType; (*n)++;
	typeArray[*n] = &MFDoubleType; (*n)++;
	typeArray[*n] = &SFMatrix3fType; (*n)++;
	typeArray[*n] = &MFMatrix3fType; (*n)++;
	typeArray[*n] = &SFMatrix3dType; (*n)++;
	typeArray[*n] = &MFMatrix3dType; (*n)++;
	typeArray[*n] = &SFMatrix4fType; (*n)++;
	typeArray[*n] = &MFMatrix4fType; (*n)++;
	typeArray[*n] = &SFMatrix4dType; (*n)++;
	typeArray[*n] = &MFMatrix4dType; (*n)++;
	typeArray[*n] = &SFVec2dType; (*n)++;
	typeArray[*n] = &MFVec2dType; (*n)++;
	typeArray[*n] = &SFVec4fType; (*n)++;
	typeArray[*n] = &MFVec4fType; (*n)++;
	typeArray[*n] = &SFVec4dType; (*n)++;
	typeArray[*n] = &MFVec4dType; (*n)++;
}

#endif /* ifdef JAVASCRIPT_DUK */
