/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <math.h>

#ifdef AQUA 
#include <gl.h>
#include <glu.h>
#include <glext.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#endif

#include "headers.h"

#include "jsapi.h"
#include "jsUtils.h"
#include "jsNative.h"
#include "jsVRMLClasses.h"

#include "SensInterps.h"

#define FROM_SCRIPT 1
#define TO_SCRIPT 2
#define SCRIPT_TO_SCRIPT 3


void getMFStringtype(JSContext *cx, jsval *from, struct Multi_String *to);
void getMultNumType (JSContext *cx, struct Multi_Vec3f *tn, int eletype);

/*****************************************
C Routing Methodology:

Different nodes produce eventins/eventouts...

	EventOuts only:
		MovieTexture
		AudioClip
		TimeSensor
		TouchSensor
		PlaneSensor
		SphereSensor
		CylinderSensor
		VisibilitySensor
		ProximitySensor
	
	EventIn/EventOuts:
		ScalarInterpolator
		OrientationInterpolator
		ColorInterpolator
		PositionInterpolator
		NormalInterpolator
		CoordinateInterpolator
		Fog
		Background
		Viewpoint
		NavigationInfo
		Collision
	
	EventIns only:
		Almost everything else...


	Nodes with ClockTicks:
		MovieTexture, AudioClip, TimeSensor, 
		ProximitySensor, Collision, ...?

	Nodes that have the EventsProcessed method:
		ScalarInterpolator, OrientationInterpolator,
		ColorInterpolator, PositionInterpolator,
		NormalInterpolator,  (should be all the interpolators)
		.... ??


	

	So, in the event loop, (Events.pm, right now), the call to

		push @e, $_->get_firstevent($timestamp);

	Does all the nodes with clock ticks - ie, it starts off
	generating a series of events.

		push @ne,$_->events_processed($timestamp,$be);
	
	goes through the list of routes, copies the source event to
	the destination, and if the node is one of the EventIn/EventOut
	style, it then re-tells the routing table to do another route.
	The table is gone through until all events are done with.
	
		 

	--------------------------------------------------------------------------	
	C Routes are stored in a table with the following entries:
		Fromnode 	- the node that created an event address
		actual ptr	- pointer to the exact field within the address
		Tonode		- destination node address
		actual ptr	- pointer to the exact field within the address
		active		- True of False for each iteration
		length		- data field length
		interpptr	- pointer to an interpolator node, if this is one



	SCRIPTS handled like this:

		1) a call is made to        CRoutes_js_new (num,cx,glob,brow);
		   with the script number (0 on up), script context, script globals,
		   and browser data.

		2) Initialize called;


		3) scripts that have eventIns have the values copied over and
		   sent to the script by the routine "sendScriptEventIn".

		4) scripts that have eventOuts have the eventOut values copied over
		   and acted upon by the routine "gatherScriptEventOuts".

		
******************************************/
struct CRjsnameStruct {
	int	type;
	char	name[MAXJSVARIABLELENGTH];
};

typedef struct _CRnodeStruct {
	unsigned int node;
	unsigned int foffset;
} CRnodeStruct;

struct CRStruct {
	unsigned int	fromnode;
	unsigned int	fnptr;
	unsigned int tonode_count;
	CRnodeStruct *tonodes;
	int	act;
	int	len;
	void	(*interpptr)(void *);
	int	direction_flag;	/* if non-zero indicates script in/out,
						   proto in/out */
	int	extra;		/* used to pass a parameter (eg, 1 = addChildren..) */
};

/* Routing table */
struct CRStruct *CRoutes;
static int CRoutes_Initiated = FALSE;
int CRoutes_Count;
int CRoutes_MAX;

/* Structure table */
struct CRjsStruct *JSglobs = 0; 	/* global objects and contexts for each script */
int *scr_act = 0;			/* this script has been sent an eventIn */
int scripts_active;		/* a script has been sent an eventIn */
int max_script_found = -1;	/* the maximum script number found */

/* Script name/type table */
struct CRjsnameStruct *JSparamnames = 0;
int jsnameindex = -1;
int MAXJSparamNames = 0;

/* EAI needs the extra parameter, so we put it globally when a RegisteredListener is clicked. */
int CRoutesExtra = 0;

int CRVerbose = 0;

/* global return value for getting the value of a variable within Javascript */
jsval global_return_val;

/* ClockTick structure for processing all of the initevents - eg, TimeSensors */
struct FirstStruct {
	unsigned int	tonode;
	void (*interpptr)(unsigned *);
};

/* ClockTick structure and counter */
struct FirstStruct *ClockEvents = 0;
int num_ClockEvents = 0;


/****************************************************************************/
/*									    */
/* get_touched_flag - see if this variable (can be a sub-field; see tests   */
/* 8.wrl for the DEF PI PositionInterpolator). return true if variable is   */
/* touched, and pointer to touched value is in global variable              */
/* global_return_val							    */
/*                                                                          */
/****************************************************************************/

int get_touched_flag (int fptr, int actualscript) {
	char fullname[100];
	char tmethod[100];
	jsval v, retval, retval2;
	jsval interpobj;
        JSString *strval; /* strings */
	char *strtouched;
	int intval = 0;
	int touched_function;


	int index, locindex;
	int len;
	int complex_name; /* a name with a period in it */
	char *myname;

	if (JSVerbose) 
		printf ("\nget_touched_flag, name %s script %d context %#x \n",JSparamnames[fptr].name,
				actualscript,JSglobs[actualscript].cx);

	myname = JSparamnames[fptr].name;
	len = strlen(myname);
	index = 0;
	interpobj = JSglobs[actualscript].glob;
	complex_name = (strstr(myname,".") != NULL);
	fullname[0] = 0;


	// if this is a complex name (ie, it is like a field of a field) then get the
	// first part. 
	if (complex_name) {
		// get first part, and convert it into a handle name.
		locindex = 0;
		while (*myname!='.') {
			tmethod[locindex] = *myname;
			locindex++; 
			myname++;
		}
		tmethod[locindex] = 0;
		myname++;

		//printf ("getting intermediate value by using %s\n",tmethod);
		 if (!JS_GetProperty((JSContext *) JSglobs[actualscript].cx, (JSObject *) interpobj,tmethod,&retval)) {
			printf ("cant get property for name %s\n",tmethod);
			return FALSE;
		} else {
               		strval = JS_ValueToString((JSContext *)JSglobs[actualscript].cx, retval);
                	strtouched = JS_GetStringBytes(strval);
                	//printf ("interpobj %d and getproperty returns %s\n",retval,strtouched);
		}
		strcpy (fullname,strtouched);
		strcat (fullname,"_");
	}

	// now construct the varable name; it might have a prefix as found above.

	//printf ("before constructor, fullname is %s\n",fullname);
	strcat (fullname,myname);
	touched_function = FALSE;

	// Find out the method of getting the touched flag from this variable type

	/* Multi types */
	switch (JSparamnames[fptr].type) {
	case MFFLOAT: case MFTIME: case MFINT32: case MFCOLOR:
	case MFROTATION: case MFNODE: case MFVEC2F: 
	case MFSTRING: {
		strcpy (tmethod,"__touched_flag");
		complex_name = TRUE;
		break;
		}
	
	/* ECMAScriptNative types */
	case SFBOOL: case SFFLOAT: case SFTIME: case SFINT32: case SFSTRING: {
		if (complex_name) strcpy (tmethod,"_touched");
		else sprintf (tmethod, "_%s_touched",fullname);
		break;
		}

	case SFCOLOR:
	case SFNODE: case SFROTATION: case SFVEC2F: {
		if (complex_name) strcpy (tmethod,"__touched()");
		else sprintf (tmethod, "%s.__touched()",fullname);
		touched_function = TRUE;
		break;
		}
	default: {
		printf ("WARNING, this type (%d) not handled yet\n",
			JSparamnames[fptr].type);
		return FALSE;
		}
	}

	// get the property value, if we can
	//printf ("getting property for fullname %s\n",fullname);
	if (!JS_GetProperty((JSContext *) JSglobs[actualscript].cx, (JSObject *) interpobj ,fullname,&retval)) {
               	printf ("cant get property for %s\n",fullname);
		return FALSE;
        } else {
       	        strval = JS_ValueToString((JSContext *)JSglobs[actualscript].cx, retval);
               	strtouched = JS_GetStringBytes(strval);
               	//printf ("and get of actual property %d returns %s\n",retval,strtouched);

		// this can be undefined, as the associated route is created if there is a DEF
		// node in the parameter list, and the function does not touch this node/field.
		// if this is the case, just ignore it.
		if (strcmp("undefined",strtouched)==0) {
			//printf ("abnormal return here\n");
			return FALSE;
		}

		// Save this value for later parsing
		global_return_val = retval;
	}


	// Now, for the Touched (and thus the return) value 
	if (touched_function) {
		//printf ("Function, have to run script\n");
	
		if (!ActualrunScript(actualscript, tmethod ,&retval)) 
			printf ("failed to get touched, line %s\n",tmethod);

		if (JSVAL_IS_INT(retval)) {
			intval = JSVAL_TO_INT(retval);
			return (intval!=0);
		}
		return FALSE; // should never get here
	}


	// now, if this is a complex name, we get property relative to what was before;
	// if not (ie, this is a standard, simple, name, use the object as before
	if (complex_name) {
		interpobj = retval;
	}	

	//printf ("using touched method %s on %d %d\n",tmethod,JSglobs[actualscript].cx,interpobj);

	if (!JS_GetProperty((JSContext *) JSglobs[actualscript].cx, (JSObject *) interpobj ,tmethod,&retval2)) {
               	printf ("cant get property for %s\n",tmethod);
		return FALSE;
        } else {
       	        //strval = JS_ValueToString((JSContext *)JSglobs[actualscript].cx, retval2);
               	//strtouched = JS_GetStringBytes(strval);
               	//printf ("and getproperty 3 %d returns %s\n",retval2,strtouched);

		if (JSVAL_IS_INT(retval2)) {
			intval = JSVAL_TO_INT(retval2);
		}

		// set it to 0 now.
		v = INT_TO_JSVAL(0);
		JS_SetProperty ((JSContext *) JSglobs[actualscript].cx, (JSObject *) interpobj, tmethod, &v);

		return (intval!=0);

	}
	return FALSE; // should never get here
}

