/*
=INSERT_TEMPLATE_HERE=

$Id$

???

*/

#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeX3D.h>


#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../world_script/jsUtils.h"
#include "../world_script/CScripts.h"
#include "../main/Snapshot.h"
#include "../scenegraph/Collision.h"
#include "../scenegraph/quaternion.h"
#include "../scenegraph/Viewer.h"
#include "../input/SensInterps.h"
#include "../x3d_parser/Bindable.h"
/* #include "../input/EAIheaders.h" */

#include "jsUtils.h"
#include "jsNative.h"
#include "jsVRMLClasses.h"


/********************************************************/
/*							*/
/* first part - standard helper functions		*/
/*							*/
/********************************************************/

void _get4f(double *ret, double *mat, int row);
void _set4f(double len, double *mat, int row);

/* for keeping track of the ECMA values */
#define ECMAValueTableSize 100
struct ECMAValueStruct ECMAValues[ECMAValueTableSize];
int maxECMAVal = 0;


/*
 * VRML Node types as JS classes:
 */



JSClass SFColorClass = {
	"SFColor",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	SFColorGetProperty,
	SFColorSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_MY_Finalize
};

JSPropertySpec (SFColorProperties)[] = {
	{"r", 0, JSPROP_ENUMERATE},
	{"g", 1, JSPROP_ENUMERATE},
	{"b", 2, JSPROP_ENUMERATE},
	{0}
};

JSFunctionSpec (SFColorFunctions)[] = {
	{"getHSV", SFColorGetHSV, 0},
	{"setHSV", SFColorSetHSV, 0},
	{"toString", SFColorToString, 0},
	{"assign", SFColorAssign, 0},
	{0}
};



JSClass SFColorRGBAClass = {
	"SFColorRGBA",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	SFColorRGBAGetProperty,
	SFColorRGBASetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_MY_Finalize
};

JSPropertySpec (SFColorRGBAProperties)[] = {
	{"r", 0, JSPROP_ENUMERATE},
	{"g", 1, JSPROP_ENUMERATE},
	{"b", 2, JSPROP_ENUMERATE},
	{"a", 3, JSPROP_ENUMERATE},
	{0}
};

JSFunctionSpec (SFColorRGBAFunctions)[] = {
	{"getHSV", SFColorRGBAGetHSV, 0},
	{"setHSV", SFColorRGBASetHSV, 0},
	{"toString", SFColorRGBAToString, 0},
	{"assign", SFColorRGBAAssign, 0},
	{0}
};



JSClass SFImageClass = {
	"SFImage",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	SFImageGetProperty,
	SFImageSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_MY_Finalize
};

JSPropertySpec (SFImageProperties)[] = {
	{"x", 0, JSPROP_ENUMERATE},
	{"y", 1, JSPROP_ENUMERATE},
	{"comp", 2, JSPROP_ENUMERATE},
	{"array", 3, JSPROP_ENUMERATE},
	{0}
};

JSFunctionSpec (SFImageFunctions)[] = {
	{"toString", SFImageToString, 0},
	{"assign", SFImageAssign, 0},
	{0}
};



JSClass SFNodeClass = {
	"SFNode",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	SFNodeGetProperty,
	SFNodeSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	SFNodeFinalize	/* note, this is different, as it contains a string to get rid of */
};

JSPropertySpec (SFNodeProperties)[] = {
	{0}
};

JSFunctionSpec (SFNodeFunctions)[] = {
	{"toString", SFNodeToString, 0},
	{"assign", SFNodeAssign, 0},
	{0}
};



JSClass SFRotationClass = {
	"SFRotation",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	SFRotationGetProperty,
	SFRotationSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_MY_Finalize
};

JSPropertySpec (SFRotationProperties)[] = {
	{"x", 0, JSPROP_ENUMERATE},
	{"y", 1, JSPROP_ENUMERATE},
	{"z", 2, JSPROP_ENUMERATE},
	{"angle",3, JSPROP_ENUMERATE},
	{0}
};

JSFunctionSpec (SFRotationFunctions)[] = {
	{"getAxis", SFRotationGetAxis, 0},
	{"inverse", SFRotationInverse, 0},
	{"multiply", SFRotationMultiply, 0},
	{"multVec", SFRotationMultVec, 0},
	{"setAxis", SFRotationSetAxis, 0},
	{"slerp", SFRotationSlerp, 0},
	{"toString", SFRotationToString, 0},
	{"assign", SFRotationAssign, 0},
	{0}
};



JSClass SFVec2fClass = {
	"SFVec2f",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	SFVec2fGetProperty,
	SFVec2fSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_MY_Finalize
};

JSPropertySpec (SFVec2fProperties)[] = {
	{"x", 0, JSPROP_ENUMERATE},
	{"y", 1, JSPROP_ENUMERATE},
	{0}
};

JSFunctionSpec (SFVec2fFunctions)[] = {
	{"add", SFVec2fAdd, 0},
	{"divide", SFVec2fDivide, 0},
	{"dot", SFVec2fDot, 0},
	{"length", SFVec2fLength, 0},
	{"multiply", SFVec2fMultiply, 0},
	/* {"negate", SFVec2fNegate, 0}, */
	{"normalize", SFVec2fNormalize, 0},
	{"subtract", SFVec2fSubtract, 0},
	{"toString", SFVec2fToString, 0},
	{"assign", SFVec2fAssign, 0},
	{0}
};



JSClass SFVec3fClass = {
	"SFVec3f",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	SFVec3fGetProperty,
	SFVec3fSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_MY_Finalize
};

JSPropertySpec (SFVec3fProperties)[] = {
	{"x", 0, JSPROP_ENUMERATE},
	{"y", 1, JSPROP_ENUMERATE},
	{"z", 2, JSPROP_ENUMERATE},
	{0}
};

JSFunctionSpec (SFVec3fFunctions)[] = {
	{"add", SFVec3fAdd, 0},
	{"cross", SFVec3fCross, 0},
	{"divide", SFVec3fDivide, 0},
	{"dot", SFVec3fDot, 0},
	{"length", SFVec3fLength, 0},
	{"multiply", SFVec3fMultiply, 0},
	{"negate", SFVec3fNegate, 0},
	{"normalize", SFVec3fNormalize, 0},
	{"subtract", SFVec3fSubtract, 0},
	{"toString", SFVec3fToString, 0},
	{"assign", SFVec3fAssign, 0},
	{0}
};


