/*


Javascript C language binding.

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
#include "../main/headers.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../vrml_parser/CRoutes.h"
#include "../main/Snapshot.h"
#include "../scenegraph/Collision.h"
#include "../scenegraph/quaternion.h"
#include "../scenegraph/Viewer.h"
#include "../input/EAIHelpers.h"
#include "../input/SensInterps.h"
#include "../x3d_parser/Bindable.h"

#include "JScript.h"
#include "CScripts.h"
#include "jsUtils.h"
#include "jsNative.h"
#include "jsVRMLClasses.h"
#include "jsVRMLBrowser.h"


#if defined(HAVE_JAVASCRIPT)

#ifndef JSCLASS_GLOBAL_FLAGS
//spidermonkey < 1.7 doesn't have so define here
#define JSCLASS_GLOBAL_FLAGS 0
#endif


static JSClass staticGlobalClass = {
	"global",		// char *name
	JSCLASS_GLOBAL_FLAGS,	// uint32 flags
	JS_PropertyStub,	// JSPropertyOp addProperty
	JS_PropertyStub,	// JSPropertyOp delProperty
	JS_PropertyStub,	// JSPropertyOp getProperty
	JS_StrictPropertyStub,	// JSStrictPropertyOp setProperty
	JS_EnumerateStub,	// JSEnumerateOp enumerate
	globalResolve,		// JSResolveOp resolve
	JS_ConvertStub,		// JSConvertOp convert
	// following are optional and can be NULL
	JS_FinalizeStub,	// JSFinalizeOp finalize
	NULL,			// JSClassInternal reserved
	NULL,			// JSCheckAccessOp checkAccess
	NULL,			// JSNative call
	NULL,			// JSNative construct
	NULL,			// JSXDRObjectOp xdrObject
	NULL,			// JSJasInstanceOp hasInstance
	NULL			// JSTraceOp trace
};


#endif // HAVE_JAVASCRIPT


typedef struct pJScript{
	/* Script name/type table */
	struct CRjsnameStruct *JSparamnames;// = NULL;
	int JSMaxScript;// = 0;


#ifdef HAVE_JAVASCRIPT
	JSRuntime *runtime;// = NULL;
	JSClass globalClass;
	jsval JSglobal_return_value;
#endif // HAVE_JAVASCRIPT

}* ppJScript;


void *JScript_constructor(){
	void *v = malloc(sizeof(struct pJScript));
	memset(v,0,sizeof(struct pJScript));
	return v;
}
void JScript_init(struct tJScript *t){
	//public
	t->jsnameindex = -1;
	t->MAXJSparamNames = 0;
	t->JSglobal_return_val = NULL;
	//private
	t->prv = JScript_constructor();
	{
		ppJScript p = (ppJScript)t->prv;
		/* Script name/type table */
		p->JSparamnames = NULL;
#ifdef HAVE_JAVASCRIPT
		p->JSMaxScript = 0;
		p->runtime = NULL;
		memcpy(&p->globalClass,&staticGlobalClass,sizeof(staticGlobalClass));
		t->JSglobal_return_val = &p->JSglobal_return_value;
#endif // HAVE_JAVASCRIPT
	}
}
//	ppJScript p = (ppJScript)gglobal()->JScript.prv;

jsval *getJSglobalRval()
{
	ppJScript p = (ppJScript)gglobal()->JScript.prv;
	return &p->JSglobal_return_value;
}

void js_cleanup_script_context(int counter){
	ttglobal tg = gglobal();
	ppJScript p = (ppJScript)tg->JScript.prv;
	//CLEANUP_JAVASCRIPT(p->ScriptControl[counter].cx);
	CLEANUP_JAVASCRIPT(getScriptControlIndex(counter)->cx);
}

void initializeAnyScripts()
{
/*
   we want to run initialize() from the calling thread. NOTE: if
   initialize creates VRML/X3D nodes, it will call the ProdCon methods
   to do this, and these methods will check to see if nodes, yada,
   yada, yada, until we run out of stack. So, we check to see if we
   are initializing; if so, don't worry about checking for new scripts
   any scripts to initialize here? we do it here, because we may just
   have created new scripts during  X3D/VRML parsing. Routing in the
   Display thread may have noted new scripts, but will ignore them
   until   we have told it that the scripts are initialized.  printf
   ("have scripts to initialize in fwl_RenderSceneUpdateScene old %d new
   %d\n",max_script_found, max_script_found_and_initialized);
*/

//#define INITIALIZE_ANY_SCRIPTS 
	ttglobal tg = (ttglobal)gglobal();
	if( tg->CRoutes.max_script_found != tg->CRoutes.max_script_found_and_initialized) 
	{ 
		struct CRscriptStruct *ScriptControl = getScriptControl(); 
		int i; jsval retval; 
		for (i=tg->CRoutes.max_script_found_and_initialized+1; i <= tg->CRoutes.max_script_found; i++) 
		{ 
			/* printf ("initializing script %d in thread %u\n",i,pthread_self());  */ 
			JSCreateScriptContext(i); 
			JSInitializeScriptAndFields(i); 
			if (ScriptControl[i].scriptOK) ACTUALRUNSCRIPT(i, "initialize()" ,&retval); 
			 /* printf ("initialized script %d\n",i);*/  
		} 
		tg->CRoutes.max_script_found_and_initialized = tg->CRoutes.max_script_found; 
	}
}
/********************************************************************

process_eventsProcessed()

According to the spec, all scripts can have an eventsProcessed
function - see section C.4.3 of the spec.

********************************************************************/
/* run the script from within C */
void process_eventsProcessed() {
#ifdef HAVE_JAVASCRIPT

	int counter;
	jsval retval;
	struct CRscriptStruct *scriptcontrol;
	ttglobal tg = gglobal();
	ppJScript p = (ppJScript)tg->JScript.prv;
	for (counter = 0; counter <= tg->CRoutes.max_script_found_and_initialized; counter++) {
		scriptcontrol = getScriptControlIndex(counter);
		if (scriptcontrol->eventsProcessed == NULL) {
#if defined(JS_THREADSAFE)
			JS_BeginRequest(scriptcontrol->cx);
#endif
			scriptcontrol->eventsProcessed = (void *)JS_CompileScript(
				scriptcontrol->cx,
				scriptcontrol->glob,
				"eventsProcessed()", strlen ("eventsProcessed()"),
				"compile eventsProcessed()", 1);
#if JS_VERSION >= 185
			if (!JS_AddObjectRoot(scriptcontrol->cx,&((JSSCRIPT*)(scriptcontrol->eventsProcessed)))) {
				printf ("can not add object root for compiled eventsProcessed() for script %d\n",counter);
			}
#endif
#if defined(JS_THREADSAFE)
			JS_EndRequest(scriptcontrol->cx);
#endif
		}

#if defined(JS_THREADSAFE)
		JS_BeginRequest(p->ScriptControl[counter].cx);
#endif
		if (!JS_ExecuteScript( scriptcontrol->cx,
                                 scriptcontrol->glob,
				scriptcontrol->eventsProcessed, &retval)) {
#if defined(_MSC_VER)
			printf ("can not run eventsProcessed() for script %d thread %u\n",counter,(unsigned int)pthread_self().x);
#else
			printf ("can not run eventsProcessed() for script %d thread %p\n",counter,(void *)pthread_self());
#endif
		}
#if defined(JS_THREADSAFE)
		JS_EndRequest(scriptcontrol->cx);
#endif

	}
#endif /* HAVE_JAVASCRIPT */
}



struct CRjsnameStruct *getJSparamnames()
{
	ppJScript p = (ppJScript)gglobal()->JScript.prv;
	return p->JSparamnames;
}
void setJSparamnames(struct CRjsnameStruct *JSparamnames)
{
	ppJScript p = (ppJScript)gglobal()->JScript.prv;
	p->JSparamnames = JSparamnames;
}

/********************************************************************

JSparamIndex.

stores ascii names with types (see code for type equivalences).

********************************************************************/

int JSparamIndex (const char *name, const char *type) {
	size_t len;
	int ty;
	int ctr;
	ttglobal tg = gglobal();
	struct CRjsnameStruct *JSparamnames = getJSparamnames();

	#ifdef CRVERBOSE
	printf ("start of JSparamIndex, name %s, type %s\n",name,type);
	printf ("start of JSparamIndex, lengths name %d, type %d\n",
			strlen(name),strlen(type)); 
	#endif

	ty = findFieldInFIELDTYPES(type);

	#ifdef CRVERBOSE
	printf ("JSparamIndex, type %d, %s\n",ty,type); 
	#endif

	len = strlen(name);

	/* is this a duplicate name and type? types have to be same,
	   name lengths have to be the same, and the strings have to be the same.
	*/
	for (ctr=0; ctr<=tg->JScript.jsnameindex; ctr++) {
		if (ty==JSparamnames[ctr].type) {
			if ((strlen(JSparamnames[ctr].name) == len) &&
				(strncmp(name,JSparamnames[ctr].name,len)==0)) {
				#ifdef CRVERBOSE
				printf ("JSparamIndex, duplicate, returning %d\n",ctr);
				#endif

				return ctr;
			}
		}
	}

	/* nope, not duplicate */

	tg->JScript.jsnameindex ++;

	/* ok, we got a name and a type */
	if (tg->JScript.jsnameindex >= tg->JScript.MAXJSparamNames) {
		/* oooh! not enough room at the table */
		tg->JScript.MAXJSparamNames += 100; /* arbitrary number */
		setJSparamnames( (struct CRjsnameStruct*)REALLOC (JSparamnames, sizeof(*JSparamnames) * tg->JScript.MAXJSparamNames));
		JSparamnames = getJSparamnames();
	}

	if (len > MAXJSVARIABLELENGTH-2) len = MAXJSVARIABLELENGTH-2;	/* concatenate names to this length */
	strncpy (JSparamnames[tg->JScript.jsnameindex].name,name,len);
	JSparamnames[tg->JScript.jsnameindex].name[len] = 0; /* make sure terminated */
	JSparamnames[tg->JScript.jsnameindex].type = ty;
	JSparamnames[tg->JScript.jsnameindex].eventInFunction = NULL;
	#ifdef CRVERBOSE
	printf ("JSparamIndex, returning %d\n",tg->JScript.jsnameindex); 
	#endif

	return tg->JScript.jsnameindex;
}