void set_one_ECMAtype (int tonode, int toname, int dataType, void *Data, unsigned datalen) {

	char scriptline[100];
	jsval retval;
	float fl;
	double dl;
	int il;
	int intval = 0;

	//printf ("set_one_ECMAtype, to %d namepointer %d, fieldname %s, datatype %d length %d\n",
	//	tonode,toname,JSparamnames[toname].name,dataType,datalen);

	switch (dataType) {
		case SFBOOL:	{	/* SFBool */
			memcpy ((void *) &intval,Data, datalen);
			if (intval == 1) sprintf (scriptline,"__tmp_arg_%s=true",JSparamnames[toname].name);
			else sprintf (scriptline,"__tmp_arg_%s=false",JSparamnames[toname].name);
			break;
		}

		case SFFLOAT:	{
			memcpy ((void *) &fl, Data, datalen);
			sprintf (scriptline,"__tmp_arg_%s=%f", JSparamnames[toname].name,fl);
			break;
		}
		case SFTIME:	{
			memcpy ((void *) &dl, Data, datalen);
			sprintf (scriptline,"__tmp_arg_%s=%f", JSparamnames[toname].name,dl);
			break;
		}
		case SFNODE:
		case SFINT32:	{ /* SFInt32 */
			memcpy ((void *) &il,Data, datalen);
			sprintf (scriptline,"__tmp_arg_%s=%d", JSparamnames[toname].name,il);
			break;
		}
		default: {	printf("WARNING: SHOULD NOT BE HERE! %d\n",JSparamnames[toname].type);
		}
	}

	/* set property */
	if (!ActualrunScript(tonode, scriptline ,&retval)) 
		printf ("failed to set parameter, line %s\n",scriptline);

	/* ECMAScriptNative SF nodes require a touched=0 */
	sprintf (scriptline,"___tmp_arg_%s__touched=0", JSparamnames[toname].name);
	if (!ActualrunScript(tonode, scriptline ,&retval)) 
		printf ("failed to set parameter, line %s\n",scriptline);


	/* and set the value */
	sprintf (scriptline,"%s(__tmp_arg_%s,%f)",
			 JSparamnames[toname].name,JSparamnames[toname].name,
			 TickTime);
	if (!ActualrunScript(tonode, scriptline ,&retval)) {
		printf ("failed to set parameter, line %s\n",scriptline);
	}
}


/* sets a SFBool, SFFloat, SFTime, SFIint32, SFString in a script */
void setECMAtype (int num) {
	int fn, tn, tptr;
	int len;
	unsigned int to_counter;
	CRnodeStruct *to_ptr = NULL;

	fn = (int) CRoutes[num].fromnode + (int) CRoutes[num].fnptr;
	len = CRoutes[num].len;
	
	for (to_counter = 0; to_counter < CRoutes[num].tonode_count; to_counter++) {
		to_ptr = &(CRoutes[num].tonodes[to_counter]);
		tn = (int) to_ptr->node;
		tptr = (int) to_ptr->foffset;

		set_one_ECMAtype (tn, tptr, JSparamnames[tptr].type, (void *)fn,(unsigned) len);
	}
}

/* Verify that this structure points to a series of SvPVs not
 * SvPVMGs or anything like that If it does, convert it to a SvPV */

void verifySVtype(struct Multi_String *to) {
	int i;
	SV **svptr;
	struct STRUCT_SV *newSV;

	svptr = to->p;
	//printf ("\n verifySVtype old structure: %d %d\n",svptr,to->n);
	for (i=0; i<(to->n); i++) {
		//printf ("indx %d flag %x string :%s: len1 %d len2 %d\n",i,
		//		(svptr[i])->sv_flags,
		//		 SvPVX(svptr[i]), SvCUR(svptr[i]), SvLEN(svptr[i]));

		if (SvFLAGS(svptr[i]) |= SVt_PVMG) {
			//printf ("have to convert element %d\n",i);
			newSV = malloc (sizeof (struct STRUCT_SV));
			/* copy over old to new */
			newSV->sv_flags = SVt_PV | SVf_POK;
			newSV->sv_refcnt = 1;
			newSV->sv_any = (svptr[i])->sv_any;
			free (svptr[i]);
			svptr[i] = newSV;
		}
	}
	//printf ("done verifySVtype\n");
}

/****************************************************************/
/* a script is returning a MFString type; add this to the C	*/
/* children field						*/
/****************************************************************/
void getMFStringtype (JSContext *cx, jsval *from, struct Multi_String *to) {
	int xx;
	int oldlen, newlen;
	jsval _v;
	JSObject *obj;
	int i;
	char *valStr, *OldvalStr;
	SV **svptr;
	SV *tmpSV;
	SV **newp, **oldp;
	int myv;
	int count;
	struct xpv *mypv;

	JSString *strval; /* strings */

	/* oldlen = what was there in the first place */
	verifySVtype(to);
	
	oldlen = to->n;
	svptr = to->p;
	newlen=0;

	if (!JS_ValueToObject(cx, (jsval) from, &obj)) 
		printf ("JS_ValueToObject failed in getMFStringtype\n");

	if (!JS_GetProperty(cx, obj, "length", &_v)) {
		printf ("JS_GetProperty failed for \"length\" in getMFStringtype.\n");
        }

	newlen = JSVAL_TO_INT(_v);

	// if we have to expand size of SV...
	if (newlen > oldlen) {
		oldp = to->p; // same as svptr, assigned above
		to->n = newlen;
		to->p = malloc(newlen * sizeof(to->p));
		newp = to->p; 

		// copy old values over
		for (count = 0; count <oldlen; count ++) {
			//printf ("copying over element %d\n",count);
			*newp = *oldp;
			newp+= sizeof (to->p);
			oldp+= sizeof (to->p);	
		}
		
		// zero new entries
		for (count = oldlen; count < newlen; count ++) {
			/* make the new SV */
			*newp = malloc (sizeof (struct STRUCT_SV));
			(*newp)->sv_flags = SVt_PV | SVf_POK;
			(*newp)->sv_refcnt=1;
			mypv = malloc(sizeof (struct xpv));
			(*newp)->sv_any = mypv;

			/* now, make it point to a blank string */
			(*mypv).xpv_pv = malloc (2);
			strcpy((*mypv).xpv_pv ,"");
			(*mypv).xpv_cur = 0;
			(*mypv).xpv_len = 1;
		}
		free (svptr);
		svptr = to->p;
	}
	//printf ("verifying structure here\n");
	//for (i=0; i<(to->n); i++) {
	//	printf ("indx %d flag %x string :%s: len1 %d len2 %d\n",i,
	//			(svptr[i])->sv_flags,
	//			 SvPVX(svptr[i]), SvCUR(svptr[i]), SvLEN(svptr[i]));
	//}
	//printf ("done\n");


	for (i = 0; i < newlen; i++) {
		// get the old string pointer
		OldvalStr = SvPVX(svptr[i]);
		//printf ("old string at %d is %s len %d\n",i,OldvalStr,strlen(OldvalStr));

		// get the new string pointer
		if (!JS_GetElement(cx, obj, i, &_v)) {
			fprintf(stderr,
				"JS_GetElement failed for %d in getMFStringtype\n",i);
			return;
		}
		strval = JS_ValueToString(cx, _v);
		valStr = JS_GetStringBytes(strval);

		//printf ("new string %d is %s\n",i,valStr);

		// if the strings are different...
		if (strncmp(valStr,OldvalStr,strlen(valStr)) != 0) {
			// now Perl core dumps since this is the wrong thread, so lets do this 
			// ourselves: sv_setpv(svptr[i],valStr);

			// get a pointer to the xpv to modify
			mypv = SvANY(svptr[i]);

			// free the old string
			free (mypv->xpv_pv); 

			// malloc a new string, of correct len for terminator
			mypv->xpv_pv = malloc (strlen(valStr)+2);

			// copy string over
			strcpy (mypv->xpv_pv, valStr);

			// and tell us that it is now longer
			mypv->xpv_len = strlen(valStr)+1;
			mypv->xpv_cur = strlen(valStr)+0;
		}
	}
	//printf ("\n new structure: %d %d\n",svptr,newlen);
	//for (i=0; i<newlen; i++) {
	//	printf ("indx %d string :%s: len1 %d len2 %d\n",i,
	//			//mypv->xpv_pv, mypv->xpv_cur,mypv->xpv_len);
	//			 SvPVX(svptr[i]), SvCUR(svptr[i]), SvLEN(svptr[i]));
	//}

	myv = INT_TO_JSVAL(1);
	if (!JS_SetProperty(cx, obj, "__touched_flag", (jsval *)&myv)) {
		fprintf(stderr,
			"JS_SetProperty failed for \"__touched_flag\" in doMFAddProperty.\n");
	}
}