JSClass SFVec3dClass = {
	"SFVec3d",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	SFVec3dGetProperty,
	SFVec3dSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_MY_Finalize
};

JSPropertySpec (SFVec3dProperties)[] = {
	{"x", 0, JSPROP_ENUMERATE},
	{"y", 1, JSPROP_ENUMERATE},
	{"z", 2, JSPROP_ENUMERATE},
	{0}
};

JSFunctionSpec (SFVec3dFunctions)[] = {
	{"add", SFVec3dAdd, 0},
	{"cross", SFVec3dCross, 0},
	{"divide", SFVec3dDivide, 0},
	{"dot", SFVec3dDot, 0},
	{"length", SFVec3dLength, 0},
	{"multiply", SFVec3dMultiply, 0},
	{"negate", SFVec3dNegate, 0},
	{"normalize", SFVec3dNormalize, 0},
	{"subtract", SFVec3dSubtract, 0},
	{"toString", SFVec3dToString, 0},
	{"assign", SFVec3dAssign, 0},
	{0}
};

JSClass MFColorClass = {
	"MFColor",
	JSCLASS_HAS_PRIVATE,
	MFColorAddProperty,
	JS_PropertyStub,
	MFColorGetProperty,
	MFColorSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_MY_Finalize
};

JSFunctionSpec (MFColorFunctions)[] = {
	{"toString", MFColorToString, 0},
	{"assign", MFColorAssign, 0},
	{0}
};



JSClass MFFloatClass = {
	"MFFloat",
	JSCLASS_HAS_PRIVATE,
	MFFloatAddProperty,
	JS_PropertyStub,
	MFFloatGetProperty,
	MFFloatSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_MY_Finalize
};

JSFunctionSpec (MFFloatFunctions)[] = {
	{"toString", MFFloatToString, 0},
	{"assign", MFFloatAssign, 0},
	{0}
};



JSClass MFInt32Class = {
	"MFInt32",
	JSCLASS_HAS_PRIVATE,
	MFInt32AddProperty,
	JS_PropertyStub,
	MFInt32GetProperty,
	MFInt32SetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_MY_Finalize
};

JSFunctionSpec (MFInt32Functions)[] = {
	{"toString", MFInt32ToString, 0},
	{"assign", MFInt32Assign, 0},
	{0}
};



JSClass MFNodeClass = {
	"MFNode",
	JSCLASS_HAS_PRIVATE,
	MFNodeAddProperty,
	JS_PropertyStub,
	MFNodeGetProperty,
	MFNodeSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_MY_Finalize
};

JSFunctionSpec (MFNodeFunctions)[] = {
	{"toString", MFNodeToString, 0},
	{"assign", MFNodeAssign, 0},
	{0}
};



JSClass MFRotationClass = {
	"MFRotation",
	JSCLASS_HAS_PRIVATE,
	MFRotationAddProperty,
	JS_PropertyStub,
	MFRotationGetProperty,
	MFRotationSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_MY_Finalize
};

JSFunctionSpec (MFRotationFunctions)[] = {
	{"toString", MFRotationToString, 0},
	{"assign", MFRotationAssign, 0},
	{0}
};



/* this one is not static. */
JSClass MFStringClass = {
	"MFString",
	JSCLASS_HAS_PRIVATE,
	MFStringAddProperty,
	JS_PropertyStub,
	MFStringGetProperty,
	MFStringSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_MY_Finalize
};

JSFunctionSpec (MFStringFunctions)[] = {
	{"toString", MFStringToString, 0},
	{"assign", MFStringAssign, 0},
	{0}
};



JSClass MFTimeClass = {
	"MFTime",
	JSCLASS_HAS_PRIVATE,
	MFTimeAddProperty,
	JS_PropertyStub,
	MFTimeGetProperty,
	MFTimeSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_MY_Finalize
};

JSPropertySpec (MFTimeProperties)[] = { 
 	{0} 
};

JSFunctionSpec (MFTimeFunctions)[] = {
	{"toString", MFTimeToString, 0},
	{"assign", MFTimeAssign, 0},
	{0}
};



JSClass MFVec2fClass = {
	"MFVec2f",
	JSCLASS_HAS_PRIVATE,
	MFVec2fAddProperty,
	JS_PropertyStub,
	MFVec2fGetProperty,
	MFVec2fSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_MY_Finalize
};

JSFunctionSpec (MFVec2fFunctions)[] = {
	{"toString", MFVec2fToString, 0},
	{"assign", MFVec2fAssign, 0},
	{0}
};



JSClass MFVec3fClass = {
	"MFVec3f",
	JSCLASS_HAS_PRIVATE,
	MFVec3fAddProperty,
	JS_PropertyStub,
	MFVec3fGetProperty,
	MFVec3fSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_MY_Finalize
};

JSFunctionSpec (MFVec3fFunctions)[] = {
	{"toString", MFVec3fToString, 0},
	{"assign", MFVec3fAssign, 0},
	{0}
};

/* VrmlMatrix - JAS */
JSObject *proto_VrmlMatrix;

JSClass VrmlMatrixClass = {
	"VrmlMatrix",
	JSCLASS_HAS_PRIVATE,
	VrmlMatrixAddProperty,
	JS_PropertyStub,
	VrmlMatrixGetProperty,
	VrmlMatrixSetProperty,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_MY_Finalize
};

JSFunctionSpec (VrmlMatrixFunctions)[] = {
	{"toString", VrmlMatrixToString, 0},
	{"assign", VrmlMatrixAssign, 0},
	{"getTransform", VrmlMatrixgetTransform, 0},
	{"setTransform", VrmlMatrixsetTransform, 0},
	{"inverse", VrmlMatrixinverse, 0},
	{"transpose", VrmlMatrixtranspose, 0},
	{"multLeft", VrmlMatrixmultLeft, 0},
	{"multRight", VrmlMatrixmultRight, 0},
	{"multVecMatrix", VrmlMatrixmultVecMatrix, 0},
	{"multMatrixVec", VrmlMatrixmultMatrixVec, 0},
	{0}
};


struct JSLoadPropElement {
	JSClass *class;
	void *constr;
	void *Functions;
	char *id;
};