/* Save the text, so that when the script is initialized in the fwl_RenderSceneUpdateScene thread, it will be there */
void SaveScriptText(int num, const char *text) {
	ttglobal tg = gglobal();
	ppJScript p = (ppJScript)tg->JScript.prv;
	struct CRscriptStruct *ScriptControl = getScriptControl();

	/* printf ("SaveScriptText, num %d, thread %u saving :%s:\n",num, pthread_self(),text); */
	if (num >= p->JSMaxScript)  {
		ConsoleMessage ("SaveScriptText: warning, script %d initialization out of order",num);
		return;
	}
	FREE_IF_NZ(ScriptControl[num].scriptText);
	ScriptControl[num].scriptText = STRDUP(text);
/* NOTE - seems possible that a script could be overwritten; if so then fix eventsProcessed */
	if (ScriptControl[num].eventsProcessed != NULL) {
#if JS_VERSION >= 185
		if (ScriptControl[num].cx != NULL) {
			JS_RemoveObjectRoot(ScriptControl[num].cx,&((JSSCRIPT*)(ScriptControl[num].eventsProcessed)));
		}
#endif
		ScriptControl[num].eventsProcessed = NULL;
	}

	if (((int)num) > tg->CRoutes.max_script_found) tg->CRoutes.max_script_found = num;
	/* printf ("SaveScriptText, for script %d scriptText %s\n",text);
	printf ("SaveScriptText, max_script_found now %d\n",max_script_found); */
}

#ifdef HAVE_JAVASCRIPT

/* MAX_RUNTIME_BYTES controls when garbage collection takes place. */
/* #define MAX_RUNTIME_BYTES 0x1000000 */
#define MAX_RUNTIME_BYTES 0x4000000L
/* #define MAX_RUNTIME_BYTES 0xC00000L */

#define STACK_CHUNK_SIZE 8192

static int JSaddGlobalECMANativeProperty(int num, const char *name);
static int JSaddGlobalAssignProperty(int num, const char *name, const char *str);
static void InitScriptField(int num, indexT kind, indexT type, const char* field, union anyVrml value);

/*
 * Global JS variables (from Brendan Eichs short embedding tutorial):
 *
 * JSRuntime       - 1 runtime per process
 * JSContext       - 1 CONTEXT per thread
 * global JSObject - 1 global object per CONTEXT
 *
 * struct JSClass {
 *     char *name;
 *     uint32 flags;
 * Mandatory non-null function pointer members:
 *     JSPropertyOp addProperty;
 *     JSPropertyOp delProperty;
 *     JSPropertyOp getProperty;
 *     JSPropertyOp setProperty;
 *     JSEnumerateOp enumerate;
 *     JSResolveOp resolve;
 *     JSConvertOp convert;
 *     JSFinalizeOp finalize;
 * Optionally non-null members start here:
 *     JSGetObjectOps getObjectOps;
 *     JSCheckAccessOp checkAccess;
 *     JSNative call;
 *     JSNative construct;
 *     JSXDRObjectOp xdrObject;
 *     JSHasInstanceOp hasInstance;
 *     prword spare[2];
 * };
 *
 * global JSClass  - populated by stubs
 *
 */

static char *DefaultScriptMethods = "function initialize() {}; " \
			" function shutdown() {}; " \
			" function eventsProcessed() {}; " \
			" TRUE=true; FALSE=false; " \
			" function print(x) {Browser.print(x)}; " \
			" function println(x) {Browser.println(x)}; " \
			" function getName() {return Browser.getName()}; "\
			" function getVersion() {return Browser.getVersion()}; "\
			" function getCurrentSpeed() {return Browser.getCurrentSpeed()}; "\
			" function getCurrentFrameRate() {return Browser.getCurrentFrameRate()}; "\
			" function getWorldURL() {return Browser.getWorldURL()}; "\
			" function replaceWorld(x) {Browser.replaceWorld(x)}; "\
			" function loadURL(x,y) {Browser.loadURL(x,y)}; "\
			" function setDescription(x) {Browser.setDescription(x)}; "\
			" function createVrmlFromString(x) {Browser.createVrmlFromString(x)}; "\
			" function createVrmlFromURL(x,y,z) {Browser.createVrmlFromURL(x,y,z)}; "\
			" function createX3DFromString(x) {Browser.createX3DFromString(x)}; "\
			" function createX3DFromURL(x,y,z) {Browser.createX3DFromURL(x,y,z)}; "\
			" function addRoute(a,b,c,d) {Browser.addRoute(a,b,c,d)}; "\
			" function deleteRoute(a,b,c,d) {Browser.deleteRoute(a,b,c,d)}; "
			"";

/* housekeeping routines */
void kill_javascript(void) {
	int i;
	ttglobal tg = gglobal();
	ppJScript p = (ppJScript)tg->JScript.prv;
	struct CRscriptStruct *ScriptControl = getScriptControl();

	/* printf ("calling kill_javascript()\n"); */
	zeroScriptHandles();
	if (p->runtime != NULL) {
		for (i=0; i<=tg->CRoutes.max_script_found_and_initialized; i++) {
			/* printf ("kill_javascript, looking at %d\n",i); */
			if (ScriptControl[i].cx != 0) {
				/* printf ("kill_javascript, context is %p\n",ScriptControl[i].cx); */
#if JS_VERSION >= 185
				if (ScriptControl[i].eventsProcessed != NULL) {
					JS_RemoveObjectRoot(ScriptControl[i].cx,&((JSSCRIPT *)(ScriptControl[i].eventsProcessed)));
				}
#endif
				JS_DestroyContextMaybeGC(ScriptControl[i].cx);
			}
		}

		JS_DestroyRuntime(p->runtime);
		p->runtime = NULL;
	}
	p->JSMaxScript = 0;
	tg->CRoutes.max_script_found = -1;
	tg->CRoutes.max_script_found_and_initialized = -1;
	FREE_IF_NZ (ScriptControl);
	setScriptControl(NULL);
	FREE_IF_NZ(tg->CRoutes.scr_act);

	/* Script name/type table */
	FREE_IF_NZ(p->JSparamnames);
	tg->JScript.jsnameindex = -1;
	tg->JScript.MAXJSparamNames = 0;

}

void cleanupDie(int num, const char *msg) {
	kill_javascript();
	freewrlDie(msg);
}

void JSMaxAlloc() {
	/* perform some REALLOCs on JavaScript database stuff for interfacing */
	int count;
	ttglobal tg = gglobal();
	ppJScript p = (ppJScript)tg->JScript.prv;
	/* printf ("start of JSMaxAlloc, JSMaxScript %d\n",JSMaxScript); */
	struct CRscriptStruct *ScriptControl = getScriptControl();

	p->JSMaxScript += 10;
	setScriptControl( (struct CRscriptStruct*)REALLOC (ScriptControl, sizeof (*ScriptControl) * p->JSMaxScript));
	ScriptControl = getScriptControl();
	tg->CRoutes.scr_act = (int *)REALLOC (tg->CRoutes.scr_act, sizeof (*tg->CRoutes.scr_act) * p->JSMaxScript);

	/* mark these scripts inactive */
	for (count=p->JSMaxScript-10; count<p->JSMaxScript; count++) {
		tg->CRoutes.scr_act[count]= FALSE;
		ScriptControl[count].thisScriptType = NOSCRIPT;
		ScriptControl[count].eventsProcessed = NULL;
		ScriptControl[count].cx = 0;
		ScriptControl[count].glob = 0;
		ScriptControl[count]._initialized = FALSE;
		ScriptControl[count].scriptOK = FALSE;
		ScriptControl[count].scriptText = NULL;
		ScriptControl[count].paramList = NULL;
	}
}

/* set up table entry for this new script */
void JSInit(int num) {
	ppJScript p = (ppJScript)gglobal()->JScript.prv;
	#ifdef JAVASCRIPTVERBOSE 
	printf("JSinit: script %d\n",num);
	#endif

	/* more scripts than we can handle right now? */
	if (num >= p->JSMaxScript)  {
		JSMaxAlloc();
	}
}