/************************************************************************/
/* a script is returning a MFNode type; add or remove this to the C	*/
/* children field							*/
/************************************************************************/

void getMFNodetype (char *strp, struct Multi_Node *par, int ar) {
	unsigned int newptr;
	int oldlen, newlen;
	char *cptr;
	void *newmal;
	unsigned int *tmpptr;

	unsigned int *remptr;
	unsigned int remchild;
	int num_removed;
	int counter;

	if (CRVerbose) {
		printf ("getMFNodetype, %s ar %d\n",strp,ar);
		printf ("getMFNodetype, parent %d has %d nodes currently\n",(int)par,par->n); 
	}

	/* oldlen = what was there in the first place */
	oldlen = par->n;
	newlen=0;

	/* this string will be in the form "[ CNode addr CNode addr....]" */
	/* count the numbers to add  or remove */
	if (*strp == '[') {
		strp++;
	}
	while (*strp == ' ') strp++; /* skip spaces */
	cptr = strp;

	while (sscanf (cptr,"%d",&newptr) == 1) {
		newlen++;
		/* skip past this number */
		while (isdigit(*cptr) || (*cptr == ',') || (*cptr == '-')) cptr++;
		while (*cptr == ' ') cptr++; /* skip spaces */
	}
	cptr = strp; /* reset this pointer to the first number */

	if (ar != 0) {
		/* addChildren - now we know how many SFNodes are in this MFNode, lets malloc and add */

		newmal = malloc ((oldlen+newlen)*sizeof(unsigned int));
	
		if (newmal == 0) {
			printf ("cant malloc memory for addChildren");
			return;
		}
	
		/* copy the old stuff over */
		if (oldlen > 0) memcpy (newmal,par->p,oldlen*sizeof(unsigned int));
	
		/* set up the C structures for this new MFNode addition */
		free (par->p);
		par->p = newmal;
		par->n = oldlen+newlen;
	
		newmal = (void *) ((int) newmal + sizeof (unsigned int) * oldlen);
	
		while (sscanf (cptr,"%d", (int *)newmal) == 1) {
			/* skip past this number */
			while (isdigit(*cptr) || (*cptr == ',') || (*cptr == '-')) cptr++;
			while (*cptr == ' ') cptr++; /* skip spaces */
			newmal = (void *) ((int)newmal + sizeof (unsigned int));
		}

	} else {
		/* this is a removeChildren */

		/* go through the original array, and "zero" out children that match one of
		   the parameters */

		num_removed = 0;
		while (sscanf (cptr,"%d", &remchild) == 1) {
			/* skip past this number */
			while (isdigit(*cptr) || (*cptr == ',') || (*cptr == '-')) cptr++;
			while (*cptr == ' ') cptr++; /* skip spaces */

			remptr = (int *)par->p;
			for (counter = 0; counter < par->n; counter ++) {
				if (*remptr == remchild) {
					*remptr = 0;  /* "0" can not be a valid memory address */
					num_removed ++;
				}
				remptr ++;
			}
		}

		if (num_removed > 0) {
			newmal = malloc ((oldlen-num_removed)*sizeof(unsigned int));
			tmpptr = newmal;
			remptr = (int *)par->p;
			if (newmal == 0) {
				printf ("cant malloc memory for removeChildren");
				return;
			}

			/* go through and copy over anything that is not zero */
			for (counter = 0; counter < par->n; counter ++) {
				if (*remptr != 0) {
					*tmpptr = *remptr;
					tmpptr ++;
				}
				remptr ++;
			}

			free (par->p);
			par->p = newmal;
			par->n = oldlen - num_removed;
		}
	}
}


/****************************************************************/
/* a script is returning a Multi-number type; copy this from 	*/
/* the script return string to the data structure within the	*/
/* freewrl C side of things.					*/
/*								*/
/* note - this cheats in that the code assumes that it is 	*/
/* a series of Multi_Vec3f's while in reality the structure	*/
/* of the multi structures is the same - so we "fudge" things	*/
/* to make this multi-purpose.					*/
/* eletype switches depending on:				*/
/* 	0: MFINT32						*/
/* 	1: MFFLOAT						*/
/* 	2: MFVEC2F						*/
/* 	3: MFCOLOR						*/
/* 	4: MFROTATION						*/
/*	5: MFTIME						*/
/****************************************************************/

void getMultNumType (JSContext *cx, struct Multi_Vec3f *tn, int eletype) {
	float *fl;
	int *il;
	double *dl;

	float f2, f3, f4;
/* 	int shouldfind; */
	jsval mainElement/* , subElement */;
	int len/* , len2 */;
	int i/* , j */;
	JSString *_tmpStr;
	char *strp;
	int elesize;

	/* get size of each element, used for mallocing memory */
	if (eletype == 0) elesize = sizeof (int);		// integer
	else if (eletype == 5) elesize = sizeof (double);	// doubles.
	else elesize = sizeof (float)*eletype;			// 1, 2, 3 or 4 floats per element.

	/* rough check of return value */
	if (!JSVAL_IS_OBJECT(global_return_val)) {
		if (JSVerbose) printf ("getMultNumType - did not get an object\n");
		return;
	}

	//printf ("getmultielementtypestart, tn %d %#x dest has  %d size %d\n",tn,tn,eletype, elesize);

	if (!JS_GetProperty(cx, (JSObject *)global_return_val, "length", &mainElement)) {
		printf ("JS_GetProperty failed for \"length\" in getMultNumType\n");
		return;
	}
	len = JSVAL_TO_INT(mainElement);
	//printf ("getmuiltie length of grv is %d old len is %d\n",len,tn->n);

	/* do we have to realloc memory? */
	if (len != tn->n) {
		/* yep... */
			// printf ("old pointer %d\n",tn->p);
		if (tn->p != NULL) free (tn->p);
		tn->p = malloc ((unsigned)(elesize*len));
		if (tn->p == NULL) {
			printf ("can not malloc memory in getMultNumType\n");
			return;
		}
		tn->n = len;
	}

	/* set these three up, but we only use one of them */
	fl = (float *) tn->p;
	il = (int *) tn->p;
	dl = (double *) tn->p;

	/* go through each element of the main array. */
	for (i = 0; i < len; i++) {
		if (!JS_GetElement(cx, (JSObject *)global_return_val, i, &mainElement)) {
			printf ("JS_GetElement failed for %d in getMultNumType\n",i);
			return;
		}

                _tmpStr = JS_ValueToString(cx, mainElement);
		strp = JS_GetStringBytes(_tmpStr);
                //printf ("sub element %d is %s as a string\n",i,strp);

		switch (eletype) {
		case 0: { sscanf(strp,"%d",il); il++; break;}
		case 1: { sscanf(strp,"%f",fl); fl++; break;}
		case 2: { sscanf (strp,"%f %f",fl,&f2);
			fl++; *fl=f2; fl++; break;}
		case 3: { sscanf (strp,"%f %f %f",fl,&f2,&f3);
			fl++; *fl=f2; fl++; *fl=f3; fl++; break;}
		case 4: { sscanf (strp,"%f %f %f %f",fl,&f2,&f3,&f4);
			fl++; *fl=f2; fl++; *fl=f3; fl++; *fl=f4; fl++; break;}
		case 5: {sscanf (strp,"%lf",dl); dl++; break;}

		default : {printf ("getMultNumType unhandled eletype: %d\n",
				eletype);
			   return;
			}
		}
		
	}
}