struct JSLoadPropElement (JSLoadProps) [] = {
        { &SFColorClass, SFColorConstr, &SFColorFunctions, "__SFColor_proto"},
        { &SFVec2fClass, SFVec2fConstr, &SFVec2fFunctions, "__SFVec2f_proto"},
        { &SFColorRGBAClass, SFColorRGBAConstr, &SFColorRGBAFunctions, "__SFColorRGBA_proto"},
        { &SFVec3fClass, SFVec3fConstr, &SFVec3fFunctions, "__SFVec3f_proto"},
        { &SFVec3dClass, SFVec3dConstr, &SFVec3dFunctions, "__SFVec3d_proto"},
        { &SFRotationClass, SFRotationConstr, &SFRotationFunctions, "__SFRotation_proto"},
        { &SFNodeClass, SFNodeConstr, &SFNodeFunctions, "__SFNode_proto"},
        { &MFFloatClass, MFFloatConstr, &MFFloatFunctions, "__MFFloat_proto"},
        { &MFTimeClass, MFTimeConstr, &MFTimeFunctions, "__MFTime_proto"},
        { &MFInt32Class, MFInt32Constr, &MFInt32Functions, "__MFInt32_proto"},
        { &MFColorClass, MFColorConstr, &MFColorFunctions, "__MFColor_proto"},
        { &MFVec2fClass, MFVec2fConstr, &MFVec2fFunctions, "__MFVec2f_proto"},
        { &MFVec3fClass, MFVec3fConstr, &MFVec3fFunctions, "__MFVec3f_proto"},
        { &MFRotationClass, MFRotationConstr, &MFRotationFunctions, "__MFRotation_proto"},
        { &MFNodeClass, MFNodeConstr, &MFNodeFunctions, "__MFNode_proto"},
        { &SFImageClass, SFImageConstr, &SFImageFunctions, "__SFImage_proto"},
/*        { &MFColorRGBAClass, MFColorRGBAConstr, &MFColorRGBAFunctions, "__MFColorRGBA_proto"},*/
        { &MFStringClass, MFStringConstr, &MFStringFunctions, "__MFString_proto"},
        { &VrmlMatrixClass, VrmlMatrixConstr, &VrmlMatrixFunctions, "__VrmlMatrix_proto"},
        {0}
};
/* try and print what an element is in case of error */
void printJSNodeType (JSContext *context, JSObject *myobj) {
	int i;
	i=0;
	
	#ifdef JSVRMLCLASSESVERBOSE
	printf ("printJSNodeType, obj pointer is %u\n",myobj);
	#endif

	/* ok - this is an object, lets find out what class it is */
	while (JSLoadProps[i].class != NULL) {
		if (JS_InstanceOf(context, myobj, JSLoadProps[i].class, NULL)) {
			printf ("%s\n",JSLoadProps[i].id);
			return;
		}
		i++;
	}
}

/* do a simple copy; from, to, and count */
JSBool _simplecopyElements (JSContext *cx,
		JSObject *fromObj,
		JSObject *toObj,
		int count,
		int type) {
	int i;
	jsval val;
	double dd;
	int ii;
        jsdouble *dp;


	#ifdef JSVRMLCLASSESVERBOSE
	printf ("simpleCopyElements, count %d\n",count);
	#endif

	for (i = 0; i < count; i++) {
		if (!JS_GetElement(cx, fromObj, (jsint) i, &val)) {
			printf( "failed in get %d index %d.\n",type, i);
			return JS_FALSE;
		}
		/* ensure that the types are ok */
		if ((type == FIELDTYPE_SFFloat) || (type == FIELDTYPE_SFTime)) {
			/* if we expect a double, and we have an INT... */
			if (JSVAL_IS_INT(val)) {
				ii = JSVAL_TO_INT(val);
				dd = (double) ii;
				/* printf ("integer is %d doulbe %lf\n",ii,dd); */
		                if ((dp = JS_NewDouble(cx,dd)) == NULL) {
                		        printf( "JS_NewDouble failed for %f in simplecopyelements.\n",dd);
                        		return JS_FALSE;
                		}
                		val = DOUBLE_TO_JSVAL(dp);

			}
		}

		/*
		if (JSVAL_IS_OBJECT(val)) printf ("sc, element %d is an OBJECT\n",i);
		if (JSVAL_IS_STRING(val)) printf ("sc, element %d is an STRING\n",i);
		if (JSVAL_IS_NUMBER(val)) printf ("sc, element %d is an NUMBER\n",i);
		if (JSVAL_IS_DOUBLE(val)) printf ("sc, element %d is an DOUBLE\n",i);
		if (JSVAL_IS_INT(val)) printf ("sc, element %d is an INT\n",i);
		*/

		if (!JS_SetElement(cx, toObj, (jsint) i, &val)) {
			printf( "failed in set %s index %d.\n", type, i);
			return JS_FALSE;
		}
	}
	return JS_TRUE;
}


/* make a standard assignment for MF variables */
JSBool _standardMFAssign(JSContext *cx,
	JSObject *obj,
	uintN argc,
	jsval *argv,
	jsval *rval,
	JSClass *myClass,
	int type) {

	JSObject *_from_obj;
	jsval val;
	int32 len;
	char *_id_str;
        SFImageNative *ptr;

	if (!JS_InstanceOf(cx, obj, myClass, argv)) {
		printf("JS_InstanceOf failed for fieldType %s.\n",stringFieldtypeType(type));
		return JS_FALSE;
	}

	if (!JS_ConvertArguments(cx, argc, argv, "o s", &_from_obj, &_id_str)) {
		printf("JS_ConvertArguments failed in %s.\n",stringFieldtypeType(type));
		return JS_FALSE;
	}
	if (!JS_InstanceOf(cx, _from_obj, myClass, argv)) {
		printf("JS_InstanceOf failed for fieldType %s.\n",stringFieldtypeType(type));
		return JS_FALSE;
	}

	if (!JS_GetProperty(cx, _from_obj, "length", &val)) {
		printf("JS_GetProperty failed for \"length\" in %s.\n",stringFieldtypeType(type));
		return JS_FALSE;
	}

	if (!JS_SetProperty(cx, obj, "length", &val)) {
		printf("JS_SetProperty failed for \"length\" in %s\n",stringFieldtypeType(type));
		return JS_FALSE;
	}

	len = JSVAL_TO_INT(val);

	#ifdef JSVRMLCLASSESVERBOSE
		printf("StandardMFAssign %s: obj = %u, id = \"%s\", from = %u, len = %d\n",stringFieldtypeType(type),
		VERBOSE_OBJ obj, _id_str, VERBOSE_OBJ _from_obj, len);
	#endif

	/* copyElements */
	*rval = OBJECT_TO_JSVAL(obj);

	/* SF* values that use this routine - check if we need to set valueChanged in private area */

	if (type == FIELDTYPE_SFImage) {
        	if ((ptr = (SFImageNative *)JS_GetPrivate(cx, obj)) == NULL) {
        	        printf( "JS_GetPrivate failed in standard MF assign.\n");
        	        return;
        	}
		ptr->valueChanged = 1;
	}

	return _simplecopyElements(cx, _from_obj, obj, len,type);
}