void JSInitializeScriptAndFields (int num) {
        struct ScriptParamList *thisEntry;
        struct ScriptParamList *nextEntry;
	jsval rval;
	ppJScript p = (ppJScript)gglobal()->JScript.prv;
	struct CRscriptStruct *ScriptControl = getScriptControl();

	/* printf ("JSInitializeScriptAndFields script %d, thread %u\n",num,pthread_self());   */
	/* run through paramList, and run the script */
	/* printf ("JSInitializeScriptAndFields, running through params and main script\n");  */
	if (num >= p->JSMaxScript)  {
		ConsoleMessage ("JSInitializeScriptAndFields: warning, script %d initialization out of order",num);
		return;
	}
	/* run through fields in order of entry in the X3D file */
        thisEntry = ScriptControl[num].paramList;
        while (thisEntry != NULL) {
		/* printf ("script field is %s\n",thisEntry->field);  */
		InitScriptField(num, thisEntry->kind, thisEntry->type, thisEntry->field, thisEntry->value);

		/* get the next block; free the current name, current block, and make current = next */
		nextEntry = thisEntry->next;
		FREE_IF_NZ (thisEntry->field);
		FREE_IF_NZ (thisEntry);
		thisEntry = nextEntry;
	}
	
	/* we have freed each element, set list to NULL in case anyone else comes along */
	ScriptControl[num].paramList = NULL;

	if (!ACTUALRUNSCRIPT(num, ScriptControl[num].scriptText, &rval)) {
		ConsoleMessage ("JSInitializeScriptAndFields, script failure");
		ScriptControl[num].scriptOK = FALSE;
		ScriptControl[num]._initialized = TRUE;
		return;
	}
	FREE_IF_NZ(ScriptControl[num].scriptText);
	ScriptControl[num]._initialized = TRUE;
	ScriptControl[num].scriptOK = TRUE;

}

/* create the script context for this script. This is called from the thread
   that handles script calling in the fwl_RenderSceneUpdateScene */
void JSCreateScriptContext(int num) {
	jsval rval;
	JSContext *_context; 	/* these are set here */
	JSObject *_globalObj; 	/* these are set here */
	BrowserNative *br; 	/* these are set here */
	ppJScript p = (ppJScript)gglobal()->JScript.prv;
	struct CRscriptStruct *ScriptControl = getScriptControl();

	/* is this the first time through? */
	if (p->runtime == NULL) {
		p->runtime = JS_NewRuntime(MAX_RUNTIME_BYTES);
		if (!p->runtime) freewrlDie("JS_NewRuntime failed");

		#ifdef JAVASCRIPTVERBOSE 
		printf("\tJS runtime created,\n");
		#endif
	}


	_context = JS_NewContext(p->runtime, STACK_CHUNK_SIZE);
	if (!_context) freewrlDie("JS_NewContext failed");

	#ifdef JAVASCRIPTVERBOSE 
	printf("\tJS context created,\n");
	#endif

#if defined(JS_THREADSAFE)
	JS_BeginRequest(_context);
#endif
	#if JS_VERSION >= 185
	if (num == 0) {
		_globalObj = JS_NewCompartmentAndGlobalObject(_context, &p->globalClass, NULL);
	} else {
		JS_SetGlobalObject(_context,ScriptControl[0].glob);
		_globalObj = JS_NewGlobalObject(_context,&p->globalClass);
		JS_SetGlobalObject(_context,_globalObj);
	}	
	#else
	_globalObj = JS_NewObject(_context, &p->globalClass, NULL, NULL);
	#endif
#if defined(JS_THREADSAFE)
	JS_EndRequest(_context);
#endif
	if (!_globalObj) freewrlDie("JS_NewObject failed");

	#ifdef JAVASCRIPTVERBOSE 
	printf("\tJS global object created,\n");
	#endif


	/* gets JS standard classes */
#if defined(JS_THREADSAFE)
	JS_BeginRequest(_context);
#endif
	if (!JS_InitStandardClasses(_context, _globalObj))
#if defined(JS_THREADSAFE)
	{	JS_EndRequest(_context);
#endif
		freewrlDie("JS_InitStandardClasses failed");
#if defined(JS_THREADSAFE)
	} else {
		JS_EndRequest(_context);
	}
#endif
	#ifdef JAVASCRIPTVERBOSE 
	printf("\tJS standard classes initialized,\n");
	#endif

	#ifdef JAVASCRIPTVERBOSE 
	 	reportWarningsOn();
	#endif

	JS_SetErrorReporter(_context, errorReporter);

	#ifdef JAVASCRIPTVERBOSE 
	printf("\tJS error reporter set,\n");
	#endif

	br = (BrowserNative *) JS_malloc(_context, sizeof(BrowserNative));

	/* for this script, here are the necessary data areas */
	ScriptControl[num].cx =  _context;
	ScriptControl[num].glob =  _globalObj;


#if defined(JS_THREADSAFE)
	JS_BeginRequest(_context);
#endif
	if (!loadVrmlClasses(_context, _globalObj))
#if defined(JS_THREADSAFE)
	{	JS_EndRequest(_context);
#endif
		freewrlDie("loadVrmlClasses failed");
#if defined(JS_THREADSAFE)
	} else {
		JS_EndRequest(_context);
	}
#endif


	#ifdef JAVASCRIPTVERBOSE 
	printf("\tVRML classes loaded,\n");
	#endif

#if defined(JS_THREADSAFE)
	JS_BeginRequest(_context);
#endif
	if (!VrmlBrowserInit(_context, _globalObj, br))
#if defined(JS_THREADSAFE)
	{	JS_EndRequest(_context);
#endif
		freewrlDie("VrmlBrowserInit failed");
#if defined(JS_THREADSAFE)
	} else {
		JS_EndRequest(_context);
	}
#endif

	#ifdef JAVASCRIPTVERBOSE 
	printf("\tVRML Browser interface loaded,\n");
	#endif


	if (!ACTUALRUNSCRIPT(num,DefaultScriptMethods,&rval))
		cleanupDie(num,"runScript failed in VRML::newJS DefaultScriptMethods");

	/* send this data over to the routing table functions. */
	CRoutes_js_new (num, JAVASCRIPT);

	#ifdef JAVASCRIPTVERBOSE 
	printf("\tVRML browser initialized, thread %u\n",pthread_self());
	#endif
}