/****************************************************************/
/* sets a SFVec3f and SFColor in a script 			*/
/* sets a SFRotation and SFVec2fin a script 			*/
/*								*/
/* all *Native types have the same structure of the struct -	*/
/* we are just looking for the pointer, thus we can handle	*/
/* multi types here 						*/
/* sets a SFVec3f and SFColor in a script 			*/
/****************************************************************/

/* really do the individual set; used by script routing and EAI sending to a script */
void Set_one_MultiElementtype (int tonode, int tnfield, void *Data, unsigned dataLen ) {

	char scriptline[100];
	jsval retval;
	SFVec3fNative *_privPtr; 

	JSContext *_context;
	JSObject *_globalObj, *_sfvec3fObj;


	/* get context and global object for this script */
	_context = (JSContext *) JSglobs[tonode].cx;
	_globalObj = (JSObject *)JSglobs[tonode].glob;


	/* make up the name */
	sprintf (scriptline,"__tmp_arg_%s", JSparamnames[tnfield].name);

	if (CRVerbose) printf ("script %d line %s\n",tonode, scriptline);

	if (!JS_GetProperty(_context,_globalObj,scriptline,&retval)) 
		printf ("JS_GetProperty failed in jsSFVec3fSet.\n");

	if (!JSVAL_IS_OBJECT(retval)) 
		printf ("jsSFVec3fSet - not an object\n");

	_sfvec3fObj = JSVAL_TO_OBJECT(retval);

	if ((_privPtr = JS_GetPrivate(_context, _sfvec3fObj)) == NULL) 
		printf("JS_GetPrivate failed in jsSFVec3fSet.\n");

	/* copy over the data from the perl/C VRML side into the script. */
	memcpy ((void *) &_privPtr->v,Data, dataLen);

	_privPtr->touched = 0;

	/* now, runscript to tell it that it has been touched */
	sprintf (scriptline,"__tmp_arg_%s.__touched()", JSparamnames[tnfield].name);
	if (!ActualrunScript(tonode, scriptline ,&retval)) 
		printf ("failed to set parameter, line %s\n",scriptline);

	/* and run the function */
	sprintf (scriptline,"%s(__tmp_arg_%s,%f)",
			 JSparamnames[tnfield].name,JSparamnames[tnfield].name,
			 TickTime);
	if (!ActualrunScript(tonode, scriptline ,&retval)) {
		printf ("failed to set parameter, line %s\n",scriptline);
	}
}


void setMultiElementtype (int num) {
	int fn, fptr, tn, tptr;
	int len;
	unsigned int to_counter;
	CRnodeStruct *to_ptr = NULL;

	JSContext *_context;
	JSObject *_globalObj;

	fn = (int) CRoutes[num].fromnode; 
	fptr = (int) CRoutes[num].fnptr;
	len = CRoutes[num].len;
	
	for (to_counter = 0; to_counter < CRoutes[num].tonode_count; to_counter++) {
		to_ptr = &(CRoutes[num].tonodes[to_counter]);
		tn = (int) to_ptr->node;
		tptr = (int) to_ptr->foffset;

		if (CRVerbose) {
			printf ("got a script event! index %d type %d\n",
					num, CRoutes[num].direction_flag);
			printf ("\tfrom %#x from ptr %#x\n\tto %#x toptr %#x\n",fn,fptr,tn,tptr);
			printf ("\tdata length %d\n",len);
			printf ("setMultiElementtype here tn %d tptr %d len %d\n",tn, tptr,len);
		}

		/* get context and global object for this script */
		_context = (JSContext *) JSglobs[tn].cx;
		_globalObj = (JSObject *)JSglobs[tn].glob;
		fn += fptr;
		Set_one_MultiElementtype (tn, tptr, (void *)fn, (unsigned)len);
	}
}


void setMFElementtype (int num) {
	int fn, fptr, tn, tptr;
	int len;
	unsigned int to_counter;
	CRnodeStruct *to_ptr = NULL;
	char scriptline[2000];
	char sline[100];
	jsval retval;
	int x;
	int elementlen;
	int pptr;
	float *fp;
	int *ip;
	double *dp;

	JSContext *_context;
	JSObject *_globalObj;

	fn = (int) CRoutes[num].fromnode; 
	fptr = (int) CRoutes[num].fnptr;
	pptr = fn + fptr;
	len = CRoutes[num].len;
	
	for (to_counter = 0; to_counter < CRoutes[num].tonode_count; to_counter++) {
		to_ptr = &(CRoutes[num].tonodes[to_counter]);
		tn = (int) to_ptr->node;
		tptr = (int) to_ptr->foffset;

		if (CRVerbose) {
			printf ("got a script event! index %d type %d\n",
					num, CRoutes[num].direction_flag);
			printf ("\tfrom %#x from ptr %#x\n\tto %#x toptr %#x\n",fn,fptr,tn,tptr);
			printf ("\tdata length %d\n",len);
			printf ("and, sending it to %s\n",JSparamnames[tptr].name);
		}

		/* get context and global object for this script */
		_context = (JSContext *) JSglobs[tn].cx;
		_globalObj = (JSObject *)JSglobs[tn].glob;

		/* make uep the name */
		sprintf (scriptline,"%s(",JSparamnames[tptr].name);
		switch (JSparamnames[tptr].type) {
			case MFCOLOR: {
					      strcat (scriptline, "new MFColor(");
					      elementlen = sizeof (float) * 3;
					      for (x=0; x<(len/elementlen); x++) {
						      fp = pptr;
						      sprintf (sline,"%f %f %f",*fp,
								      *(fp+elementlen),
								      *(fp+(elementlen*2)),
									      *(fp+(elementlen*3)));
						      if (x < ((len/elementlen)-1)) {
							      strcat(sline,",");
						      }
						     pptr += elementlen;
							strcat (scriptline,sline);
					      }
					      break;
				      }
			case MFFLOAT: {		
					      strcat (scriptline, "new MFFloat(");
					      elementlen = sizeof (float);
					      for (x=0; x<(len/elementlen); x++) {
						      fp = pptr;
						      sprintf (sline,"%f",*fp);
						      if (x < ((len/elementlen)-1)) {
							      strcat(sline,",");
						      }
						     pptr += elementlen;
							strcat (scriptline,sline);
					      }
						break;
				      }
			case MFTIME:  {
					      strcat (scriptline, "new MFTime(");
					      elementlen = sizeof (double);
					      for (x=0; x<(len/elementlen); x++) {
						      dp = pptr;
						      sprintf (sline,"%lf",*dp);
						      if (x < ((len/elementlen)-1)) {
							      strcat(sline,",");
						      }
						     pptr += elementlen;
							strcat (scriptline,sline);
					      }
					      break;
				      }
			case MFINT32: {	
					      strcat (scriptline, "new MFInt32(");
					      elementlen = sizeof (int);
					      for (x=0; x<(len/elementlen); x++) {
						      ip = pptr;
						      sprintf (sline,"%d",*ip);
						      if (x < ((len/elementlen)-1)) {
							      strcat(sline,",");
						      }
						     pptr += elementlen;
							strcat (scriptline,sline);
					      }
					      break;
				      }
			case MFSTRING:{	
					      strcat (scriptline, "new MFString(");
					      elementlen = sizeof (float);
					      printf ("ScriptAssign, MFString probably broken\n");
					      for (x=0; x<(len/elementlen); x++) {
						      fp = pptr;
						      sprintf (sline,"%f",*fp);
						      if (x < ((len/elementlen)-1)) {
							      strcat(sline,",");
						      }
						     pptr += elementlen;
							strcat (scriptline,sline);
					      }
					      break;
				      }
			case MFNODE:  {	
					      strcat (scriptline, "new MFNode(");
					      elementlen = sizeof (int);
					      for (x=0; x<(len/elementlen); x++) {
						      ip = pptr;
						      sprintf (sline,"%u",*ip);
						      if (x < ((len/elementlen)-1)) {
							      strcat(sline,",");
						      }
						     pptr += elementlen;
							strcat (scriptline,sline);
					      }
					      break;
				      }
			case MFROTATION: {	strcat (scriptline, "new MFRotation(");
					      elementlen = sizeof (float)*4;
					      for (x=0; x<(len/elementlen); x++) {
						      fp = pptr;
						      sprintf (sline,"%f %f %f %f",*fp,
								*(fp+elementlen),
								*(fp+(elementlen*2)),
								*(fp+(elementlen*3)),
								*(fp+(elementlen*4)));
						      sprintf (sline,"%f",*fp);
						      if (x < ((len/elementlen)-1)) {
							      strcat(sline,",");
						      }
						     pptr += elementlen;
							strcat (scriptline,sline);
					      }
						 break;
					 }
			default: {
					 printf ("setMFElement, SHOULD NOT DISPLAY THIS\n");
					 strcat (scriptline,"(");
				 }
		}
		
		/* convert these values to a jsval type */
		strcat (scriptline,"))");
		if (!ActualrunScript(tn,scriptline,&retval))
			printf ("AR failed in setxx\n");

		// JAS - not sure what to do; we have called the function with new
		// objects. What else is required?
		//
		//if (!JS_SetProperty (_context, _globalObj, scriptline, &retval))
		//	printf ("JS_SetProperty failed in setMFElementtype\n");

		//_privPtr->touched = 0;

		/* now, runscript to tell it that it has been touched */
		//sprintf (scriptline,"__tmp_arg_%s.__touched()", JSparamnames[tptr].name);
		//if (!ActualrunScript(tn, scriptline ,&retval)) 
		//	printf ("failed to set parameter, line %s\n",scriptline);

		/* and run the function */
		//sprintf (scriptline,"%s(__tmp_arg_%s,%f)",
		//	 JSparamnames[tptr].name,JSparamnames[tptr].name,
		//	 TickTime);
		//if (!ActualrunScript(tn, scriptline ,&retval)) {
		//	printf ("failed to set parameter, line %s\n",scriptline);
		//}
	}
}