/* standardized GetProperty for MF's */
JSBool
_standardMFGetProperty(JSContext *cx,
		JSObject *obj,
		jsval id,
		jsval *vp,
		char *makeNewElement,
		int type) {

	int32 _length, _index;
	jsval _length_val;

	/*  in case we need to run makeNewElement*/
	int newElemenLen;
	jsval newEle;

	#ifdef JSVRMLCLASSESVERBOSE
	printf ("_standardMFGetProperty starting for type %d\n",type);
	printJSNodeType (cx,obj);
	#endif

	if (!JS_GetProperty(cx, obj, "length", &_length_val)) {
		printf( "JS_GetProperty failed for \"length\" in %d.\n",type);
		return JS_FALSE;
	}

	_length = JSVAL_TO_INT(_length_val);
	#ifdef JSVRMLCLASSESVERBOSE
	printf ("standarg get property, len %d\n",_length);
	#endif

	if (JSVAL_IS_INT(id)) {
		_index = JSVAL_TO_INT(id);
		#ifdef JSVRMLCLASSESVERBOSE
		printf ("standard get property, index requested %d\n",_index);
		#endif

		if (_index >= _length) {
			#ifdef JSVRMLCLASSESVERBOSE
			printf ("\n\nconstructing new object\n");
			#endif
			/*  we were making this with C calls, but it would fail with a*/
			/*  segfault; so, now, we run a script to do it.*/


			newElemenLen = strlen(makeNewElement);

			if (!JS_EvaluateScript(cx, obj, makeNewElement, newElemenLen,
				FNAME_STUB, LINENO_STUB, &newEle)) {
				ConsoleMessage ("standardMFGetProperty: JS_EvaluateScript failed for %s", makeNewElement);
				return JS_FALSE;
			}

			*vp = OBJECT_TO_JSVAL(newEle);

			#ifdef JSVRMLCLASSESVERBOSE
			printf ("defining element %d now... is %d %x\n",_index,*vp,*vp);
			#endif

			if (!JS_DefineElement(cx, obj, (jsint) _index, *vp,
				JS_GET_PROPERTY_STUB, JS_SET_PROPERTY_STUB5,
				JSPROP_ENUMERATE)) {
				printf( "JS_DefineElement failed in %d.\n",type);
				return JS_FALSE;
			}

			if (!doMFSetProperty(cx,obj,id,vp,type)) {
				printf ("wow, cant assign property\n");
			}
		}
		#ifdef JSVRMLCLASSESVERBOSE
		printf ("object might already have this index\n");
		#endif
		if (!JS_LookupElement(cx, obj, _index, vp)) {
			printf( "JS_LookupElement failed in %d.\n",type);
			return JS_FALSE;
		}
		if (*vp == JSVAL_VOID) {
			printf( "warning: %d: obj = %u, jsval = %d does not exist!\n",type,
				VERBOSE_OBJ obj, (int) _index);
			return JS_TRUE;
		}
	}
	#ifdef JSVRMLCLASSESVERBOSE
	printf ("_standardMFGetProperty finishing; element is %u\n",*vp);
	#endif

	return JS_TRUE;
}