/* run the script from within C */
#ifdef JAVASCRIPTVERBOSE
int ActualrunScript(int num, char *script, jsval *rval, char *fn, int line) {
#else
int ActualrunScript(int num, char *script, jsval *rval) {
#endif
	int len;
	JSContext *_context;
	JSObject *_globalObj;
	struct CRscriptStruct *ScriptControl = getScriptControl();


	/* get context and global object for this script */
	_context = (JSContext*)ScriptControl[num].cx;
	_globalObj = (JSObject*)ScriptControl[num].glob;

	#ifdef JAVASCRIPTVERBOSE
		printf("ActualrunScript script called at %s:%d  num: %d cx %p \"%s\", \n", 
			fn, line, num, _context, script);
	#endif

#if defined(JS_THREADSAFE)
	JS_BeginRequest(_context);
#endif
	CLEANUP_JAVASCRIPT(_context)
#if defined(JS_THREADSAFE)
	JS_EndRequest(_context);
#endif

	len = (int) strlen(script);
#if defined(JS_THREADSAFE)
	JS_BeginRequest(_context);
#endif
	if (!JS_EvaluateScript(_context, _globalObj, script, len, FNAME_STUB, LINENO_STUB, rval)) {
		printf ("ActualrunScript - JS_EvaluateScript failed for %s", script);
		printf ("\n");
		ConsoleMessage ("ActualrunScript - JS_EvaluateScript failed for %s", script);
#if defined(JS_THREADSAFE)
		JS_EndRequest(_context);
#endif
		return JS_FALSE;
#if defined(JS_THREADSAFE)
	} else {
		JS_EndRequest(_context);
#endif
	}

	#ifdef JAVASCRIPTVERBOSE 
	printf ("runscript passed\n");
	#endif

	return JS_TRUE;
}


/* run the script from within Javascript  */
int jsrrunScript(JSContext *_context, JSObject *_globalObj, char *script, jsval *rval) {

	int len;

	#ifdef JAVASCRIPTVERBOSE
		printf("jsrrunScript script cx %p \"%s\", \n",
			   _context, script);
	#endif

	len = (int) strlen(script);
#if defined(JS_THREADSAFE)
	JS_BeginRequest(_context);
#endif
	if (!JS_EvaluateScript(_context, _globalObj, script, len,
						   FNAME_STUB, LINENO_STUB, rval)) {
		ConsoleMessage ("jsrunScript - JS_EvaluateScript failed for %s", script);
#if defined(JS_THREADSAFE)
		JS_EndRequest(_context);
#endif
		return JS_FALSE;
#if defined(JS_THREADSAFE)
	} else {
		JS_EndRequest(_context);
#endif
	}

	#ifdef JAVASCRIPTVERBOSE 
	printf ("runscript passed\n");
	#endif

	return JS_TRUE;
}

/* FROM VRMLC.pm */
void *SFNodeNativeNew()
{
	SFNodeNative *ptr;
	ptr = MALLOC(SFNodeNative *, sizeof(*ptr));

	/* printf ("SFNodeNativeNew; string len %d handle_len %d\n",vrmlstring_len,handle_len);*/

	ptr->handle = 0;
	ptr->valueChanged = 0;
	ptr->X3DString = NULL;
	ptr->fieldsExpanded = FALSE;
	return ptr;
}

/* assign this internally to the Javascript engine environment */
int SFNodeNativeAssign(void *top, void *fromp)
{
	SFNodeNative *to = (SFNodeNative *)top;
	SFNodeNative *from = (SFNodeNative *)fromp;

	/* indicate that this was touched; and copy contents over */
	to->valueChanged++;

	if (from != NULL) {
		to->handle = from->handle;
		to->X3DString = STRDUP(from->X3DString);

		#ifdef JAVASCRIPTVERBOSE
		printf ("SFNodeNativeAssign, copied %p to %p, handle %p, string %s\n", from, to, to->handle, to->X3DString);
		#endif
	} else {
		to->handle = 0;
		to->X3DString = STRDUP("from a NULL assignment");
	}

	return JS_TRUE;
}

void *SFColorRGBANativeNew()
{
	SFColorRGBANative *ptr;
	ptr = MALLOC(SFColorRGBANative *, sizeof(*ptr));
	ptr->valueChanged = 0;
	return ptr;
}

void SFColorRGBANativeAssign(void *top, void *fromp)
{
	SFColorRGBANative *to = (SFColorRGBANative *)top;
	SFColorRGBANative *from = (SFColorRGBANative *)fromp;
	to->valueChanged ++;
	(to->v) = (from->v);
}

void *SFColorNativeNew()
{
	SFColorNative *ptr;
	ptr = MALLOC(SFColorNative *, sizeof(*ptr));
	ptr->valueChanged = 0;
	return ptr;
}

void SFColorNativeAssign(void *top, void *fromp)
{
	SFColorNative *to = (SFColorNative *)top;
	SFColorNative *from = (SFColorNative *)fromp;
	to->valueChanged++;
	(to->v) = (from->v);
}

void *SFImageNativeNew()
{
	SFImageNative *ptr;
	ptr =MALLOC(SFImageNative *, sizeof(*ptr));
	ptr->valueChanged = 0;
	return ptr;
}

void SFImageNativeAssign(void *top, void *fromp)
{
	SFImageNative *to = (SFImageNative *)top;
	/* SFImageNative *from = fromp; */
	UNUSED(fromp);

	to->valueChanged++;
/* 	(to->v) = (from->v); */
}

void *SFRotationNativeNew()
{
	SFRotationNative *ptr;
	ptr = MALLOC(SFRotationNative *, sizeof(*ptr));
	ptr->valueChanged = 0;
	return ptr;
}

void SFRotationNativeAssign(void *top, void *fromp)
{
	SFRotationNative *to = (SFRotationNative *)top;
	SFRotationNative *from = (SFRotationNative *)fromp;
	to->valueChanged++;
	(to->v) = (from->v);
}

void *SFVec2fNativeNew()
{
	SFVec2fNative *ptr;
	ptr = MALLOC(SFVec2fNative *, sizeof(*ptr));
	ptr->valueChanged = 0;
	return ptr;
}

void SFVec2fNativeAssign(void *top, void *fromp)
{
	SFVec2fNative *to = (SFVec2fNative *)top;
	SFVec2fNative *from = (SFVec2fNative *)fromp;
	to->valueChanged++;
	(to->v) = (from->v);
}

void *SFVec3fNativeNew() {
	SFVec3fNative *ptr;
	ptr = MALLOC(SFVec3fNative *, sizeof(*ptr));
	ptr->valueChanged = 0;
	return ptr;
}

void SFVec3fNativeAssign(void *top, void *fromp) {
	SFVec3fNative *to = (SFVec3fNative *)top;
	SFVec3fNative *from = (SFVec3fNative *)fromp;
	to->valueChanged++;
	(to->v) = (from->v);
}

void *SFVec3dNativeNew() {
	SFVec3dNative *ptr;
	ptr = MALLOC(SFVec3dNative *, sizeof(*ptr));
	ptr->valueChanged = 0;
	return ptr;
}

void SFVec3dNativeAssign(void *top, void *fromp) {
	SFVec3dNative *to = (SFVec3dNative *)top;
	SFVec3dNative *from = (SFVec3dNative *)fromp;
	to->valueChanged++;
	(to->v) = (from->v);
}

void *SFVec4fNativeNew() {
	SFVec4fNative *ptr;
	ptr = MALLOC(SFVec4fNative *, sizeof(*ptr));
	ptr->valueChanged = 0;
	return ptr;
}

void SFVec4fNativeAssign(void *top, void *fromp) {
	SFVec4fNative *to = (SFVec4fNative *)top;
	SFVec4fNative *from = (SFVec4fNative *)fromp;
	to->valueChanged++;
	(to->v) = (from->v);
}

void *SFVec4dNativeNew() {
	SFVec4dNative *ptr;
	ptr = MALLOC(SFVec4dNative *, sizeof(*ptr));
	ptr->valueChanged = 0;
	return ptr;
}

void SFVec4dNativeAssign(void *top, void *fromp) {
	SFVec4dNative *to = (SFVec4dNative *)top;
	SFVec4dNative *from = (SFVec4dNative *)fromp;
	to->valueChanged++;
	(to->v) = (from->v);
}


/* A new version of InitScriptField which takes "nicer" arguments; currently a
 * simple and restricted wrapper, but it could replace it soon? */
/* Parameters:
	num:		Script number. Starts at 0. 
	kind:		One of PKW_initializeOnly PKW_outputOnly PKW_inputOutput PKW_inputOnly
	type:		One of the FIELDTYPE_ defines, eg, FIELDTYPE_MFFloat
	field:		the field name as found in the VRML/X3D file. eg "set_myField"
		
*/

/* save this field from the parser; initialize it when the fwl_RenderSceneUpdateScene wants to initialize it */
void SaveScriptField (int num, indexT kind, indexT type, const char* field, union anyVrml value) {
	struct ScriptParamList **nextInsert;
	struct ScriptParamList *newEntry;
	struct CRscriptStruct *ScriptControl = getScriptControl();
	ppJScript p = (ppJScript)gglobal()->JScript.prv;

	if (num >= p->JSMaxScript)  {
		ConsoleMessage ("JSSaveScriptText: warning, script %d initialization out of order",num);
		return;
	}

	/* generate a new ScriptParamList entry */
	/* note that this is a linked list, and we put things on at the end. The END MUST
	   have NULL termination */
	nextInsert = &(ScriptControl[num].paramList);
	while (*nextInsert != NULL) {
		nextInsert = &(*nextInsert)->next;
	}

	/* create a new entry and link it in */
	newEntry = MALLOC (struct ScriptParamList *, sizeof (struct ScriptParamList));
	*nextInsert = newEntry;
	
	/* initialize the new entry */
	newEntry->next = NULL;
	newEntry->kind = kind;
	newEntry->type = type;
	newEntry->field = STRDUP(field);
	newEntry->value = value;
}

static char* re_strcat(char *_Dest, char *_Source, int *destLen, int *destDim)
{
	/* strcats, but first checks strlen on source and destination
	   and reallocs if necessary - good when you are doing a lot of strcatting of un-pre-known elements
	   (Q. is there something for this already?)
	   _Dest, _Source - as with strcat(_Dest,_Source)
	   destLen - current cumulative strlen(_Dest)
	   destDim - current malloc/realloc dimension
	   Usage example:
		dstdim = (rows+1)*(elements*15) + 100; //a guess
		dstlen = 0;
		smallfield = MALLOC (char *, dstdim+1); //rows+1)*(elements*15) + 100);
		smallfield[0] = '\0';
		...
		for(;;)
		{
			...
			smallfield = re_strcat(smallfield, "new ",&dstlen,&dstdim);
		...
		FREE_IF_NZ(smallfield)
	*/
	int srclen = (int) strlen(_Source);
	*destLen = *destLen + srclen;
	if(*destLen > *destDim -1)
	{
		*destDim = *destDim + srclen + 1 + 100;
		_Dest = realloc(_Dest,*destDim);
	}
	_Dest = strcat(_Dest,_Source);
	return _Dest;
}
/* the fwl_RenderSceneUpdateScene is initializing this field now */
static void InitScriptField(int num, indexT kind, indexT type, const char* field, union anyVrml value) {
	jsval rval;
	char *smallfield = NULL;
	char mynewname[400];
	char *thisValue;
	int rows, elements;
	char *sftype = NULL;

	int haveMulti;
	int MFhasECMAtype;
	int rowCount, eleCount;

	int tlen;
	float *FloatPtr;
	struct X3D_Node **VoidPtr;
	int *IntPtr;
	double *DoublePtr;
	struct Uni_String **SVPtr;

	float defaultFloat[] = {0.0f,0.0f,0.0f,0.0f};
	int defaultInt[] = {0,0,0,0};
	double defaultDouble[] = {0.0, 0.0, 0.0, 0.0};
	struct Uni_String *sptr[1];
	struct X3D_Node *defaultVoid[] = {NULL,NULL};
	struct CRscriptStruct *ScriptControl = getScriptControl();

	#ifdef JAVASCRIPTVERBOSE
	printf ("calling InitScriptField from thread %u\n",pthread_self());
	printf ("\nInitScriptField, num %d, kind %s type %s field %s value %d\n", num,PROTOKEYWORDS[kind],FIELDTYPES[type],field,value);
	#endif

        if ((kind != PKW_inputOnly) && (kind != PKW_outputOnly) && (kind != PKW_initializeOnly) && (kind != PKW_inputOutput)) {
                ConsoleMessage ("InitScriptField: invalid kind for script: %d\n",kind);
                return;
        }

        if (type >= FIELDTYPES_COUNT) {
                ConsoleMessage ("InitScriptField: invalid type for script: %d\n",type);
                return;
        }

	/* first, make a new name up */
	if (kind == PKW_inputOnly) {
		sprintf (mynewname,"__eventIn_Value_%s",field);
	} else strcpy(mynewname,field);

	/* ok, lets handle the types here */
	switch (type) {
		/* ECMA types */
		case FIELDTYPE_SFBool:
		case FIELDTYPE_SFFloat:
		case FIELDTYPE_SFTime:
		case FIELDTYPE_SFDouble:
		case FIELDTYPE_SFInt32:
		case FIELDTYPE_SFString: {
			/* do not care about eventIns */
			if (kind != PKW_inputOnly)  {
				JSaddGlobalECMANativeProperty(num, field);
				if (kind == PKW_initializeOnly) {
					if  (type == FIELDTYPE_SFString) {
						tlen = (int) strlen(value.sfstring->strptr) + strlen(field) + 20;
					} else {
						tlen = (int) strlen(field) + 400; /* long, in the case of doubles */
					}
					smallfield = MALLOC (char *, tlen);
					smallfield[0] = '\0';

					switch (type) {
						case FIELDTYPE_SFFloat: sprintf (smallfield,"%s=%f\n",field,value.sffloat);break;
						case FIELDTYPE_SFTime: sprintf (smallfield,"%s=%f\n",field,value.sftime);break;
						case FIELDTYPE_SFDouble: sprintf (smallfield,"%s=%f\n",field,value.sftime);break;
						case FIELDTYPE_SFInt32: sprintf (smallfield,"%s=%d\n",field,value.sfint32); break;
						case FIELDTYPE_SFBool: 
							if (value.sfbool == 1) sprintf (smallfield,"%s=true",field);
							else sprintf (smallfield,"%s=false",field);
							break;
						case FIELDTYPE_SFString:  
							sprintf (smallfield,"%s=\"%s\"\n",field,value.sfstring->strptr); break;
					}

					if (!ACTUALRUNSCRIPT(num,smallfield,&rval))
						printf ("huh??? Field initialization script failed %s\n",smallfield);
				}
			}
			break;
		}
		/* non ECMA types */
		default: {
			/* get an appropriate pointer - we either point to the initialization value
			   in the script header, or we point to some data here that are default values */
			
			/* does this MF type have an ECMA type as a single element? */
			switch (type) {
				case FIELDTYPE_MFString:
				case FIELDTYPE_MFTime:
				case FIELDTYPE_MFBool:
				case FIELDTYPE_MFInt32:
				case FIELDTYPE_MFFloat: 
				JSaddGlobalECMANativeProperty(num, field);
					MFhasECMAtype = TRUE;
					break;
				default: {
					MFhasECMAtype = FALSE;
				}
			}

			elements=0;
			IntPtr = NULL;
			FloatPtr = NULL;
			DoublePtr = NULL;
			SVPtr = NULL;
			VoidPtr = NULL;
			if (kind == PKW_initializeOnly) {
				switch (type) {
					case FIELDTYPE_SFImage:
						VoidPtr = (struct X3D_Node **) (&(value.sfimage)); elements = 1;
						break;
					case FIELDTYPE_SFNode:
						VoidPtr = (struct X3D_Node **) (&(value.sfnode)); elements = 1;
						break;
					case FIELDTYPE_MFColor:
						FloatPtr = (float *) value.mfcolor.p; elements = value.mfcolor.n;
						break;
					case FIELDTYPE_MFColorRGBA:
						FloatPtr = (float *) value.mfcolorrgba.p; elements = value.mfcolorrgba.n;
						break;
					case FIELDTYPE_MFVec2f:
						FloatPtr = (float *) value.mfvec2f.p; elements = value.mfvec2f.n;
						break;
					case FIELDTYPE_MFVec3f:
						FloatPtr = (float *) value.mfvec3f.p; elements = value.mfvec3f.n;
						break;
					case FIELDTYPE_MFRotation: 
						FloatPtr = (float *) value.mfrotation.p; elements = value.mfrotation.n;
						break;
					case FIELDTYPE_SFVec2f:
						FloatPtr = (float *) value.sfvec2f.c; elements = 1;
						break;
					case FIELDTYPE_SFColor:
						FloatPtr = value.sfcolor.c; elements = 1;
						break;
					case FIELDTYPE_SFColorRGBA:
						FloatPtr = value.sfcolorrgba.c; elements = 1;
						break;
					case FIELDTYPE_SFRotation:
						FloatPtr = value.sfrotation.c; elements = 1;
						break;
					case FIELDTYPE_SFVec3f: 
						FloatPtr = value.sfvec3f.c; elements =1;
						break;
					case FIELDTYPE_SFVec3d: 
						DoublePtr = value.sfvec3d.c; elements =1;
						break;
					case FIELDTYPE_MFString:
						SVPtr = value.mfstring.p; elements = value.mfstring.n;
						break;
					case FIELDTYPE_MFTime:
						DoublePtr = value.mftime.p; elements = value.mftime.n;
						break;
					case FIELDTYPE_MFBool:
						IntPtr = value.mfbool.p; elements = value.mfbool.n;
						break;
					case FIELDTYPE_MFInt32:
						IntPtr = value.mfint32.p; elements = value.mfint32.n;
						break;
					case FIELDTYPE_MFNode:
						VoidPtr = (struct X3D_Node **)(value.mfnode.p); elements = value.mfnode.n;
						break;
					case FIELDTYPE_MFFloat: 
						FloatPtr = value.mffloat.p; elements = value.mffloat.n;
						break;
					case FIELDTYPE_SFVec4f:
						FloatPtr = value.sfvec4f.c; elements = 1;
						break;
					case FIELDTYPE_SFVec4d:
						DoublePtr = value.sfvec4d.c; elements = 1;
						break;

					default: {
						printf ("unhandled type, in InitScriptField %d\n",type);
						return;
					}
				}

			} else {
				/* make up a default pointer */
				elements = 1;
				switch (type) {
					/* Void types */
					case FIELDTYPE_SFNode:
					case FIELDTYPE_MFNode:
						VoidPtr = (struct X3D_Node **) &defaultVoid;
						break;

					/* Float types */
					case FIELDTYPE_MFColor:
					case FIELDTYPE_MFColorRGBA:
					case FIELDTYPE_MFVec2f:
					case FIELDTYPE_MFVec3f:
					case FIELDTYPE_MFRotation: 
					case FIELDTYPE_SFVec2f:
					case FIELDTYPE_SFColor:
					case FIELDTYPE_SFColorRGBA:
					case FIELDTYPE_SFRotation:
					case FIELDTYPE_SFVec3f: 
					case FIELDTYPE_SFVec4f: 
					case FIELDTYPE_MFFloat: 
						FloatPtr = defaultFloat;
						break;

					/* Int types */
					case FIELDTYPE_MFBool:
					case FIELDTYPE_MFInt32:
						IntPtr = defaultInt;
						break;

					/* String types */
					case FIELDTYPE_SFString:
					case FIELDTYPE_MFString:
						sptr[0] = newASCIIString("");
						SVPtr = sptr;
						break;

					/* SFImage */
					case FIELDTYPE_SFImage:
						IntPtr = defaultInt;
						break;

					/* Double types */
					case FIELDTYPE_SFVec2d:
					case FIELDTYPE_SFVec3d:
					case FIELDTYPE_MFTime:
					case FIELDTYPE_SFTime:
					case FIELDTYPE_SFDouble:
					case FIELDTYPE_SFVec4d:
						DoublePtr = defaultDouble;
						break;
						
					default: {
						printf ("unhandled type, in InitScriptField part 2 %d\n",type);
						return;
					}
				}

			}

			rows = returnElementRowSize (type);

			#ifdef JAVASCRIPTVERBOSE
			printf ("in fieldSet, we have ElementRowSize %d and individual elements %d\n",rows,elements);
			#endif

			/* make this at least as large as required, then add some more on to the end... */
			/*
			Old Approach 
					step1: compute using guestimate formulas
					step2: malloc
					step3: loop through strcat() and hope no overrun 
				Problem: heap corruption from array overrun - the guestimate has been bad 
				    a few times in 2010 with MFVec2fs and MFStrings with 42 and 47 elements, strings of varying length
				example for MFVec2f
				'new MFVec2f(new SFVec2f(1234.678910,1234.678910),...)'
				each SF 2 numbers each 10 digits plus new type(,), 15 chars  =35.
				3 x 15 = 45 (or (rows+1)x(elements*15)+100) 
				old formula falls short:
					old formula: smallfield = MALLOC (rows*((elements*15) + 100));
					example 47 SFVec2fs
					actual bytes: 47 x 35 bytes = 1645 + 13 for the MF = 1658
					old formula  2 x ((47*15)+100) = 1610   //thats 48 bytes short and I bomb out
					better formula  3 x (47*15) + 100 = 2215
			New Approach (July 28, 2010)
					step1: compute using guestimate formulas
					step2: malloc
					step3: loop through and realloc before strcat() if short
			*/
			{
				int dstlen, dstdim, tdim;
				tdim = 200;
				thisValue = MALLOC(char *, tdim+1);
				dstdim = (rows+1)*(elements*15) + 100; /* a guess */
				dstlen = 0;
				smallfield = MALLOC (char *, dstdim+1); //rows+1)*(elements*15) + 100);
				/* what is the equivalent SF for this MF?? */
				if (type != convertToSFType(type)) haveMulti = TRUE;
				 else haveMulti = FALSE;
				
				/* the sftype is the SF form of either the MF or SF */
				sftype = STRDUP((char *)FIELDTYPES[convertToSFType(type)]);

				/* SFStrings are Strings */
				if (strncmp(sftype,"SFString",8)==0) strcpy (sftype,"String");


				/* start the string */
				smallfield[0] = '\0';

				/* is this an MF variable, with SFs in it? */
				if (haveMulti) {
					smallfield = re_strcat(smallfield, "new ",&dstlen,&dstdim);
					smallfield = re_strcat(smallfield, (char *)FIELDTYPES[type],&dstlen,&dstdim);
					smallfield = re_strcat(smallfield, "(",&dstlen,&dstdim);
				}

				/* loop through, and put values in */
				for (eleCount=0; eleCount<elements; eleCount++) {
					/* ECMA native types can just be passed in... */
					if (!MFhasECMAtype) {
						smallfield = re_strcat(smallfield, "new ",&dstlen,&dstdim);
						smallfield = re_strcat(smallfield, sftype,&dstlen,&dstdim);
						smallfield = re_strcat(smallfield, "(",&dstlen,&dstdim);
					}

					/* go through the SF type; SFints will have 1; SFVec3f's will have 3, etc */
					for (rowCount=0; rowCount<rows; rowCount++ ) {
						if (IntPtr != NULL) {
							sprintf (thisValue,"%d",*IntPtr); IntPtr++;
						} else if (FloatPtr != NULL) {
							sprintf (thisValue,"%f",*FloatPtr); FloatPtr++;
						} else if (DoublePtr != NULL) {
							sprintf (thisValue,"%f",*DoublePtr); DoublePtr++;
						} else if (SVPtr != NULL) {
							sptr[0] = *SVPtr; SVPtr++;
							if(strlen(sptr[0]->strptr)+2 > tdim-1)
							{	
								tdim = (int) strlen(sptr[0]->strptr) + 1 + 100;
								thisValue = realloc(thisValue,tdim);
							}
							sprintf (thisValue,"\"%s\"",sptr[0]->strptr);
						} else { /* must be a Void */
							/* printf ("sending in a VoidPtr, it is %p\n",VoidPtr[0]);
							if (VoidPtr[0] != NULL) {printf ("it is a %s type\n",stringNodeType(X3D_NODE(VoidPtr[0])->_nodeType));} */
							sprintf (thisValue,"\"%p\"", VoidPtr[0]); VoidPtr++;
						}
						smallfield = re_strcat(smallfield, thisValue,&dstlen,&dstdim);
						if (rowCount < (rows-1)) smallfield = re_strcat(smallfield,",",&dstlen,&dstdim);
					}

					if (!MFhasECMAtype) smallfield = re_strcat(smallfield, ")",&dstlen,&dstdim);
					if (eleCount < (elements-1)) smallfield = re_strcat(smallfield,",",&dstlen,&dstdim);

				}


				if (haveMulti) {
					smallfield = re_strcat(smallfield,")",&dstlen,&dstdim);
				}
				/* printf("dstlen=%d dstdim=%d\n",dstlen,dstdim); */
				FREE_IF_NZ (thisValue);
			}
			/* Warp factor 5, Dr Sulu... */
			#ifdef JAVASCRIPTVERBOSE 
			printf ("JScript, for non-ECMA newname %s, sending :%s:\n",mynewname,smallfield); 
			#endif

			JSaddGlobalAssignProperty (num,mynewname,smallfield);
		}
	}

	/* Fields can generate an event, so we allow the touched flag to remain set. eventOuts have just
	   been initialized, and as such, should not send events, until after they really have been set.
	*/
	if (kind == PKW_outputOnly) {
		int fptr;
		int touched;

		UNUSED(touched); // compiler warning mitigation

		/* get the number representing this type */
		fptr = JSparamIndex (field, FIELDTYPES[type]);

		/* set up global variables so that we can reset the touched flag */
		touched = get_valueChanged_flag (fptr, num);

		/* and, reset the touched flag, knowing that we have the variables set properly */
		resetScriptTouchedFlag(num, fptr); 
	}

#if defined(JS_THREADSAFE)
	JS_BeginRequest(ScriptControl[num].cx);
#endif
	CLEANUP_JAVASCRIPT(ScriptControl[num].cx)
#if defined(JS_THREADSAFE)
	JS_EndRequest(ScriptControl[num].cx);
#endif

	FREE_IF_NZ (smallfield);
	FREE_IF_NZ (sftype);

	#ifdef JAVASCRIPTVERBOSE
	printf ("finished InitScriptField\n");
	#endif
}

static int JSaddGlobalECMANativeProperty(int num, const char *name) {
	JSContext *_context;
	JSObject *_globalObj;
	jsval rval = INT_TO_JSVAL(0);
	struct CRscriptStruct *ScriptControl = getScriptControl();

	/* get context and global object for this script */
	_context =  (JSContext*)ScriptControl[num].cx;
	_globalObj = (JSObject*)ScriptControl[num].glob;

	#ifdef  JAVASCRIPTVERBOSE
		printf("addGlobalECMANativeProperty: name \"%s\"\n", name);
	#endif

#if defined(JS_THREADSAFE)
	JS_BeginRequest(_context);
#endif

/* Note, for JS-185+, JSPROP_PERMANENT makes properties non-configurable, which can cause runtime 
 * errors from the JS engine when said property gets redefined to a function by the script.  The
 * example file tests/Javascript_tests/MFFloat.wrl had this issue. */

	if (!JS_DefineProperty(_context, _globalObj, name, rval, NULL, setECMANative, 
#if JS_VERSION < 185
		0 | JSPROP_PERMANENT
#else
		0
#endif
	)) {
		printf("JS_DefineProperty failed for \"%s\" in addGlobalECMANativeProperty.\n", name);
#if defined(JS_THREADSAFE)
		JS_EndRequest(_context);
#endif
		return JS_FALSE;
#if defined(JS_THREADSAFE)
	} else {
		JS_EndRequest(_context);
#endif
	}

	return JS_TRUE;
}

static int JSaddGlobalAssignProperty(int num, const char *name, const char *str) {
	jsval _rval = INT_TO_JSVAL(0);
	JSContext *_context;
	JSObject *_globalObj;
	struct CRscriptStruct *ScriptControl = getScriptControl();

	/* get context and global object for this script */
	_context =  (JSContext*)ScriptControl[num].cx;
	_globalObj = (JSObject*)ScriptControl[num].glob;

	#ifdef JAVASCRIPTVERBOSE 
		printf("addGlobalAssignProperty: cx: %p obj %p name \"%s\", evaluate script \"%s\"\n",
			   _context, _globalObj, name, str);
	#endif

#if defined(JS_THREADSAFE)
	JS_BeginRequest(_context);
#endif
	if (!JS_EvaluateScript(_context, _globalObj, str, (int) strlen(str), FNAME_STUB, LINENO_STUB, &_rval)) {
		ConsoleMessage ("JSaddGlobalAssignProperty - JS_EvaluateScript failed for %s", str);
#if defined(JS_THREADSAFE)
		JS_EndRequest(_context);
#endif
		return JS_FALSE;
#if defined(JS_THREADSAFE)
	} else {
		JS_EndRequest(_context);
#endif
	}
#if defined(JS_THREADSAFE)
	JS_BeginRequest(_context);
#endif
	if (!JS_DefineProperty(_context, _globalObj, name, _rval, getAssignProperty, setAssignProperty, 0 | JSPROP_PERMANENT)) {
		printf("JS_DefineProperty failed for \"%s\" in addGlobalAssignProperty.\n", name);
#if defined(JS_THREADSAFE)
		JS_EndRequest(_context);
#endif
		return JS_FALSE;
#if defined(JS_THREADSAFE)
	} else {
		JS_EndRequest(_context);
#endif
	}
	return JS_TRUE;
}

/* defines for getting touched flags and exact Javascript pointers */

/* ... make a #define to handle JS requests that can easily be substituted into these other #defines */
#if defined(JS_THREADSAFE)
# define JSBEGINREQUEST_SUBSTITUTION(mycx) JS_BeginRequest(mycx);
# define JSENDREQUEST_SUBSTITUTION(mycx) JS_EndRequest(mycx);
#else
# define JSBEGINREQUEST_SUBSTITUTION(mycx) /* */
# define JSENDREQUEST_SUBSTITUTION(mycx) /* */
#endif

/****************************** ECMA types ******************************************/
/* where we have a Native structure to go along with it */
#define GETJSPTR_TYPE_A(thistype) \
			 case FIELDTYPE_##thistype:  {  \
				thistype##Native *ptr; \
				/* printf ("getting private data in GETJSPTR for %p \n",JSglobal_return_val); */ \
        			if ((ptr = (thistype##Native *)JS_GetPrivate(cx, JSVAL_TO_OBJECT(*(jsval *)(tg->JScript.JSglobal_return_val)))) == NULL) { \
                			printf( "JS_GetPrivate failed in get_valueChanged_flag\n"); \
					JSENDREQUEST_SUBSTITUTION(cx) \
                			return JS_FALSE; \
				} \
				/* if (ptr->valueChanged > 0) printf ("private is %d valueChanged %d\n",ptr,ptr->valueChanged); */ \
				tg->CRoutes.JSSFpointer = (void *)ptr; /* save this for quick extraction of values */ \
				touched = ptr->valueChanged; \
				break; \
			} 

#define RESET_TOUCHED_TYPE_A(thistype) \
                case FIELDTYPE_##thistype: { \
                        ((thistype##Native *)tg->CRoutes.JSSFpointer)->valueChanged = 0; \
                        break; \
                }       

#define GETJSPTR_TYPE_MF_A(thisMFtype,thisSFtype) \
	case FIELDTYPE_##thisMFtype: { \
		thisSFtype##Native *ptr; \
		jsval mainElement; \
		int len; \
		int i; \
		if (!JS_GetProperty(cx, JSVAL_TO_OBJECT(*(jsval *)(tg->JScript.JSglobal_return_val)), "length", &mainElement)) { \
			printf ("JS_GetProperty failed for \"length\" in get_valueChanged_flag\n"); \
			JSENDREQUEST_SUBSTITUTION(cx) \
			return FALSE; \
		} \
		len = JSVAL_TO_INT(mainElement); \
		/* go through each element of the main array. */ \
		for (i = 0; i < len; i++) { \
			if (!JS_GetElement(cx, JSVAL_TO_OBJECT(*(jsval*)(tg->JScript.JSglobal_return_val)), i, &mainElement)) { \
				printf ("JS_GetElement failed for %d in get_valueChanged_flag\n",i); \
				JSENDREQUEST_SUBSTITUTION(cx) \
				return FALSE; \
			} \
			if ((ptr = (thisSFtype##Native *)JS_GetPrivate(cx, JSVAL_TO_OBJECT(mainElement))) == NULL) { \
				printf( "JS_GetPrivate failed for obj in setField_javascriptEventOut.\n"); \
				JSENDREQUEST_SUBSTITUTION(cx) \
				return FALSE; \
			} \
			if (ptr->valueChanged > 0) touched = TRUE; /* did this element change? */ \
			/* printf ("touched flag for element %d is %d\n",i,ptr->touched); */ \
		} \
		break; \
	} 

#define RESET_TOUCHED_TYPE_MF_A(thisMFtype,thisSFtype) \
	case FIELDTYPE_##thisMFtype: { \
		thisSFtype##Native *ptr; \
		jsval mainElement; \
		int len; \
		int i; \
		JSContext *cx; \
		cx = scriptcontrol->cx; \
		JSBEGINREQUEST_SUBSTITUTION(cx) \
		if (!JS_GetProperty(cx, JSVAL_TO_OBJECT(*(jsval*)(tg->JScript.JSglobal_return_val)), "length", &mainElement)) { \
			printf ("JS_GetProperty failed for \"length\" in get_valueChanged_flag\n"); \
			JSENDREQUEST_SUBSTITUTION(cx) \
			break; \
		} \
		len = JSVAL_TO_INT(mainElement); \
		/* go through each element of the main array. */ \
		for (i = 0; i < len; i++) { \
			if (!JS_GetElement(cx, JSVAL_TO_OBJECT(*(jsval*)(tg->JScript.JSglobal_return_val)), i, &mainElement)) { \
				printf ("JS_GetElement failed for %d in get_valueChanged_flag\n",i); \
				JSENDREQUEST_SUBSTITUTION(cx) \
				break; \
			} \
			if ((ptr = (thisSFtype##Native *)JS_GetPrivate(cx, JSVAL_TO_OBJECT(mainElement))) == NULL) { \
				printf( "JS_GetPrivate failed for obj in setField_javascriptEventOut.\n"); \
				JSENDREQUEST_SUBSTITUTION(cx) \
				break; \
			} \
			ptr->valueChanged = 0; \
		} \
		JSENDREQUEST_SUBSTITUTION(cx) \
		break; \
	} 

/****************************** ECMA types ******************************************/

/* "Bool" might be already declared - we DO NOT want it to be declared as an "int" */
#define savedBool Bool
#ifdef Bool
#undef Bool
#endif

/* NOTE - BeginRequest is already called prior to any GET_* defines */

#define GET_ECMA_TOUCHED(thistype) \
	case FIELDTYPE_SF##thistype: {	\
				touched = findNameInECMATable( scriptcontrol->cx,fullname);\
				break;\
			}

#define GET_ECMA_MF_TOUCHED(thistype) \
	case FIELDTYPE_MF##thistype: {\
		jsval mainElement; \
		/* printf ("GET_ECMA_MF_TOUCHED called on %d\n",JSglobal_return_val);  */ \
		if (!JS_GetProperty(cx, JSVAL_TO_OBJECT(*(jsval*)(tg->JScript.JSglobal_return_val)), "MF_ECMA_has_changed", &mainElement)) { \
			printf ("JS_GetProperty failed for \"MF_ECMA_HAS_changed\" in get_valueChanged_flag\n"); \
		} /* else printf ("GET_ECMA_MF_TOUCHED MF_ECMA_has_changed is %d for %d %d\n",JSVAL_TO_INT(mainElement),cx,JSglobal_return_val); */  \
		touched = JSVAL_TO_INT(mainElement);\
		break; \
	}

#define RESET_ECMA_MF_TOUCHED(thistype) \
	case FIELDTYPE_##thistype: {\
		jsval myv = INT_TO_JSVAL(0); \
		/* printf ("RESET_ECMA_MF_TOUCHED called on %d ",JSglobal_return_val); */ \
		JSBEGINREQUEST_SUBSTITUTION(p->ScriptControl[actualscript].cx) \
        	if (!JS_SetProperty(scriptcontrol->cx, JSVAL_TO_OBJECT(*(jsval*)(tg->JScript.JSglobal_return_val)), "MF_ECMA_has_changed", &myv)) { \
        		printf( "JS_SetProperty failed for \"MF_ECMA_has_changed\" in RESET_ECMA_MF_TOUCHED.\n"); \
        	}\
                /* if (!JS_GetProperty( p->ScriptControl[actualscript].cx, JSVAL_TO_OBJECT(JSglobal_return_val), "MF_ECMA_has_changed", &mainElement)) { \
                        printf ("JS_GetProperty failed for \"MF_ECMA_HAS_changed\" in get_valueChanged_flag\n"); \
		} \
                printf ("and MF_ECMA_has_changed is %d\n",JSVAL_TO_INT(mainElement)); */\
		JSENDREQUEST_SUBSTITUTION(scriptcontrol->cx) \
	break; \
	}

#define RESET_TOUCHED_TYPE_ECMA(thistype) \
			case FIELDTYPE_##thistype: { \
				JSBEGINREQUEST_SUBSTITUTION(scriptcontrol->cx) \
				resetNameInECMATable( scriptcontrol->cx,JSparamnames[fptr].name); \
				JSENDREQUEST_SUBSTITUTION(scriptcontrol->cx) \
				break; \
			}
/* in case Bool was defined above, restore the value */
#define Bool savedBool




/********************************************************************************/
/*									    	*/
/* get_valueChanged_flag - see if this variable (can be a sub-field; see tests   	*/
/* 8.wrl for the DEF PI PositionInterpolator). return true if variable is   	*/
/* touched, and pointer to touched value is in global variable              	*/
/* JSglobal_return_val, AND possibly:						*/
/*	void *JSSFpointer for SF non-ECMA nodes.				*/
/* 										*/
/* the way touched, and, the actual values work is as follows:			*/
/*										*/
/* keep track of the name in a table, and set valueChanged flag.		*/
/* look around the function setECMANative to see how this is done.		*/
/* FIELDTYPE_SFInt32								*/
/* FIELDTYPE_SFBool								*/
/* FIELDTYPE_SFFloat								*/
/* FIELDTYPE_SFTime								*/
/* FIELDTYPE_SFDouble								*/
/* FIELDTYPE_SFString								*/
/*										*/
/* check the "touched" flag for non-zero in the private area:			*/
/* FIELDTYPE_SFRotation								*/
/* FIELDTYPE_SFNode								*/
/* FIELDTYPE_SFVec2f								*/
/* FIELDTYPE_SFVec3f								*/
/* FIELDTYPE_SFImage								*/
/* FIELDTYPE_SFColor								*/
/* FIELDTYPE_SFColorRGBA							*/
/*										*/
/* go through all elements, and find if at least one SF has been touched:	*/
/* FIELDTYPE_MFRotation								*/
/* FIELDTYPE_MFNode								*/
/* FIELDTYPE_MFVec2f								*/
/* FIELDTYPE_MFVec3f								*/
/* FIELDTYPE_MFColor								*/
/* FIELDTYPE_MFColorRGBA							*/


/* has a flag called "MF_ECMA_has_changed" that is used here 			*/
/* FIELDTYPE_MFFloat	*/
/* FIELDTYPE_MFBool	*/
/* FIELDTYPE_MFInt32	*/
/* FIELDTYPE_MFTime	*/
/* FIELDTYPE_MFString	*/
/*                                                                          */
/****************************************************************************/

int get_valueChanged_flag (int fptr, int actualscript) {

#ifdef HAVE_JAVASCRIPT
	struct CRscriptStruct *scriptcontrol;
	JSContext *cx;
	JSObject *interpobj;
	char *fullname;
	int touched;
	ppJScript p;
	ttglobal tg = gglobal();
	struct CRjsnameStruct *JSparamnames = getJSparamnames();
	p = (ppJScript)tg->JScript.prv;

	touched = FALSE;
	scriptcontrol = getScriptControlIndex(actualscript);
	interpobj = (JSObject*)scriptcontrol->glob;
	cx =  (JSContext*)scriptcontrol->cx;
	fullname = JSparamnames[fptr].name;

#if defined(JS_THREADSAFE)
	JS_BeginRequest(cx);
#endif
	#ifdef CRVERBOSE
	printf ("\ngetting property for fullname %s, cx %p, interpobj %d script %d, fptr %d (%s:%s)\n",
		fullname,cx,interpobj,actualscript, fptr,
		JSparamnames[fptr].name, FIELDTYPES[JSparamnames[fptr].type]);
	#endif

	if (!JS_GetProperty(cx,  interpobj ,fullname,tg->JScript.JSglobal_return_val)) {
               	printf ("cant get property for %s\n",fullname);
#if defined(JS_THREADSAFE)
		JS_EndRequest(cx);
#endif
		return FALSE;
        } else {
		#ifdef CRVERBOSE
		printf ("so, property is %d (%p)\n",tg->CRoutes.JSglobal_return_val,tg->CRoutes.JSglobal_return_val);
		printf("get_valueChanged_flag: node type: %s name %s\n",FIELDTYPES[JSparamnames[fptr].type],JSparamnames[fptr].name);
		#endif

		switch (JSparamnames[fptr].type) {
			GETJSPTR_TYPE_A(SFRotation)
			GETJSPTR_TYPE_A(SFNode)
			GETJSPTR_TYPE_A(SFVec2f)
			/* GETJSPTR_TYPE_A(SFVec2d) */
			GETJSPTR_TYPE_A(SFVec3f)
			GETJSPTR_TYPE_A(SFVec3d)
			GETJSPTR_TYPE_A(SFVec4f)
			GETJSPTR_TYPE_A(SFVec4d)
			GETJSPTR_TYPE_A(SFImage)
			GETJSPTR_TYPE_A(SFColor)
			GETJSPTR_TYPE_A(SFColorRGBA)

			GETJSPTR_TYPE_MF_A(MFRotation,SFRotation)
			GETJSPTR_TYPE_MF_A(MFNode,SFNode)
			GETJSPTR_TYPE_MF_A(MFVec2f,SFVec2f)
			GETJSPTR_TYPE_MF_A(MFVec3f,SFVec3f)
			GETJSPTR_TYPE_MF_A(MFVec4f,SFVec4f)
			GETJSPTR_TYPE_MF_A(MFVec4d,SFVec4d)
			/* GETJSPTR_TYPE_MF_A(MFImage,SFImage)  */
			GETJSPTR_TYPE_MF_A(MFColor,SFColor)
			GETJSPTR_TYPE_MF_A(MFColorRGBA,SFColorRGBA)
			
			GET_ECMA_MF_TOUCHED(Int32)
			GET_ECMA_MF_TOUCHED(Bool)
			GET_ECMA_MF_TOUCHED(Time)
			GET_ECMA_MF_TOUCHED(Double)
			GET_ECMA_MF_TOUCHED(Float)
			GET_ECMA_MF_TOUCHED(String)

			GET_ECMA_TOUCHED(Int32) 
			GET_ECMA_TOUCHED(Bool) 
			GET_ECMA_TOUCHED(Float)
			GET_ECMA_TOUCHED(Time)
			GET_ECMA_TOUCHED(Double)
			GET_ECMA_TOUCHED(String)
			
			default: {printf ("not handled yet in get_valueChanged_flag %s\n",FIELDTYPES[JSparamnames[fptr].type]);
			}
		}
#if defined(JS_THREADSAFE)
		JS_EndRequest(cx);
#endif
	}

#ifdef CHECKER
	if (JSparamnames[fptr].type == FIELDTYPE_MFString) {
		int len; int i;
                jsval mainElement; 
                int len; 

		unsigned CRCCheck = 0;
                cx = p->ScriptControl[actualscript].cx; 
#if defined(JS_THREADSAFE)
		JS_BeginRequest(cx);
#endif
                if (!JS_GetProperty(cx, JSglobal_return_val, "length", &mainElement)) { 
                        printf ("JS_GetProperty failed for length_flag\n"); 
                } 
                len = JSVAL_TO_INT(mainElement); 
                /* go through each element of the main array. */ 
                for (i = 0; i < len; i++) { 
                        if (!JS_GetElement(cx, JSglobal_return_val, i, &mainElement)) { 
                                printf ("JS_GetElement failed for %d in get_valueChanged_flag\n",i); 
                                break; 
                        } 
		CRCCheck += (unsigned) mainElement;

/*
                if (JSVAL_IS_OBJECT(mainElement)) printf ("sc, element %d is an OBJECT\n",i);
                if (JSVAL_IS_STRING(mainElement)) printf ("sc, element %d is an STRING\n",i);
                if (JSVAL_IS_NUMBER(mainElement)) printf ("sc, element %d is an NUMBER\n",i);
                if (JSVAL_IS_DOUBLE(mainElement)) printf ("sc, element %d is an DOUBLE\n",i);
                if (JSVAL_IS_INT(mainElement)) printf ("sc, element %d is an INT\n",i);
*/

                } 
		printf ("CRCcheck %u\n",CRCCheck);
#if defined(JS_THREADSAFE)
		JS_EndRequest(cx);
#endif
	}
#endif



	return touched;
#else
    return FALSE;
#endif /* HAVE_JAVASCRIPT */
}


/* this script value has been looked at, set the touched flag in it to FALSE. */
void resetScriptTouchedFlag(int actualscript, int fptr) {
#ifdef HAVE_JAVASCRIPT
	struct CRscriptStruct *scriptcontrol;
	ttglobal tg = gglobal();
	struct CRjsnameStruct *JSparamnames = getJSparamnames();
	ppJScript p = (ppJScript)tg->JScript.prv;
	#ifdef CRVERBOSE
	printf ("resetScriptTouchedFlag, name %s type %s script %d, fptr %d\n",JSparamnames[fptr].name, stringFieldtypeType(JSparamnames[fptr].type), actualscript, fptr);
	#endif
	scriptcontrol = getScriptControlIndex(actualscript);
	switch (JSparamnames[fptr].type) {
		RESET_TOUCHED_TYPE_A(SFRotation)
		RESET_TOUCHED_TYPE_A(SFNode)
		RESET_TOUCHED_TYPE_A(SFVec2f)
		RESET_TOUCHED_TYPE_A(SFVec3f)
		RESET_TOUCHED_TYPE_A(SFVec4f)
		/* RESET_TOUCHED_TYPE_A(SFVec2d) */
		RESET_TOUCHED_TYPE_A(SFVec3d)
		RESET_TOUCHED_TYPE_A(SFVec4d)
		RESET_TOUCHED_TYPE_A(SFImage)
		RESET_TOUCHED_TYPE_A(SFColor)
		RESET_TOUCHED_TYPE_A(SFColorRGBA)
		RESET_TOUCHED_TYPE_MF_A(MFRotation,SFRotation)
		RESET_TOUCHED_TYPE_MF_A(MFNode,SFNode)
		RESET_TOUCHED_TYPE_MF_A(MFVec2f,SFVec2f)
		RESET_TOUCHED_TYPE_MF_A(MFVec3f,SFVec3f)
		RESET_TOUCHED_TYPE_MF_A(MFVec4f,SFVec4f)
		RESET_TOUCHED_TYPE_MF_A(MFVec4d,SFVec4d)
		/* RESET_TOUCHED_TYPE_MF_A(MFImage,SFImage) */
		RESET_TOUCHED_TYPE_MF_A(MFColor,SFColor)
		RESET_TOUCHED_TYPE_MF_A(MFColorRGBA,SFColorRGBA)

		RESET_TOUCHED_TYPE_ECMA (SFInt32)
		RESET_TOUCHED_TYPE_ECMA (SFBool)
		RESET_TOUCHED_TYPE_ECMA (SFFloat)
		RESET_TOUCHED_TYPE_ECMA (SFTime)
		RESET_TOUCHED_TYPE_ECMA (SFDouble)
		RESET_TOUCHED_TYPE_ECMA (SFString)
		RESET_ECMA_MF_TOUCHED(MFInt32)
		RESET_ECMA_MF_TOUCHED(MFBool) 
		RESET_ECMA_MF_TOUCHED(MFFloat) 
		RESET_ECMA_MF_TOUCHED(MFTime) 
		RESET_ECMA_MF_TOUCHED(MFString) 
		
			
		default: {printf ("can not reset touched_flag for %s\n",stringFieldtypeType(JSparamnames[fptr].type));
		}
	}
#endif /* HAVE_JAVASCRIPT */
}



#endif /* HAVE_JAVASCRIPT */