/* internal variable to copy a C structure's Multi* field */
void Multimemcpy (void *tn, void *fn, int multitype) {
	unsigned int structlen;
	unsigned int fromcount, tocount;
	void *fromptr, *toptr;

	struct Multi_Vec3f *mv3ffn, *mv3ftn;

	//printf ("Multimemcpy, copying structures %d %d type %d\n",tn,fn,multitype);

	/* copy a complex (eg, a MF* node) node from one to the other 
	   the following types are currently found in VRMLNodes.pm - 

		 -1  is a Multi_Color or MultiVec3F
		 -10 is a Multi_Node 
		 -12 is a SFImage 
		 -13 is a Multi_String 
		 -14 is a Multi_Float 
		 -15 is a Multi_Rotation 
		 -16 is a Multi_Int32 
		 -18 is a Multi_Vec2f 
	*/

	/* Multi_XXX nodes always consist of a count then a pointer - see
	   Structs.h */

	/* making the input pointers into a (any) structure helps deciphering params */
	mv3ffn = (struct Multi_Vec3f *)fn;
	mv3ftn = (struct Multi_Vec3f *)tn;

	/* so, get the from memory pointer, and the to memory pointer from the structs */
	fromptr = (void *)mv3ffn->p;

	/* and the from and to sizes */
	fromcount = mv3ffn->n;
	tocount = mv3ftn->n;

	/* get the structure length */
	switch (multitype) {
		case -1: {structlen = sizeof (struct SFColor); break; }
		case -10: {structlen = sizeof (unsigned int); break; }
		case -12: {structlen = sizeof (unsigned int); break; } // this is broken in many, many ways
		case -13: {structlen = sizeof (unsigned int); break; }
		case -14: {structlen = sizeof (float); break; }
		case -15: {structlen = sizeof (struct SFRotation); break;}
		case -16: {structlen = sizeof (int); break;}
		case -18: {structlen = sizeof (struct SFVec2f); break;}
		default: {
			printf("WARNING: Multimemcpy, don't handle type %d yet\n", multitype);
			structlen=0;
			return;
		}
	}


	/* free the old data, if there is old data... */
	if ((mv3ftn->p) != NULL) free (mv3ftn->p);

	/* malloc the toptr */
	mv3ftn->p = malloc (structlen*fromcount);
	toptr = (void *)mv3ftn->p;

	/* tell the recipient how many elements are here */
	mv3ftn->n = fromcount;

	//printf ("Multimemcpy, fromcount %d tocount %d fromptr %d toptr %d\n",fromcount,tocount,fromptr,toptr);

	/* and do the copy of the data */
	memcpy (toptr,fromptr,structlen * fromcount);
}



/* These events must be run first during the event loop, as they start an event cascade. 
   Regsister them with add_first, then call them during the event loop with do_first.    */

void add_first(char *clocktype,unsigned int node) {
	void (*myp)(unsigned *);

	if (strncmp("TimeSensor",clocktype,10) == 0) { myp =  (void *)do_TimeSensorTick;
	} else if (strncmp("ProximitySensor",clocktype,10) == 0) { myp = (void *)do_ProximitySensorTick;
	} else if (strncmp("Collision",clocktype,10) == 0) { myp = (void *)do_CollisionTick;
	} else if (strncmp("MovieTexture",clocktype,10) == 0) { myp = (void *)do_MovieTextureTick;
	} else if (strncmp("AudioClip",clocktype,10) == 0) { myp = (void *)do_AudioTick;

	} else {
		printf ("VRML::VRMLFunc::add_first, unhandled type %s\n",clocktype);
		return;
	}

	ClockEvents = realloc(ClockEvents,sizeof (struct FirstStruct) * (num_ClockEvents+1));
	if (ClockEvents == 0) {
		printf ("can not allocate memory for add_first call\n");
		num_ClockEvents = 0;
	}

	if (node == 0) {
		printf ("error in add_first; somehow the node datastructure is zero for type %s\n",clocktype);
		return;
	}

	/* now, put the function pointer and data pointer into the structure entry */
	ClockEvents[num_ClockEvents].interpptr = myp;
	ClockEvents[num_ClockEvents].tonode = node;

	num_ClockEvents++;
}



/*******************************************************************

CRoutes_js_new;

Register a new script for future routing

********************************************************************/

void CRoutes_js_new (int num,unsigned int cx, unsigned int glob, unsigned int brow) {
	/* jsval retval; */
	UNUSED(cx);
	UNUSED(glob);
	UNUSED(brow);

	//printf ("CRoutes_js_new, number %d\n",num);

	/* more scripts than we can handle right now? */
	if (num >= JSMaxScript)  {
		JSMaxAlloc();
	}

	if (num > max_script_found) max_script_found = num;
}

int convert_typetoInt (char *type) {
	/* first, convert the type to an integer value */
	if (strncmp("SFBool",type,7) == 0) return SFBOOL;
	else if (strncmp ("SFColor",type,7) == 0) return SFCOLOR;
	else if (strncmp ("SFVec3f",type,7) == 0) return SFCOLOR; /*Colors and Vec3fs are same */
	else if (strncmp ("SFFloat",type,7) == 0) return SFFLOAT;
	else if (strncmp ("SFTime",type,6) == 0) return SFTIME;
	else if (strncmp ("SFInt32",type,6) == 0) return SFINT32;
	else if (strncmp ("SFString",type,6) == 0) return SFSTRING;
	else if (strncmp ("SFNode",type,6) == 0) return SFNODE;
	else if (strncmp ("SFVec2f",type,6) == 0) return SFVEC2F;
	else if (strncmp ("SFRotation",type,6) == 0) return SFROTATION;
	else if (strncmp ("MFColor",type,7) == 0) return MFCOLOR;
	else if (strncmp ("MFVec3f",type,7) == 0) return MFCOLOR; /*Colors and Vec3fs are same */
	else if (strncmp ("MFFloat",type,7) == 0) return MFFLOAT;
	else if (strncmp ("MFTime",type,6) == 0) return MFTIME;
	else if (strncmp ("MFInt32",type,6) == 0) return MFINT32;
	else if (strncmp ("MFString",type,6) == 0) return MFSTRING;
	else if (strncmp ("MFNode",type,6) == 0) return MFNODE;
	else if (strncmp ("MFVec2f",type,6) == 0) return MFVEC2F;
	else if (strncmp ("MFRotation",type,6) == 0) return MFROTATION;

	else {
		printf("WARNING: JSparamIndex, cant match type %s\n",type);
		return SFUNKNOWN;
	}
}

/********************************************************************

JSparamIndex. 

stores ascii names with types (see code for type equivalences).

********************************************************************/