JSBool doMFToString(JSContext *cx, JSObject *obj, const char *className, jsval *rval)
{
    JSString *_str, *_tmpStr;
    jsval _v;
	char *_buff, *_tmp_valStr, *_tmp_buff;
	const char *_empty_array = "[]";
    int len = 0, i;
	size_t buff_size = 0, tmp_valStr_len = 0, tmp_buff_len = 0, className_len = 0;
	JSBool isString = JS_FALSE;
	JSBool isImage = JS_FALSE;

    if (!JS_GetProperty(cx, obj, "length", &_v)) {
		printf( "JS_GetProperty failed for \"length\" in doMFToString for %s.\n",
				className);
        return JS_FALSE;
	}
	len = JSVAL_TO_INT(_v);

	#ifdef JSVRMLCLASSESVERBOSE
	printf ("doMFToString, obj%u len %d\n",obj, len);
	printJSNodeType (cx,obj);
	#endif

	if (len == 0) {
		_str = JS_NewStringCopyZ(cx, _empty_array);
		*rval = STRING_TO_JSVAL(_str);
		#ifdef JSVRMLCLASSESVERBOSE
		printf ("doMFToString, len is zero, returning JS_TRUE, and %d\n",*rval);
		#endif
		return JS_TRUE;
	}

	className_len = strlen(className);
	if (!strcmp(className, "MFString")) {
		isString = JS_TRUE;
	}
	if (!strcmp(className, "SFImage")) {
		isImage = JS_TRUE;
		#ifdef JSVRMLCLASSESVERBOSE
		printf ("doMFToString - doing an image\n");
		#endif
	}

	buff_size = LARGESTRING;
	_buff = (char *) MALLOC(buff_size * sizeof(char));
	memset(_buff, 0, buff_size);

    for (i = 0; i < len; i++) {
		if (!JS_GetElement(cx, obj, i, &_v)) {
			printf("warning, no element %d of %d in doMFToString for a type of %s.\n",
				i, len,className);
			_tmp_valStr = "NULL";
		} else {

			#ifdef JSVRMLCLASSESVERBOSE
			if (JSVAL_IS_NUMBER(_v)) printf ("is a number\n");
			if (JSVAL_IS_INT(_v)) printf ("is an integer\n");
			if (JSVAL_IS_DOUBLE(_v)) printf ("is an double\n");
			#endif

			_tmpStr = JS_ValueToString(cx, _v);
			if (_tmpStr==NULL) {
				_tmp_valStr = "NULL";
			} else {
				_tmp_valStr = JS_GetStringBytes(_tmpStr);
			}
		}

		#ifdef JSVRMLCLASSESVERBOSE
		printf ("doMFToString, element %d is %d, string %s\n",i,_v,_tmp_valStr);
	
		#endif
		tmp_valStr_len = strlen(_tmp_valStr) + 1;
		tmp_buff_len = strlen(_buff);

		if ((buff_size - (tmp_buff_len + 1)) < (tmp_valStr_len + 1)) {
			buff_size += LARGESTRING;
			if ((_buff =
				 (char *)
				 JS_realloc(cx, _buff, buff_size * sizeof(char *)))
				== NULL) {
				printf( "JS_realloc failed for %d in doMFToString for %s.\n", i, className);
				return JS_FALSE;
			}
		}

		if (len == 1) {
			if (isString) {
				sprintf(_buff, "[ \"%.*s\" ]", tmp_valStr_len, _tmp_valStr);
			} else {
				sprintf(_buff, "[ %.*s ]", tmp_valStr_len, _tmp_valStr);
			}
			break;
		}

		_tmp_buff = (char *) MALLOC((tmp_buff_len + 1) * sizeof(char));
		memset(_tmp_buff, 0, tmp_buff_len + 1);
		memmove(_tmp_buff, _buff, tmp_buff_len);
		memset(_buff, 0, buff_size);

		if (i == 0 && len > 1) {
			if (isString) {
				sprintf(_buff, "[ \"%.*s\"",
						tmp_valStr_len, _tmp_valStr);
			} else {
				sprintf(_buff, "[ %.*s", tmp_valStr_len, _tmp_valStr);
			}
		} else if (i == (len - 1)) {
			if (isString) {
				sprintf(_buff, "%.*s, \"%.*s\" ]",
					tmp_buff_len, _tmp_buff, tmp_valStr_len, _tmp_valStr);
			} else {
				sprintf(_buff, "%.*s, %.*s ]",
					tmp_buff_len, _tmp_buff, tmp_valStr_len, _tmp_valStr);
			}
		} else {
			if (isString) {
				sprintf(_buff, "%.*s, \"%.*s\"",
					tmp_buff_len, _tmp_buff, tmp_valStr_len, _tmp_valStr);
			} else {
				sprintf(_buff, "%.*s, %.*s",
					tmp_buff_len, _tmp_buff, tmp_valStr_len, _tmp_valStr);
			}
		}

		FREE_IF_NZ (_tmp_buff);
    }

	/* PixelTextures are stored in Javascript as MFInt32s but in FreeWRL/Perl as an ascii string.
	   we have to remove some characters to make it into a valid VRML image string */
	if (isImage) {
		for (i=0; i<strlen(_buff); i++) {
			if (_buff[i] == ',') _buff[i]=' ';
			if (_buff[i] == ']') _buff[i]=' ';
			if (_buff[i] == '[') _buff[i]=' ';
		}
	}
	/* printf ("domfstring, buff %s\n",_buff);*/
	_str = JS_NewStringCopyZ(cx, _buff);
	*rval = STRING_TO_JSVAL(_str);

	FREE_IF_NZ (_buff);
    return JS_TRUE;
}


JSBool
doMFAddProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp, char *name) {
	JSString *str;
	jsval v;
	jsval myv;
	char *p;
	size_t p_len = 0;
	int len = 0, ind = JSVAL_TO_INT(id);

	#ifdef JSVRMLCLASSESVERBOSE
		printf("\tdoMFAddProperty:%s id %d NodeType: ",name,id);
		printJSNodeType(cx,obj);
	#endif

	str = JS_ValueToString(cx, id);
	p = JS_GetStringBytes(str);
	#ifdef JSVRMLCLASSESVERBOSE
		printf("\tid string  %s\n ",p);
	#endif

	p_len = strlen(p);
	if (!strcmp(p, "length") ||
		!strcmp(p, "MF_ECMA_has_changed") ||
		!strcmp(p, "_parentField") ||
		!strcmp(p, "toString") ||
		!strcmp(p, "setTransform") ||
		!strcmp(p, "assign") ||
		!strcmp(p, "inverse") ||
		!strcmp(p, "transpose") ||
		!strcmp(p, "multLeft") ||
		!strcmp(p, "multRight") ||
		!strcmp(p, "multVecMatrix") ||
		!strcmp(p, "multMatrixVec") ||
		!strcmp(p, "constructor") ||
		!strcmp(p, "getTransform")) {
		#ifdef JSVRMLCLASSESVERBOSE
			printf("property \"%s\" is one of the standard properties. Do nothing.\n", p);
		#endif
		return JS_TRUE;
	}
	#ifdef JSVRMLCLASSESVERBOSE
		printf("\tdoMFAddProperty:%s id %d NodeType: ",name,id);
		printJSNodeType(cx,obj);
		printf("\tdoMFAddProperty:%s id %d string %s ",name,id,p);
	#endif

	if (!JSVAL_IS_INT(id)){
		printf( "JSVAL_IS_INT failed for id in doMFAddProperty.\n");
		return JS_FALSE;
	}
	if (!JS_GetProperty(cx, obj, "length", &v)) {
		printf( "JS_GetProperty failed for \"length\" in doMFAddProperty.\n");
		return JS_FALSE;
	}

	len = JSVAL_TO_INT(v);
	if (ind >= len) {
		len = ind + 1;

		#ifdef JSVRMLCLASSESVERBOSE
		printf ("doMFAddProperty, len %d ind %d\n",len,ind);
		#endif

		v = INT_TO_JSVAL(len);
		if (!JS_SetProperty(cx, obj, "length", &v)) {
			printf( "JS_SetProperty failed for \"length\" in doMFAddProperty.\n");
			return JS_FALSE;
		}
	}

	#ifdef JSVRMLCLASSESVERBOSE
		printf("index = %d, length = %d\n", ind, len);
	#endif

	myv = INT_TO_JSVAL(1);
	return JS_TRUE;
}


JSBool
doMFSetProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp, int type)
{
	JSString *_sstr;
	jsval myv;
	int i;
	double dd;

        int ii;
        jsdouble *dp;

	JSObject *par;
	JSObject *me;
	jsval pf;
	jsval nf;
	char * _cc;

	#ifdef JSVRMLCLASSESVERBOSE
	JSString *_str;
	char * _c;
		printf ("doMFSetProperty, for object %u, vp %u\n", obj,*vp);
		_str = JS_ValueToString(cx, id);
		_c = JS_GetStringBytes(_str);
		printf ("id is %s\n",_c);

		_sstr = JS_ValueToString(cx, *vp);
		printf ("looking up value for %d %x object %u\n",*vp,*vp,VERBOSE_OBJ obj);
			_cc = JS_GetStringBytes(_sstr);
			printf("\tdoMFSetProperty:%d: obj = %u, id = %s, vp = %s\n",type,
			   VERBOSE_OBJ obj, _c, _cc);
		if (JSVAL_IS_OBJECT(*vp)) { printf ("doMFSet, vp is an OBJECT\n"); }
		if (JSVAL_IS_PRIMITIVE(*vp)) { printf ("doMFSet, vp is an PRIMITIVE\n"); }

		printf ("parent is a "); printJSNodeType(cx,obj);
		/* printf ("jsval is is a "); printJSNodeType(cx,*vp);  */
		
	#endif

	/* should this value be checked for possible conversions */
	if (type == FIELDTYPE_MFInt32) {
		#ifdef JSVRMLCLASSESVERBOSE
		printf ("doMFSetProperty, this should be an int \n");
		#endif

		if (!JSVAL_IS_INT(*vp)) {
			#ifdef JSVRMLCLASSESVERBOSE
			printf ("is NOT an int\n");
			#endif

			if (!JS_ValueToInt32(cx, *vp, &i)) {
				_sstr = JS_ValueToString(cx, *vp);
				_cc = JS_GetStringBytes(_sstr);
				printf ("can not convert %s to an integer in doMFAddProperty for type %d\n",_cc,type);
				return JS_FALSE;
			}

			*vp = INT_TO_JSVAL(i);
		}
	} else if ((type == FIELDTYPE_MFFloat) || (type == FIELDTYPE_MFTime)) {
		#ifdef JSVRMLCLASSESVERBOSE
		printf ("doMFSetProperty - ensure that this is a DOUBLE ");
				_sstr = JS_ValueToString(cx, *vp);
				_cc = JS_GetStringBytes(_sstr);
				printf ("value is  %s \n",_cc);
		#endif

		if (JSVAL_IS_INT(*vp)) {
			ii = JSVAL_TO_INT(*vp);
			dd = (double) ii;
			/* printf ("integer is %d doulbe %lf\n",ii,dd); */
	                if ((dp = JS_NewDouble(cx,dd)) == NULL) {
               		        printf( "JS_NewDouble failed for %f in simplecopyelements.\n",dd);
                       		return JS_FALSE;
               		}
               		*vp = DOUBLE_TO_JSVAL(dp);

		}
	}

	#ifdef JSVRMLCLASSESVERBOSE
	printf ("setting changed flag on %d\n",obj);
	#endif     

	/* is this an MF ECMA type that uses the MF_ECMA_has_changed flag? */
	switch (type) {
		case FIELDTYPE_MFInt32:
		case FIELDTYPE_MFBool:
		case FIELDTYPE_MFTime:
		case FIELDTYPE_MFFloat:
		case FIELDTYPE_MFString: {
			SET_MF_ECMA_HAS_CHANGED
			break;
		}
		default: {}
	}


	if (JSVAL_IS_INT(id)) {
		/* save this element into the parent at index */

		#ifdef JSVRMLCLASSESVERBOSE
		printf ("saving element %d\n",JSVAL_TO_INT(id));
		#endif

		if (!JS_DefineElement(cx, obj, JSVAL_TO_INT(id), *vp,
			JS_GET_PROPERTY_STUB, JS_SET_PROPERTY_STUB5,
			JSPROP_ENUMERATE)) {
			printf( "JS_DefineElement failed in doMFSetProperty.\n");
			return JS_FALSE;
		}

		/* has the length changed? */
		if (!JS_GetProperty(cx, obj, "length", &myv)) {
			printf("JS_GetProperty failed for \"length\" in doMFSetProperty.\n");
			return JS_FALSE;
		}

		#ifdef JSVRMLCLASSESVERBOSE
		printf ("object %u old length %d, possibly new length is going to be %d\n",obj,JSVAL_TO_INT(myv), JSVAL_TO_INT(id)+1);
		#endif

		if (JSVAL_TO_INT(myv) < (JSVAL_TO_INT(id)+1)) {
			printf ("new length is %d\n",JSVAL_TO_INT(id)+1);
			myv = INT_TO_JSVAL(JSVAL_TO_INT(id)+1);
			if (!JS_SetProperty(cx, obj, "length", &myv)) {
				printf("JS_SetProperty failed for \"length\" in doMFSetProperty.\n");
				return JS_FALSE;
			}
		}
	}

	#ifdef JSVRMLCLASSESVERBOSE
	printf ("doMFSetProperty, lets see if we have an SFNode somewhere up the chain...\n");
	#endif

        /* ok - if we are setting an MF* field by a thing like myField[10] = new String(); the
           set method does not really get called. So, we go up the parental chain until we get
           either no parent, or a SFNode. If we get a SFNode, we call the "save this" function
           so that the X3D scene graph gets the updated array value. To make a long story short,
           here's the call to find the parent for the above. */

	me = obj;
	par = JS_GetParent(cx, me);
	while (par != NULL) {
		#ifdef JSVRMLCLASSESVERBOSE
		printf ("for obj %u: ",me);
			printJSNodeType(cx,me);
		printf ("... parent %u\n",par);
			printJSNodeType(cx,par);
		#endif

		if (JS_InstanceOf (cx, par, &SFNodeClass, NULL)) {
			#ifdef JSVRMLCLASSESVERBOSE
			printf (" the parent IS AN SFNODE - it is %u\n",par);
			#endif


			if (!JS_GetProperty(cx, obj, "_parentField", &pf)) {
				printf ("doMFSetProperty, can not get parent field from this object\n");
				return JS_FALSE;
			}

			nf = OBJECT_TO_JSVAL(me);

			#ifdef JSVRMLCLASSESVERBOSE
			printf ("parentField is %u \"%s\"\n", pf, JS_GetStringBytes(JSVAL_TO_STRING(pf)));
			#endif

			if (!setSFNodeField (cx, par, pf, &nf)) {
				printf ("could not set field of SFNode\n");
			}

		}
		me = par;
		par = JS_GetParent(cx, me);
	}

	return JS_TRUE;
}