int JSparamIndex (char *name, char *type) {
	unsigned len;
	int ty;
	int ctr;

	ty = convert_typetoInt(type);

	len = strlen(name);

	/* is this a duplicate name and type? types have to be same,
	   name lengths have to be the same, and the strings have to be the same.
	*/
	for (ctr=0; ctr<=jsnameindex; ctr++) {
		if (ty==JSparamnames[ctr].type) {
			if ((strlen(JSparamnames[ctr].name) == len) && 
				(strncmp(name,JSparamnames[ctr].name,len)==0)) {
				return ctr;
			}
		}
	}
	
	/* nope, not duplicate */		

	jsnameindex ++;

	/* ok, we got a name and a type */
	if (jsnameindex >= MAXJSparamNames) {
		/* oooh! not enough room at the table */
		MAXJSparamNames += 100; /* arbitrary number */
		JSparamnames = realloc (JSparamnames, sizeof(*JSparamnames) * MAXJSparamNames);
	}

	if (len > MAXJSVARIABLELENGTH-2) len = MAXJSVARIABLELENGTH-2;	/* concatenate names to this length */
	strncpy (JSparamnames[jsnameindex].name,name,len);
	JSparamnames[jsnameindex].name[len] = 0; /* make sure terminated */
	JSparamnames[jsnameindex].type = ty;
	return jsnameindex;
}

/********************************************************************

CRoutes_Register. 

Register a route in the routing table.

********************************************************************/


void

CRoutes_Register(int adrem, unsigned int from, int fromoffset, unsigned int to_count, char *tonode_str,
				 int length, void *intptr, int scrdir, int extra)
{
	int insert_here, shifter;
	char *buffer;
	const char *token = " ";
	CRnodeStruct *to_ptr = NULL;
	unsigned int to_counter;
	char *chptr;
	char buf[20];
	unsigned ton, toof;		/* used to help determine duplicate routes */

	/* is this a script to script route??? */
	if (scrdir == SCRIPT_TO_SCRIPT) {
		chptr = malloc (sizeof (char) * length);
		// printf ("wwwwwoooowwww!!! script to script!! length %d\n",length);
		if (length > 0) {
			sprintf (buf,"%d:0",(int) chptr);
			CRoutes_Register (adrem, from, fromoffset,1,buf, length, 0, FROM_SCRIPT, extra);
			CRoutes_Register (adrem, (unsigned)chptr, 0, to_count, tonode_str,length, 0, TO_SCRIPT, extra);
			return;
		} else {
			// XXXX
			printf ("CRoutes_Register, can't handle script to script with MF* nodes yet\n");
			return;
		}
	}

	/* first time through, create minimum and maximum for insertion sorts */
	if (!CRoutes_Initiated) {
		/* allocate the CRoutes structure */
		CRoutes_MAX = 25; /* arbitrary number; max 25 routes to start off with */
		CRoutes = malloc (sizeof (*CRoutes) * CRoutes_MAX);

		CRoutes[0].fromnode = 0;
		CRoutes[0].fnptr = 0;
		CRoutes[0].tonode_count = 0;
		CRoutes[0].tonodes = NULL;
		CRoutes[0].act = FALSE;
		CRoutes[0].interpptr = 0;
		CRoutes[1].fromnode = 0x8FFFFFFF;
		CRoutes[1].fnptr = 0x8FFFFFFF;
		CRoutes[1].tonode_count = 0;
		CRoutes[1].tonodes = NULL;
		CRoutes[1].act = FALSE;
		CRoutes[1].interpptr = 0;
		CRoutes_Count = 2;
		CRoutes_Initiated = TRUE;

		/* and mark all scripts active to get the initialize() events */
		scripts_active = TRUE;
	}

	if (CRVerbose) 
		printf ("CRoutes_Register from %u off %u to %u %s len %d intptr %u\n",
				from, fromoffset, to_count, tonode_str, length, (unsigned)intptr);

	insert_here = 1;

	/* go through the routing list, finding where to put it */
	while (from > CRoutes[insert_here].fromnode) {
		if (CRVerbose) printf ("comparing %u to %u\n",from, CRoutes[insert_here].fromnode);
		insert_here++; 
	}

	/* hmmm - do we have a route from this node already? If so, go
	   through and put the offsets in order */
	while ((from == CRoutes[insert_here].fromnode) &&
		(fromoffset > CRoutes[insert_here].fnptr)) { 
		if (CRVerbose) printf ("same fromnode, different offset\n");
		insert_here++;
	}

	/* Quick check to verify that we don't have a duplicate route here 
	   OR to delete a route... */
	if ((CRoutes[insert_here-1].fromnode==from) &&
		(CRoutes[insert_here-1].fnptr==(unsigned)fromoffset) &&
		(CRoutes[insert_here-1].interpptr==intptr) &&
		(CRoutes[insert_here-1].tonodes!=0)) {

		/* possible duplicate route */
		sscanf (tonode_str, "%u:%u", &ton,&toof);
		if ((ton == (CRoutes[insert_here-1].tonodes)->node) &&
			(toof == (CRoutes[insert_here-1].tonodes)->foffset)) {
			/* this IS a duplicate, now, what to do? */
			
			/* is this an add? */
			if (adrem == 1) {
				/* printf ("definite duplicate, returning\n"); */
				return;
			} else {
				/* this is a remove */
				for (shifter = CRoutes_Count-1; shifter > insert_here-1; shifter--) {
				if (CRVerbose) printf ("copying from %d to %d\n",shifter, shifter-1);
					memcpy ((void *)&CRoutes[shifter-1], 
						(void *)&CRoutes[shifter], 
						sizeof (struct CRStruct));
				}
				CRoutes_Count --;
				if (CRVerbose) {
					printf ("routing table now %d\n",CRoutes_Count);
					for (shifter = 0; shifter < CRoutes_Count; shifter ++) {
						printf ("%d %d %d\n",CRoutes[shifter].fromnode, CRoutes[shifter].fnptr, 
							(int)CRoutes[shifter].interpptr);
					}
				}

				return;
			}
		}
	}

	/* is this a removeRoute? if so, its not found, and we SHOULD return here */
	if (adrem != 1) return;

	if (CRVerbose) printf ("CRoutes, inserting at %d\n",insert_here);
	/* create the space for this entry. */
	for (shifter = CRoutes_Count; shifter > insert_here; shifter--) {
		memcpy ((void *)&CRoutes[shifter], (void *)&CRoutes[shifter-1],sizeof(struct CRStruct));
		if (CRVerbose) printf ("Copying from index %d to index %d\n",shifter, shifter-1);
	}


	/* and put it in */
	CRoutes[insert_here].fromnode = from;
	CRoutes[insert_here].fnptr = fromoffset;
	CRoutes[insert_here].act = FALSE;
	CRoutes[insert_here].tonode_count = 0;
	CRoutes[insert_here].tonodes = NULL;
	CRoutes[insert_here].len = length;
	CRoutes[insert_here].interpptr = intptr;
	CRoutes[insert_here].direction_flag = scrdir;
	CRoutes[insert_here].extra = extra;

	if (to_count > 0) {
		if ((CRoutes[insert_here].tonodes =
			 (CRnodeStruct *) calloc(to_count, sizeof(CRnodeStruct))) == NULL) {
			fprintf(stderr, "CRoutes_Register: calloc failed to allocate memory.\n");
		} else {
			CRoutes[insert_here].tonode_count = to_count;
			if (CRVerbose)
				printf("CRoutes at %d to nodes: %s\n",
					   insert_here, tonode_str);

			if ((buffer = strtok(tonode_str, token)) != NULL) {
				/* printf("\t%s\n", buffer); */
				to_ptr = &(CRoutes[insert_here].tonodes[0]);
				if (sscanf(buffer, "%u:%u",
						   &(to_ptr->node), &(to_ptr->foffset)) == 2) {
					if (CRVerbose) printf("\tsscanf returned: %u, %u\n",
						  to_ptr->node, to_ptr->foffset);
				}


				/* condition statement changed */
				buffer = strtok(NULL, token);
				for (to_counter = 1;
					 ((to_counter < to_count) && (buffer != NULL));
					 to_counter++) {
					to_ptr = &(CRoutes[insert_here].tonodes[to_counter]);
					if (sscanf(buffer, "%u:%u",
							   &(to_ptr->node), &(to_ptr->foffset)) == 2) {
						if (CRVerbose) printf("\tsscanf returned: %u, %u\n",
											  to_ptr->node, to_ptr->foffset);
					}
					buffer = strtok(NULL, token);
				}
			}
		}
	}

	/* record that we have one more route, with upper limit checking... */
	if (CRoutes_Count >= (CRoutes_MAX-2)) {
		//printf("WARNING: expanding routing table\n");
		CRoutes_MAX += 50; /* arbitrary expansion number */
		CRoutes = realloc (CRoutes, sizeof (*CRoutes) * CRoutes_MAX);
	}

	CRoutes_Count ++;

	if (CRVerbose) {
		printf ("routing table now %d\n",CRoutes_Count);
		for (shifter = 0; shifter < CRoutes_Count; shifter ++) {
			printf ("%d %d %d\n",CRoutes[shifter].fromnode, CRoutes[shifter].fnptr, 
				(int)CRoutes[shifter].interpptr);
		}
	}

}