JSBool
doMFStringUnquote(JSContext *cx, jsval *vp)
{
	JSString *_str, *_vpStr;
	char *_buff, *_tmp_vpStr;
	size_t _buff_len;
	unsigned int i, j = 0;

	_str = JS_ValueToString(cx, *vp);
	_buff = JS_GetStringBytes(_str);
	_buff_len = strlen(_buff) + 1;

	#ifdef JSVRMLCLASSESVERBOSE
		printf("doMFStringUnquote: vp = \"%s\"\n", _buff);
	#endif

	if (memchr(_buff, '"', _buff_len) != NULL) {
		_tmp_vpStr = (char *) MALLOC(_buff_len * sizeof(char));

		memset(_tmp_vpStr, 0, _buff_len);

		for (i = 0; i <= (_buff_len-1); i++) {
			if (_buff[i] != '"' ||
				(i > 0 && _buff[i - 1] == '\\')) {
				_tmp_vpStr[j++] = _buff[i];
			}
		}
		#ifdef JSVRMLCLASSESVERBOSE
		printf ("new unquoted string %s\n",_tmp_vpStr);
		#endif

		_vpStr = JS_NewStringCopyZ(cx, _tmp_vpStr);
		*vp = STRING_TO_JSVAL(_vpStr);

		FREE_IF_NZ (_tmp_vpStr);
	}

	return JS_TRUE;
}


JSBool
globalResolve(JSContext *cx, JSObject *obj, jsval id)
{
	UNUSED(cx);
	UNUSED(obj);
	UNUSED(id);
	return JS_TRUE;
}


/* load the FreeWRL extra classes */
JSBool loadVrmlClasses(JSContext *context, JSObject *globalObj) {
	jsval v;
	int i;

	JSObject *myProto;
	
	i=0;
	while (JSLoadProps[i].class != NULL) {
		#ifdef JSVRMLCLASSESVERBOSE
		printf ("loading %s\n",JSLoadProps[i].id);
		#endif

		v = 0;
		if (( myProto = JS_InitClass(context, globalObj, NULL, JSLoadProps[i].class,
			  JSLoadProps[i].constr, INIT_ARGC, NULL,
			  JSLoadProps[i].Functions, NULL, NULL)) == NULL) {
			printf("JS_InitClass for %s failed in loadVrmlClasses.\n",JSLoadProps[i].id);
			return JS_FALSE;
		}
		v = OBJECT_TO_JSVAL(myProto);
		if (!JS_SetProperty(context, globalObj, JSLoadProps[i].id, &v)) {
			printf("JS_SetProperty for %s failed in loadVrmlClasses.\n",JSLoadProps[i].id);
			return JS_FALSE;
		}

		i++;
	}
	return JS_TRUE;
}

/* go through and see if the setECMA routine touched this name */
int findNameInECMATable(JSContext *context, char *toFind) {
	int i;

	#ifdef JSVRMLCLASSESVERBOSE
	printf ("findNameInECMATable, looking for %s context %u\n",toFind,context);
	#endif
	
	i=0;
	while (i < maxECMAVal) { 
		#ifdef JSVRMLCLASSESVERBOSE
		printf ("	%d: %s==%s cx %u==%u\n",i,ECMAValues[i].name,toFind,ECMAValues[i].context,context);
		#endif

		
		if ((ECMAValues[i].context == context) && (strcmp(ECMAValues[i].name,toFind)==0)) {
			#ifdef JSVRMLCLASSESVERBOSE
			printf ("fineInECMATable: found value at %d\n",i);
			#endif
			return ECMAValues[i].valueChanged;
		}
		i++;
	}
	
	/* did not find this one, add it */
	#ifdef JSVRMLCLASSESVERBOSE
	printf ("findInECMATable - did not find %s\n",toFind);
	#endif

	return FALSE;
}

/* go through the ECMA table, and reset the valueChanged flag. */
void resetNameInECMATable(JSContext *context, char *toFind) {
	int i;

	#ifdef JSVRMLCLASSESVERBOSE
	printf ("findNameInECMATable, looking for %s\n",toFind);
	#endif
	
	i=0;
	while (i < maxECMAVal) { 
		#ifdef JSVRMLCLASSESVERBOSE
		printf ("	%d: %s==%s cx %u==%u\n",i,ECMAValues[i].name,toFind,ECMAValues[i].context,context);
		#endif

		
		if ((ECMAValues[i].context == context) && (strcmp(ECMAValues[i].name,toFind)==0)) {
			#ifdef JSVRMLCLASSESVERBOSE
			printf ("fineInECMATable: found value at %d\n",i);
			#endif
			ECMAValues[i].valueChanged = FALSE;
			return;
		}
		i++;
	}
}

/* set the valueChanged flag - add a new entry to the table if required */
void setInECMATable(JSContext *context, char *toFind) {
	int i;

	#ifdef JSVRMLCLASSESVERBOSE
	printf ("setInECMATable, looking for %s\n",toFind);
	#endif
	
	i=0;
	while (i < maxECMAVal) { 
		#ifdef JSVRMLCLASSESVERBOSE
		printf ("	%d: %s==%s cx %u==%u\n",i,ECMAValues[i].name,toFind,ECMAValues[i].context,context);
		#endif

		
		if ((ECMAValues[i].context == context) && (strcmp(ECMAValues[i].name,toFind)==0)) {
			#ifdef JSVRMLCLASSESVERBOSE
			printf ("setInECMATable: found value at %d\n",i);
			#endif
			ECMAValues[i].valueChanged = TRUE;
			return;
		}
		i++;
	}
	
	/* did not find this one, add it */
	#ifdef JSVRMLCLASSESVERBOSE
	printf ("setInECMATable - new entry at %d for %s\n",maxECMAVal, toFind);
	#endif

	maxECMAVal ++;
	if (maxECMAVal == ECMAValueTableSize) {
		ConsoleMessage ("problem in setInECMATable for scripting\n");
		maxECMAVal = ECMAValueTableSize - 10;
	}
	ECMAValues[maxECMAVal-1].JS_address = (jsval) toFind;
	ECMAValues[maxECMAVal-1].valueChanged = TRUE;
	ECMAValues[maxECMAVal-1].name = strdup(toFind);
	ECMAValues[maxECMAVal-1].context = context;
}

JSBool
setECMANative(JSContext *context, JSObject *obj, jsval id, jsval *vp)
{
	JSString *_idStr;
	JSString *_vpStr, *_newVpStr;
	JSBool ret = JS_TRUE;
	char *_id_c;

	char *_vp_c, *_new_vp_c;
	size_t len = 0;

	_idStr = JS_ValueToString(context, id);
	_id_c = JS_GetStringBytes(_idStr);

        /* "register" this ECMA value for routing changed flag stuff */
       	setInECMATable(context, _id_c);

	if (JSVAL_IS_STRING(*vp)) {
		_vpStr = JS_ValueToString(context, *vp);
		_vp_c = JS_GetStringBytes(_vpStr);

		len = strlen(_vp_c);


		/* len + 3 for '\0' and "..." */
		_new_vp_c = (char *) MALLOC((len + 3) * sizeof(char));
		/* JAS - do NOT prepend/append double quotes to this string*/
		/* JAS - only for the null terminator*/
		/* JAS len += 3;*/
		len += 1;

		memset(_new_vp_c, 0, len);
		/* JAS sprintf(_new_vp_c, "\"%.*s\"", len, _vp_c);*/
		sprintf(_new_vp_c, "%.*s", len, _vp_c);
		_newVpStr = JS_NewStringCopyZ(context, _new_vp_c);
		*vp = STRING_TO_JSVAL(_newVpStr);

		#ifdef JSVRMLCLASSESVERBOSE
			printf("setECMANative: have string obj = %u, id = \"%s\", vp = %s\n",
				   VERBOSE_OBJ obj, _id_c, _new_vp_c);
		#endif
		FREE_IF_NZ (_new_vp_c);
	} else {
		#ifdef JSVRMLCLASSESVERBOSE
		_vpStr = JS_ValueToString(context, *vp);
		_vp_c = JS_GetStringBytes(_vpStr);
		printf("setECMANative: obj = %u, id = \"%s\", vp = %s\n",
			   VERBOSE_OBJ obj, _id_c, _vp_c);
		#endif
	}

	return ret;
}

/* used mostly for debugging */
JSBool
getAssignProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{

	#ifdef JSVRMLCLASSESVERBOSE
	JSString *_idStr, *_vpStr;
	char *_id_c, *_vp_c;
		_idStr = JS_ValueToString(cx, id);
		_id_c = JS_GetStringBytes(_idStr);
		_vpStr = JS_ValueToString(cx, *vp);
		_vp_c = JS_GetStringBytes(_vpStr);
		printf("getAssignProperty: obj = %u, id = \"%s\", vp = %s\n",
			   VERBOSE_OBJ obj, _id_c, _vp_c);
	#endif
	return JS_TRUE;
}


/* a kind of hack to replace the use of JSPROP_ASSIGNHACK */
JSBool
setAssignProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	JSObject *_o;
	JSString *_str;
	const uintN _argc = 2;
	jsval newVal, initVal, _argv[_argc];
	char *_id_c;

	if (JSVAL_IS_STRING(id)) {
		_str = JSVAL_TO_STRING(id);
		_id_c = JS_GetStringBytes(_str);
		if (!JS_ConvertValue(cx, *vp, JSTYPE_OBJECT, &newVal)) {
			printf( "JS_ConvertValue failed in setAssignProperty.\n");
			return JS_FALSE;
		}
		if (!JS_GetProperty(cx, obj, _id_c, &initVal)) {
			printf( "JS_GetProperty failed in setAssignProperty.\n");
			return JS_FALSE;
		}
		#ifdef JSVRMLCLASSESVERBOSE
			printf("setAssignProperty: obj = %u, id = \"%s\", from = %ld, to = %ld\n",
				   VERBOSE_OBJ obj, _id_c, newVal, initVal);

                if (JSVAL_IS_OBJECT(initVal)) printf ("initVal is an OBJECT\n");
                if (JSVAL_IS_STRING(initVal)) printf ("initVal is an STRING\n");
                if (JSVAL_IS_NUMBER(initVal)) printf ("initVal is an NUMBER\n");
                if (JSVAL_IS_DOUBLE(initVal)) printf ("initVal is an DOUBLE\n");
                if (JSVAL_IS_INT(initVal)) printf ("initVal is an INT\n");

                if (JSVAL_IS_OBJECT(newVal)) printf ("newVal is an OBJECT\n");
                if (JSVAL_IS_STRING(newVal)) printf ("newVal is an STRING\n");
                if (JSVAL_IS_NUMBER(newVal)) printf ("newVal is an NUMBER\n");
                if (JSVAL_IS_DOUBLE(newVal)) printf ("newVal is an DOUBLE\n");
                if (JSVAL_IS_INT(newVal)) printf ("newVal is an INT\n");

                if (JSVAL_IS_OBJECT(id)) printf ("id is an OBJECT\n");
                if (JSVAL_IS_STRING(id)) printf ("id is an STRING\n");
                if (JSVAL_IS_NUMBER(id)) printf ("id is an NUMBER\n");
                if (JSVAL_IS_DOUBLE(id)) printf ("id is an DOUBLE\n");
                if (JSVAL_IS_INT(id)) printf ("id is an INT\n");

		printf ("id is %s\n",JS_GetStringBytes(JS_ValueToString(cx,id)));
		printf ("initVal is %s\n",JS_GetStringBytes(JS_ValueToString(cx,initVal)));
		printf ("newVal is %s\n",JS_GetStringBytes(JS_ValueToString(cx,newVal)));

		#endif


		_o = JSVAL_TO_OBJECT(initVal);

		#ifdef xxJSVRMLCLASSESVERBOSE
			printf ("in setAssignProperty, o is %u type ",_o);
			printJSNodeType(cx,_o);
			printf ("\n");
		#endif
 
		_argv[0] = newVal;
		_argv[1] = id;

		if (!JS_CallFunctionName(cx, _o, "assign", _argc, _argv, vp)) {
			printf( "JS_CallFunctionName failed in setAssignProperty.\n");
			return JS_FALSE;
		}
	} else {
		#ifdef JSVRMLCLASSESVERBOSE
			_str = JS_ValueToString(cx, id);
			_id_c = JS_GetStringBytes(_str);
			printf("setAssignProperty: obj = %u, id = \"%s\"\n",
				   VERBOSE_OBJ obj, _id_c);
		#endif
	}

	return JS_TRUE;
}