void
CRoutes_free()
{
	int i;
	for (i = 0; i < CRoutes_Count; i++) {
		if (CRoutes[i].tonodes != NULL) {
			free(CRoutes[i].tonodes);
		}
	}
}

/********************************************************************

mark_event - something has generated an eventOut; record the node
data structure pointer, and the offset. Mark all relevant entries
in the routing table that this node/offset triggered an event.

********************************************************************/

void mark_event (unsigned int from, unsigned int totalptr) {
	int findit;

	if (!CRoutes_Initiated) return;  /* no routes registered yet */

	findit = 1;

	if (CRVerbose)
		/* printf ("mark_event, from %#x fromoffset %#x\n",from,totalptr); */
		printf ("\nmark_event, from %u fromoffset %u\n", from, totalptr);

	/* events in the routing table are sorted by fromnode. Find
	   out if we have at least one route from this node */
	while (from > CRoutes[findit].fromnode) findit ++;

	/* while we have an eventOut from this NODE/OFFSET, mark it as 
	   active. If no event from this NODE/OFFSET, ignore it */
	while ((from == CRoutes[findit].fromnode) &&
		(totalptr != CRoutes[findit].fnptr)) findit ++;

	/* did we find the exact entry? */
	if (CRVerbose) {
 		printf ("ep, (%#x %#x) (%#x %#x) at %d \n",
			from,CRoutes[findit].fromnode, totalptr,
			CRoutes[findit].fnptr,findit); }

	/* if we did, signal it to the CEvents loop  - maybe more than one ROUTE,
	   eg, a time sensor goes to multiple interpolators */
	while ((from == CRoutes[findit].fromnode) && 
		(totalptr == CRoutes[findit].fnptr)) {
		if (CRVerbose) 
			printf ("found event at %d\n",findit);
		CRoutes[findit].act=TRUE;
		findit ++;
	}
	if (CRVerbose) 
		printf ("done mark_event\n");
}


/********************************************************************

mark_script - indicate that this script has had an eventIn
zero_scripts - reset all script indicators

********************************************************************/
void mark_script (int num) {

	if (CRVerbose) printf ("mark_script - script %d has been invoked\n",num); 
	scr_act[num]= TRUE;
	scripts_active = TRUE;
}


/********************************************************************

gatherScriptEventOuts - at least one script has been triggered; get the
eventOuts for this script

FIXME XXXXX =  can we do this without the string conversions?

********************************************************************/

void gatherScriptEventOuts(int actualscript, int ignore) {
	int route;	
	/* char scriptline[100]; */
	/* jsval retval; */
	int fn, tn, fptr, tptr;
	unsigned len;
	float fl[0];	/* return float values */
	double tval;
	int ival;
	/* jsval touched; */		/* was this really touched? */

        JSString *strval; /* strings */
        char *strp = 0;
	/* char *strtouched; */
	int fromalready=FALSE;	 /* we have already got the from value string */
	int touched_flag=FALSE;
	unsigned int to_counter;
	CRnodeStruct *to_ptr = NULL;

	UNUSED(ignore);

	/* go through all routes, looking for this script as an eventOut */

	/* do we have any routes yet? - we can gather events before any routes are made */
	if (!CRoutes_Initiated) return;

	/* routing table is ordered, so we can walk up to this script */
	route=1;
	while (CRoutes[route].fromnode<(unsigned)actualscript) route++;
	while (CRoutes[route].fromnode == (unsigned)actualscript) {
		/* is this the same from node/field as before? */
		if ((CRoutes[route].fromnode == CRoutes[route-1].fromnode) &&
			(CRoutes[route].fnptr == CRoutes[route-1].fnptr) &&
			(route > 1)) {
			fromalready=TRUE;
		} else {
			/* printf ("different from, have to get value\n"); */
			fromalready=FALSE;
		}
		
		fptr = CRoutes[route].fnptr;
		fn = CRoutes[route].fromnode;
		len = CRoutes[route].len;

		if (CRVerbose) 
			printf ("\ngatherSentEvents, from %s type %d len %d\n",JSparamnames[fptr].name,
				JSparamnames[fptr].type, len);	

		/* in Ayla's Perl code, the following happened:
			MF* - run __touched_flag

			SFBool, SFFloat, SFTime, SFInt32, SFString-
				this is her $ECMASCriptNative; run _name_touched
				and _name_touched=0

			else, run _name.__touched()
		*/

		/* now, set the actual properties - switch as documented above */
		if (!fromalready) {
			if (CRVerbose) printf ("Not found yet, getting touched flag fptr %d script %d \n",fptr,actualscript);
			touched_flag = get_touched_flag(fptr,actualscript);

			if (touched_flag) {
				/* we did, so get the value */
				strval = JS_ValueToString((JSContext *)JSglobs[actualscript].cx, global_return_val);
			        strp = JS_GetStringBytes(strval);

				if (JSVerbose) 
					printf ("retval string is %s\n",strp);
			}
		}


		if (touched_flag) {
			/* get some easy to use pointers */
			for (to_counter = 0; to_counter < CRoutes[route].tonode_count; to_counter++) {
				to_ptr = &(CRoutes[route].tonodes[to_counter]);
				tn = (int) to_ptr->node;
				tptr = (int) to_ptr->foffset;

				if (JSVerbose) printf ("VALUE CHANGED! copy value and update %d\n",tn);

				if (JSVerbose) printf (" -- string from javascript is %s\n",strp);
				/* eventOuts go to VRML data structures */

				switch (JSparamnames[fptr].type) {
				case SFBOOL:	{	/* SFBool */
					//printf ("we have a boolean, copy value over string is %s\n",strp); 
					if (strncmp(strp,"true",4)==0) {
						ival = 1;
					} else {
						/* printf ("ASSUMED TO BE FALSE\n"); */
						ival = 0;
					}	
					memcpy ((void *)(tn+tptr), (void *)&ival,len);
					break;
				}

				case SFTIME: {
					if (!JS_ValueToNumber((JSContext *)JSglobs[actualscript].cx, 
										  global_return_val,&tval)) tval=0.0;

					//printf ("SFTime conversion numbers %f from string %s\n",tval,strp);
					//printf ("copying to %#x offset %#x len %d\n",tn, tptr,len);
					memcpy ((void *)(tn+tptr), (void *)&tval,len);
					break;
				}
				case SFNODE:
				case SFINT32: {
					sscanf (strp,"%d",&ival);
					//printf ("SFInt, SFNode conversion number %d\n",ival);
					memcpy ((void *)((tn+tptr)), (void *)&ival,len);
					break;
				}
				case SFFLOAT: {
					sscanf (strp,"%f",&fl[0]);
					memcpy ((void *)(tn+tptr), (void *)&fl,len);
					break;
				}

				case SFVEC2F: {	/* SFVec2f */
					sscanf (strp,"%f %f",&fl[0],&fl[1]);
					//printf ("conversion numbers %f %f\n",fl[0],fl[1]);
					memcpy ((void *)(tn+tptr), (void *)fl,len);
					break;
				}

				case SFCOLOR: {	/* SFColor */
					sscanf (strp,"%f %f %f",&fl[0],&fl[1],&fl[2]);
					//printf ("conversion numbers %f %f %f\n",fl[0],fl[1],fl[2]);
					memcpy ((void *)(tn+tptr), (void *)fl,len);
					break;
				}

				case SFROTATION: {
					sscanf (strp,"%f %f %f %f",&fl[0],&fl[1],&fl[2],&fl[3]);
					//printf ("conversion numbers %f %f %f %f\n",fl[0],fl[1],fl[2],fl[3]);
					memcpy ((void *)(tn+tptr), (void *)fl,len);
					break;
				}


					/* a series of Floats... */
				case MFCOLOR: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, (void *)(tn+tptr),3); break;}
				case MFFLOAT: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, (void *)(tn+tptr),1); break;}
				case MFROTATION: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, (void *)(tn+tptr),4); break;}
				case MFVEC2F: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, (void *)(tn+tptr),2); break;}
				case MFNODE: {getMFNodetype (strp,(void *)(tn+tptr),CRoutes[route].extra); break;}
				case MFSTRING: {
					getMFStringtype ((JSContext *) JSglobs[actualscript].cx,
						 (jsval *)global_return_val,(void *)(tn+tptr)); 
					break;
				}

				case MFINT32: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, (void *)(tn+tptr),0); break;}
				case MFTIME: {getMultNumType ((JSContext *)JSglobs[actualscript].cx, (void *)(tn+tptr),5); break;}

				default: {	printf("WARNING: unhandled from type %s\n", FIELD_TYPE_STRING(JSparamnames[fptr].type));
				printf (" -- string from javascript is %s\n",strp);
				}
				}

				/* tell this node now needs to redraw  - but only if it is not a script to
				   script route - see CRoutes_Register here, and check for the malloc in that code.
				   You should see that the offset is zero, while in real nodes, the offset of user
				   accessible fields is NEVER zero - check out CFuncs/Structs.h and look at any of 
				   the node types, eg, VRML_IndexedFaceSet  the first offset is for VRML_Virt :=)
				*/
				if (tptr != 0) {
					//printf ("can update this node %d %d\n",tn,tptr);
					update_node((void *)tn);
				} else {
					//printf ("skipping this node %d %d flag %d\n",tn,tptr,CRoutes[route].direction_flag);
				}


				mark_event (CRoutes[route].fromnode,CRoutes[route].fnptr);

				/* run an interpolator, if one is attached. */
				if (CRoutes[route].interpptr != 0) {
					/* this is an interpolator, call it */
					CRoutesExtra = CRoutes[route].extra; // in case the interp requires it...
					if (CRVerbose) printf ("script propagate_events. index %d is an interpolator\n",route);
					CRoutes[route].interpptr((void *)(to_ptr->node));
				}
			}
		}
		route++;
	}
	if (JSVerbose) printf ("finished  gatherScriptEventOuts loop\n");
}

/********************************************************************

sendScriptEventIn.

this sends events to scripts that have eventIns defined.

********************************************************************/

void sendScriptEventIn(int num) {
	unsigned int to_counter;
	CRnodeStruct *to_ptr = NULL;

	if (JSVerbose) printf("sendScriptEventIn, num %d\n",num);

	/* script value: 1: this is a from script route
			 2: this is a to script route
			 3: this is a from script to a script route */

	if (CRoutes[num].direction_flag == TO_SCRIPT) {
		for (to_counter = 0; to_counter < CRoutes[num].tonode_count; to_counter++) {
			to_ptr = &(CRoutes[num].tonodes[to_counter]);

			/* get the value from the VRML structure, in order to propagate it to a script */

			/* mark that this script has been active */
			mark_script((int)(to_ptr->node));

			/* set the parameter */
			/* see comments in gatherScriptEventOuts to see exact formats */

			switch (JSparamnames[to_ptr->foffset].type) {
			case SFBOOL:	
			case SFFLOAT:
			case SFTIME:
			case SFINT32:
			case SFNODE:
			case SFSTRING: {
				setECMAtype(num);
				break;
			}
			case SFCOLOR: 
			case SFVEC2F:
			case SFROTATION: {
				setMultiElementtype(num);
				break;
			}
			case MFCOLOR:
			case MFFLOAT:
			case MFTIME:
			case MFINT32:
			case MFSTRING:
			case MFNODE:
			case MFROTATION: {
				setMFElementtype(num);
				break;
			}
			default : {
				printf("WARNING: sendScriptEventIn type %s not handled yet\n", 
					FIELD_TYPE_STRING(JSparamnames[to_ptr->foffset].type));
				}
			}
		}
	} else if (CRoutes[num].direction_flag == SCRIPT_TO_SCRIPT) {
		printf("WARNING: sendScriptEventIn, don't handle script to script routes yet\n");
	}
}

/********************************************************************

propagate_events.

Go through the event table, until the table is "active free". Some
nodes have eventins/eventouts - have to do the table multiple times
in this case.

********************************************************************/
void propagate_events() {
	int counter, havinterp;
	unsigned int to_counter;
	CRnodeStruct *to_ptr = NULL;

	/* int mvcompCount, mvcompSize; */
	/* struct Multi_Vec3f *mv3fptr; */

	if (CRVerbose) 
		printf ("\npropagate_events start\n");

	do {
		havinterp=FALSE; /* assume no interpolators triggered */

		for (counter = 1; counter < CRoutes_Count-1; counter++) {
			for (to_counter = 0; to_counter < CRoutes[counter].tonode_count; to_counter++) {
				to_ptr = &(CRoutes[counter].tonodes[to_counter]);
				if (to_ptr == NULL) {
					printf("WARNING: tonode at %u is NULL in propagate_events.\n",
							to_counter);
					continue;
				}
				if (CRVerbose)
					/* printf("propagate_events: counter %d to_counter %u from %#x off %#x to %#x off %#x oint %#x\n", */
					printf("propagate_events: counter %d to_counter %u act %s from %u off %u to %u off %u oint %u dir %d\n",
						   counter, to_counter, BOOL_STRING(CRoutes[counter].act),
						   CRoutes[counter].fromnode, CRoutes[counter].fnptr,
						   to_ptr->node, to_ptr->foffset, (int)CRoutes[counter].interpptr,
							CRoutes[counter].direction_flag);

				if (CRoutes[counter].act == TRUE) {
					if (CRVerbose)
						printf("event %u %u sent something\n", CRoutes[counter].fromnode, CRoutes[counter].fnptr);

					/* to get routing to/from exposedFields, lets
					 * mark this to/offset as an event */
					mark_event (to_ptr->node, to_ptr->foffset);

					if (CRoutes[counter].direction_flag != 0) {
						/* scripts are a bit complex, so break this out */
						sendScriptEventIn(counter);
						if (scripts_active) havinterp = TRUE;
					} else {

						/* copy the value over */
						if (CRoutes[counter].len > 0) {
						/* simple, fixed length copy */

							memcpy((void *)(to_ptr->node + to_ptr->foffset),
								   (void *)(CRoutes[counter].fromnode + CRoutes[counter].fnptr),
								   (unsigned)CRoutes[counter].len);
						} else {
							/* this is a Multi*node, do a specialized copy */

							Multimemcpy ((void *)(to_ptr->node + to_ptr->foffset),
								 (void *)(CRoutes[counter].fromnode + CRoutes[counter].fnptr),
								 CRoutes[counter].len);
						}

						/* is this an interpolator? if so call the code to do it */
						if (CRoutes[counter].interpptr != 0) {
							/* this is an interpolator, call it */
							havinterp = TRUE;
							if (CRVerbose)
								printf("propagate_events: index %d is an interpolator\n",
									   counter);
							/* copy over this "extra" data, EAI "advise" calls need this */
							CRoutesExtra = CRoutes[counter].extra;
							CRoutes[counter].interpptr((void *)(to_ptr->node));
						} else {	
							/* just an eventIn node. signal to the reciever to update */
							mark_event(to_ptr->node, to_ptr->foffset);
							update_node((void *)to_ptr->node);
						}
					}
				}
			}

			if (CRoutes[counter].act == TRUE) {
				/* we have this event found */
				CRoutes[counter].act = FALSE;
			}

		}

		/* run gatherScriptEventOuts for each active script */
		if (scripts_active) {
			for (counter =0; counter <= max_script_found; counter++) {
				gatherScriptEventOuts (counter,TRUE);
			}
		}

		/* set all script flags to false - no triggers */
		scripts_active = FALSE;

		
	} while (havinterp==TRUE);

	if (CRVerbose) printf ("done propagate_events\n\n");
}



/********************************************************************

process_eventsProcessed()

According to the spec, all scripts can have an eventsProcessed
function - see section C.4.3 of the spec.

********************************************************************/
void process_eventsProcessed() {

	int counter;
	jsval retval;

	for (counter = 0; counter <= max_script_found; counter++) {
      		if (!ActualrunScript(counter, "eventsProcessed()" ,&retval))
                	printf ("failed to run eventsProcessed for script %d\n",counter);
	}
}

/*******************************************************************

do_first()


Call the sensor nodes to get the results of the clock ticks; this is
the first thing in the event loop.

********************************************************************/

void do_first() {
	int counter;

	/* go through the array; add_first will NOT add a null pointer
	   to either field, so we don't need to bounds check here */

	for (counter =0; counter < num_ClockEvents; counter ++) {
		ClockEvents[counter].interpptr((int *)(ClockEvents[counter].tonode));
	}

	/* now, propagate these events */
	propagate_events();
}
