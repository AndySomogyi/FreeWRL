/*


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

#ifndef IPHONE

#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>
#include <list.h>
#include <io_files.h>


#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../scenegraph/Vector.h"
#include "../vrml_parser/CFieldDecls.h"
#include "../world_script/JScript.h"
#include "../world_script/CScripts.h"
#include "../world_script/fieldSet.h"
#include "../vrml_parser/CParseParser.h"
#include "../vrml_parser/CParseLexer.h"
#include "../vrml_parser/CProto.h"
#include "../vrml_parser/CParse.h"
#include "../vrml_parser/CRoutes.h"	/* for upper_power_of_two */
#include "../input/InputFunctions.h"	/* resolving implicit declarations */
#include "../input/EAIHeaders.h"	/* resolving implicit declarations */
#include "../input/EAIHelpers.h"	/* resolving implicit declarations */

#include "X3DParser.h"
#include "X3DProtoScript.h"


//int currentProtoDeclare  = INT_ID_UNDEFINED;
//int MAXProtos = 0;
//static int curProDecStackInd = 0;
//int currentProtoInstance[PROTOINSTANCE_MAX_LEVELS];
//static int currentProtoInstance = INT_ID_UNDEFINED;
static int getFieldAccessMethodFromProtoInterface (struct VRMLLexer *myLexer, char *fieldName, int protono);

#define CPI p->ProtoInstanceTable[p->curProtoInsStackInd]
#define CPD p->PROTONames[p->currentProtoDeclare]

/* for parsing script initial fields */
#define MP_NAME 0
#define MP_ACCESSTYPE 1
#define MP_TYPE 2
#define MP_VALUE 3
#define MPFIELDS 4 /* MUST be the highest MP* plus one - array size */

#define UNIQUE_NUMBER_HOLDER "-fReeWrl-UniqueNumH"

/* ProtoInstance table This table is a dynamic table that is used for keeping track of ProtoInstance field values... */
//int curProtoInsStackInd = -1;

struct PROTOInstanceEntry {
	char *name[PROTOINSTANCE_MAX_PARAMS];
	char *value[PROTOINSTANCE_MAX_PARAMS];
	int type[PROTOINSTANCE_MAX_PARAMS]; //0-string 1-itoa(DEF index) 10-(FIELDTYPE_SFNODE) union anyVrml* or X3D_Node* 11-(FIELDTYPE_MFNODE) union anyVrml* or Multi_Node*
	char *defName;
	int container;
	int paircount;
	int uniqueNumber;
};
//static struct PROTOInstanceEntry ProtoInstanceTable[PROTOINSTANCE_MAX_LEVELS];

/* PROTO table */
struct PROTOnameStruct {
	char *definedProtoName;
	char *url;
	FILE *fileDescriptor;
	char *fileName;
	int charLen;
	int fileOpen;
	int isExternProto;
	struct Shader_Script *fieldDefs;
};
//struct PROTOnameStruct *PROTONames = NULL;

/* we want to parse <field><Transform/><Transform/></field> style field values for 
  SFNode and MFNode field types, and be able to do that recursively 
  -with a stack- so you can have 
  <field><ProtoDeclare/><ProtoInstance/></field> or 
  <field><Script/></field> etc. 
*/
typedef struct fieldNodeState
{
	int parsingMFSFNode;// 0 - regular string field value, parsed from string,  1 - <field><Transform></field>
	struct X3D_Node *fieldHolder;// we fool regular parsing by giving it a X3D_Group with children field to populate
	int fieldHolderInitialized;// 0 no fieldHolder, 1 - after mallocing a X3D_Group for fieldHolder
	struct ScriptFieldDecl* mfnodeSdecl;// a direct pointer to the script field 
	int myObj_num;// script number
	struct Shader_Script *myObj;
}fieldNodeState;
//static struct fieldNodeState fieldNodeParsingStateA[PROTOINSTANCE_MAX_LEVELS]; 
//static struct fieldNodeState fieldNodeParsingStateB[PARENTSTACKSIZE];


typedef struct pX3DProtoScript{
	int currentProtoDeclare;//  = INT_ID_UNDEFINED;
	int MAXProtos;// = 0;
	int curProDecStackInd;// = 0;
	int currentProtoInstance[PROTOINSTANCE_MAX_LEVELS];
	int curProtoInsStackInd;// = -1;
	struct PROTOInstanceEntry ProtoInstanceTable[PROTOINSTANCE_MAX_LEVELS];
	struct PROTOnameStruct *PROTONames;// = NULL;
	struct fieldNodeState fieldNodeParsingStateA[PROTOINSTANCE_MAX_LEVELS]; 
	struct fieldNodeState fieldNodeParsingStateB[PARENTSTACKSIZE];
}* ppX3DProtoScript;
void *X3DProtoScript_constructor(){
	void *v = malloc(sizeof(struct pX3DProtoScript));
	memset(v,0,sizeof(struct pX3DProtoScript));
	return v;
}
void X3DProtoScript_init(struct tX3DProtoScript *t){
	//public
	//private
	t->prv = X3DProtoScript_constructor();
	{
		ppX3DProtoScript p = (ppX3DProtoScript)t->prv;
		p->currentProtoDeclare  = INT_ID_UNDEFINED;
		p->MAXProtos = 0;
		p->curProDecStackInd = 0;
		//p->currentProtoInstance[PROTOINSTANCE_MAX_LEVELS];
		p->curProtoInsStackInd = -1;
		//p->ProtoInstanceTable[PROTOINSTANCE_MAX_LEVELS];
		p->PROTONames = NULL;
		//p->fieldNodeParsingStateA[PROTOINSTANCE_MAX_LEVELS]; 
		//p->fieldNodeParsingStateB[PARENTSTACKSIZE];
	}
}
//ppX3DProtoScript p = (ppX3DProtoScript)gglobal()->X3DProtoScript.prv;



#ifdef X3DPARSERVERBOSE
static const char *parserModeStrings[] = {
                "unused",
                "PARSING_NODES",
                "PARSING_SCRIPT",
                "PARSING_PROTODECLARE ",
                "PARSING_PROTOINTERFACE ",
                "PARSING_PROTOBODY",
                "PARSING_PROTOINSTANCE",
                "PARSING_IS",
                "PARSING_CONNECT",
                "PARSING_EXTERNPROTODECLARE",
                "unused high"};
#endif


/****************************** PROTOS ***************************************************/

/* we are closing off a parse of an XML file. Lets go through and free/unlink/cleanup. */
void freeProtoMemory () {
	int i;
	ppX3DProtoScript p = (ppX3DProtoScript)gglobal()->X3DProtoScript.prv;

	
	#ifdef X3DPARSERVERBOSE
	printf ("freeProtoMemory, currentProtoDeclare is %d PROTONames = %d \n",currentProtoDeclare,(int) PROTONames);
	#endif

	if (p->PROTONames != NULL) {
		for (i=0; i<= p->currentProtoDeclare; i++) {
			
			if (p->PROTONames[i].fileName) {
				if (p->PROTONames[i].fileOpen) {
					fclose(p->PROTONames[i].fileDescriptor); /* should never happen... */
				} else {
					WARN_MSG("freeProtoMemory: trying to close openned url in PROTONames. We should use resources.\n");
				}
				UNLINK(p->PROTONames[i].fileName);
			}

			FREE_IF_NZ (p->PROTONames[i].definedProtoName);

			/* either this is not a "freeable" memory, so no free can be called,
			   either this should pass through FREE_IF_NZ / XFREE ...

			   whatever ... we shall handle the case before ...
			   (relevant info is required where this string is allocated) */
			XFREE(p->PROTONames[i].fileName);

		}
		FREE_IF_NZ(p->PROTONames);
	}

	p->currentProtoDeclare  = INT_ID_UNDEFINED;
	p->MAXProtos = 0;
}

/* record each field of each script - the type, kind, name, and associated script */
static void registerProto(const char *name, const char *url) {
	ppX3DProtoScript p = (ppX3DProtoScript)gglobal()->X3DProtoScript.prv;

	#ifdef X3DPARSERVERBOSE
	TTY_SPACE
	printf ("registerProto for currentProtoDeclare %d name %s\n",currentProtoDeclare, name);
	#endif


	/* ok, we got a name and a type */
	if (p->currentProtoDeclare >= p->MAXProtos) {
		/* oooh! not enough room at the table */
		p->MAXProtos += 10; /* arbitrary number */
		p->PROTONames = (struct PROTOnameStruct*)REALLOC (p->PROTONames, sizeof(*p->PROTONames) * p->MAXProtos);
	}

	CPD.definedProtoName = STRDUP((char *)name);

	/* a Proto Expansion (NOT ExternProtoDeclare) will have a local file */
	CPD.isExternProto = (url != NULL);

	if (!CPD.isExternProto) 
		CPD.fileName = TEMPNAM(gglobal()->Mainloop.tmpFileLocation,"freewrl_proto"); 
	else 
#ifdef _MSC_VER
		CPD.fileName = TEMPNAM(gglobal()->Mainloop.tmpFileLocation,"freewrl_proto"); 
#else
		CPD.fileName=NULL;
#endif
	CPD.fileDescriptor = fopen(CPD.fileName,"w");
	CPD.charLen =  0;
	CPD.fileOpen = TRUE;
	CPD.fieldDefs = new_Shader_Script(NULL);
	if (CPD.isExternProto) CPD.url=STRDUP((char *)url); else CPD.url=NULL;

	#ifdef X3DPARSERVERBOSE
	TTY_SPACE
	printf ("opened name %s for PROTO name %s id %d\n",CPD.fileName,
		CPD.definedProtoName,currentProtoDeclare);
	#endif
}



static int getFieldAccessMethodFromProtoInterface (struct VRMLLexer *myLexer, char *fieldName, int protono) {
	const char** userArr;
	size_t userCnt;
	//struct Shader_Script* myObj;
	indexT retUO;
	//ppX3DProtoScript p = (ppX3DProtoScript)gglobal()->X3DProtoScript.prv;

	#ifdef X3DPARSERVERBOSE
	printf ("getFieldAccessMethodFromProtoInterface, lexer %u looking for :%s: in proto %d\n",myLexer, fieldName, protono);
	#endif
	
	//myObj = p->PROTONames[protono].fieldDefs;

        
#define LOOK_FOR_FIELD_IN(whicharr) \
        userArr=&vector_get(const char*, myLexer->user_##whicharr, 0); \
        userCnt=vectorSize(myLexer->user_##whicharr);\
        retUO=findFieldInARR(fieldName, userArr, userCnt);\
        if (retUO != ID_UNDEFINED) { \
		return PKW_##whicharr; \
        } 
                
        LOOK_FOR_FIELD_IN(initializeOnly);
        LOOK_FOR_FIELD_IN(inputOnly);
        LOOK_FOR_FIELD_IN(outputOnly);
        LOOK_FOR_FIELD_IN(inputOutput);
        
#undef LOOK_FOR_FIELD_IN

	/* did not find it. */
	return INT_ID_UNDEFINED;
}

static int getProtoKind(struct VRMLLexer *myLexer, int ProtoInvoc, char *id) {
	ppX3DProtoScript p = (ppX3DProtoScript)gglobal()->X3DProtoScript.prv;

	#ifdef X3DPARSERVERBOSE
	printf ("getProtoKind for proto %d, char :%s:\n",ProtoInvoc, id);
	#endif

	/* get the start/end value pairs, and copy them into the id field. */
	if ((p->curProtoInsStackInd < 0) || (p->curProtoInsStackInd >= PROTOINSTANCE_MAX_LEVELS)) {
		return INT_ID_UNDEFINED;
	}

	return getFieldAccessMethodFromProtoInterface (myLexer, id, ProtoInvoc);
}

//#define TOD gglobal()->X3DParser.parentStack[gglobal()->X3DParser.parentIndex]


/*
int getRoutingInfo (struct VRMLLexer *myLexer, struct X3D_Node *node, int *offs, int* type, int *accessType, struct Shader_Script **myObj, char *name, int routeTo)
*/
/* lets see if this node has a routed field  fromTo  = 0 = from node, anything else = to node */
#define ROUTE_FROM_META_TO_ISD \
	getRoutingInfo (myLexer, getNode(ud,TOP), &nodeOffs, &type, &accessType, &holder, nodeField, 1); \
	if (nodeOffs != INT_ID_UNDEFINED) CRoutes_RegisterSimple(metaNode, metaFromOffs, getNode(ud,TOP), nodeOffs, type); 


#define ROUTE_FROM_ISD_TO_META \
	getRoutingInfo (myLexer, getNode(ud,TOP), &nodeOffs, &type, &accessType, &holder, nodeField, 0); \
	if (nodeOffs != INT_ID_UNDEFINED) CRoutes_RegisterSimple(getNode(ud,TOP), nodeOffs, metaNode, metaToOffs, type); 


static void generateRoute (void *ud, struct VRMLLexer *myLexer, struct ScriptFieldDecl* protoField, char *nodeField) {
	int myFieldKind;
	char newname[1000];
	struct X3D_Node *metaNode;
	int fieldInt;
	int metaToOffs;
	int metaFromOffs;
	int type;
	int accessType;
	int nodeOffs;
	struct Shader_Script *holder; /* not used, only for parameter in getRoutingInfo */
	struct CRjsnameStruct *JSparamnames = getJSparamnames();
	ppX3DProtoScript p = (ppX3DProtoScript)gglobal()->X3DProtoScript.prv;


	#ifdef X3DPARSERVERBOSE
	char *myDefName="(top of stack node)";
	#endif


	//if (TOD==NULL) {
	if(!getNode(ud,TOP)){
		printf ("generateRoute, internal problem, TOD==NULL\n");
		return;
	}

	
	/* we have names and fields, lets generate routes for this */
	myFieldKind = getProtoKind(myLexer,p->currentProtoInstance[p->curProtoInsStackInd],fieldDecl_getShaderScriptName(protoField->fieldDecl));

	/* find the MetaMF/MetaSF node to route to/from to: */
	sprintf(newname,"%s_%s_%d",fieldDecl_getShaderScriptName(protoField->fieldDecl),FREEWRL_SPECIFIC,CPI.uniqueNumber);

	/* printf ("looking for name %s\n",newname); */

	metaNode = DEFNameIndex ((const char *)newname,NULL,FALSE);
	if (metaNode == NULL) {
		printf ("generateRoute: could not find node %s\n",newname);
		return;
	}
	/* printf ("so, for name %s, metaNode is %u\n",newname,metaNode); */

	/* set up these offsets for routing to/from the Meta node */
	fieldInt = findRoutedFieldInFIELDNAMES (metaNode, "valueChanged", FALSE);

	if (fieldInt >=0) findFieldInOFFSETS(metaNode->_nodeType, fieldInt, &metaFromOffs, &type, &accessType);

	fieldInt = findRoutedFieldInFIELDNAMES (metaNode, "setValue", TRUE);
	if (fieldInt >=0) findFieldInOFFSETS(metaNode->_nodeType, fieldInt, &metaToOffs, &type, &accessType);

	/* printf (" should generate a route or two here to route from the internal Meta node to the ISd node, kind %d\n",myFieldKind); */

	switch (myFieldKind) {
		case PKW_inputOnly: 
			/* 1 */ DEBUG_X3DPARSER ("inputOnly: <ROUTE fromNode='%s_%s_%d' fromField='valueChanged' toNode='%s' toField='%s'/>\n",
					fieldDecl_getShaderScriptName(protoField->fieldDecl),FREEWRL_SPECIFIC,CPI.uniqueNumber,newname,nodeField);
					//fieldDecl_getShaderScriptName(protoField->fieldDecl),FREEWRL_SPECIFIC,CPI.uniqueNumber,myDefName,nodeField);
				ROUTE_FROM_META_TO_ISD

			break;
		case PKW_outputOnly: 
			/* 2 */DEBUG_X3DPARSER ("outputOnly: <ROUTE toNode='%s_%s_%d' toField='setValue' fromNode='%s' fromField='%s'/>\n",
					fieldDecl_getShaderScriptName(protoField->fieldDecl),FREEWRL_SPECIFIC,CPI.uniqueNumber,newname,nodeField);
					//fieldDecl_getShaderScriptName(protoField->fieldDecl),FREEWRL_SPECIFIC,CPI.uniqueNumber,myDefName,nodeField);
				ROUTE_FROM_ISD_TO_META
			break;
		case PKW_inputOutput: 
			/* 1 */ DEBUG_X3DPARSER ("inputOutput: <ROUTE fromNode='%s_%s_%d' fromField='valueChanged' toNode='%s' toField='%s'/>\n",
					fieldDecl_getShaderScriptName(protoField->fieldDecl),FREEWRL_SPECIFIC,CPI.uniqueNumber,newname,nodeField);
					//fieldDecl_getShaderScriptName(protoField->fieldDecl),FREEWRL_SPECIFIC,CPI.uniqueNumber,myDefName,nodeField);
				ROUTE_FROM_META_TO_ISD
			/* 2 */DEBUG_X3DPARSER ("inputOutput: <ROUTE toNode='%s_%s_%d' toField='setValue' fromNode='%s' fromField='%s'/>\n",
					fieldDecl_getShaderScriptName(protoField->fieldDecl),FREEWRL_SPECIFIC,CPI.uniqueNumber,newname,nodeField);
					//fieldDecl_getShaderScriptName(protoField->fieldDecl),FREEWRL_SPECIFIC,CPI.uniqueNumber,myDefName,nodeField);
				ROUTE_FROM_ISD_TO_META
			break;
		case PKW_initializeOnly: 
			/* printf ("PKW_initializeOnly - do nothing\n"); */
			break;
		default :
			printf ("generateRoute unknown proto type - ignoring\n");
	}
}

#define REPLACE_CONNECT_VALUE(value,type) \
			/* now, we have a match, replace (or push) this onto the params */ \
				for (nodeind=0; nodeind<vectorSize(tos); nodeind++) \
			{ \
				nvp = vector_get(struct nameValuePairs*, tos,nodeind); \
				/* printf ("      nvp %d, fieldName:%s fieldValue:%s\n",ind,nvp->fieldName,nvp->fieldValue);*/  \
				if (nvp!=NULL) \
				{ \
					if (strcmp(nvp->fieldName,atts[nfInd+1])==0) \
					{ \
						/* we have a field already of this name, replace the value */ \
						/*printf ("we had %s, now have %s\n",nvp->fieldValue,value); */ \
						FREE_IF_NZ(nvp->fieldValue); \
						nvp->fieldValue=STRDUP(value);\
						nvp->fieldType = type;\
						return; \
					} \
				} \
			} \
			/* printf ("no match, have to create new entty and push it\n"); */\
			nvp = MALLOC(struct nameValuePairs *, sizeof (struct nameValuePairs)); \
			nvp->fieldName=STRDUP(atts[nfInd+1]); \
			nvp->fieldValue=STRDUP(value); \
			nvp->fieldType = type;\
			vector_pushBack(struct nameValuePairs*,tos,nvp); \
			/* printf ("pushed %s=%s to tos\n",nvp->fieldName, nvp->fieldValue);*/ \
			return;

/*

parseConnect - we may have had a proto defined like:
<ProtoDeclare name='Sphere'>
 <ProtoInterface>
        <field accessType='inputOutput' name='SphereColour' type='SFColor' value='0.8 0.8 0.8' />
 </ProtoInterface>

 <ProtoBody>
        <Shape>
         <Appearance>
                <Material>
                <IS>
                <connect nodeField='diffuseColor' protoField='SphereColour'/>
                </IS>
                </Material>
         </Appearance>
         <Sphere/>
      </Shape>
 </ProtoBody>
</ProtoDeclare>

and, an invocation like:

        <ProtoInstance containerField='children' name='Sphere'>
        <fieldValue name='SphereColour' value='1 1 0'/>
        </ProtoInstance>o

will yield:

	<?xml version="1.0" encoding="utf-8"?>
	<X3D><Scene><Group FreeWRL__protoDef='0'> <!-- INITIATE SCENE -->
	<!--
	ProtoInterface fields has 1 fields -->
		<MetadataSFColor DEF='SphereColour_FrEEWrL_pRotto_0' value='0.8 0.8 0.8'/>
	<!-- end of MAKE_PROTO_COPY_FIELDS --> 
	<Shape>
	<Appearance>
	<Material>
		<IS>
			<connect nodeField="diffuseColor" protoField="SphereColour">
			</connect>
		</IS>
	</Material>
	</Appearance>
	<Sphere>
	</Sphere>
	</Shape>
</Group></Scene></X3D>



*/
BOOL nodeTypeSupportsUserFields(struct X3D_Node *node);
int getFieldFromNodeAndName(struct X3D_Node* node,const char *fieldname, int *type, int *kind, int *iifield, union anyVrml **value);
void broto_store_IS(struct X3D_Proto *proto,char *protofieldname,int pmode, int iprotofield, int type,
					struct X3D_Node *node, char* nodefieldname, int mode, int ifield, int source);
void parseConnect_B(void *ud, char **atts) {
	int i,okp, okn;
	struct X3D_Node *node;
	struct X3D_Proto *context, *proto;
	char *nodefield, *protofield;
	node = getNode(ud,TOP);
	proto = context = getContext(ud,TOP);

	nodefield = protofield = NULL;
	for(i=0;atts[i];i+=2){
		if(!strcmp(atts[i],"nodeField")) nodefield = atts[i+1];
		if(!strcmp(atts[i],"protoField")) protofield = atts[i+1];
	}
	okp = okn = 0;
	if(nodefield && protofield){
		int ptype, pkind, pifield, ntype, nkind, nifield;
		union anyVrml *pvalue, *nvalue;
		okp = getFieldFromNodeAndName(X3D_NODE(proto),protofield,&ptype, &pkind, &pifield, &pvalue);
		okn = getFieldFromNodeAndName(node, nodefield,&ntype, &nkind, &nifield, &nvalue);
		//check its mode
		// http://www.web3d.org/files/specifications/19775-1/V3.2/Part01/concepts.html#t-RulesmappingPROTOTYPEdecl
		// there's what I call a mode-jive table
		//							proto interface
		//							inputOutput	initializeOnly	inputOnly	outputOnly
		//	node	inputOutput		jives		jives			jives		jives
		//			initializeOnly				jives
		//			inputOnly									jives
		//			outputOnly												jives
		//
		// so if our nodefield's mode is inputOutput/exposedField then we are covered for all protoField modes
		// otherwise, the nodefield's mode must be the same as the protofield's mode
		if(okp && okn)
		if(ntype != ptype){
			ConsoleMessage("Parser error: IS - we have a name match: %s IS %s found protofield %s\n",
				nodefield,protofield,protofield);
			ConsoleMessage("...But the types don't match: nodefield %s protofield %s\n",
				FIELDTYPES[ntype],FIELDTYPES[ptype]);
			okp = 0;
		}
		if(okp && okn)
		if(nkind != PKW_inputOutput && nkind != pkind){
			ConsoleMessage("Parser Error: IS - we have a name match: %s IS %s found protofield %s\n",
				nodefield,protofield,protofield);
			ConsoleMessage("...But the modes don't jive: nodefield %s protofield %s\n",
				PROTOKEYWORDS[nkind],PROTOKEYWORDS[pkind]);
			okp = 0;
		}
		if(okp && okn){
			int source;
			//we have an IS that's compatible/jives
			//a) copy the value if it's an initializeOnly or inputOutput
			if(pkind == PKW_initializeOnly || pkind == PKW_inputOutput)
			{
				shallow_copy_field(ntype, pvalue , nvalue);
			}
			//b) register it in the IS-table for our context
			source = node->_nodeType == NODE_Proto ? 3 : node->_nodeType == NODE_Script ? 1 : nodeTypeSupportsUserFields(node) ? 2 : 0;
			//Q. do I need to convert builtin from field index to offset? if( source == 0) nifield *=5;
			broto_store_IS(context,protofield,pkind,pifield,ptype,
							node,nodefield,nkind,nifield,source);
		}
	}
}
void parseConnect(void *ud, struct VRMLLexer *myLexer, char **atts, struct Vector *tos) {
	int i;
	struct nameValuePairs *nvp;
	int ind;
	int nodeind;
	int nfInd;
	int pfInd;
	struct Shader_Script* myObj;
	int matched;
	struct CRjsnameStruct *JSparamnames = getJSparamnames();
	ppX3DProtoScript p = (ppX3DProtoScript)gglobal()->X3DProtoScript.prv;

	nfInd=INT_ID_UNDEFINED;
	pfInd=INT_ID_UNDEFINED;

	#ifdef X3DPARSERVERBOSE
	printf ("\nparseConnect mode is %s\n",parserModeStrings[getMode(ud,TOP)]); 
	#endif

	if (getMode(ud,TOP) != PARSING_IS) {
		ConsoleMessage ("parseConnect: got a <connect> but not in a Proto Expansion at line %d",LINE);
	}

	/* find the nodeField and protoField tags */
        for (i = 0; atts[i]; i += 2) {
                /* printf("parseConnect:%s=%s\n", atts[i], atts[i + 1]); */
		if (strcmp("nodeField",atts[i])==0) nfInd=i;
		if (strcmp("protoField",atts[i])==0) pfInd=i;
	}

	if (nfInd==INT_ID_UNDEFINED) {
		ConsoleMessage ("have a <connect> without a :nodeField: parameter");
		return;
	}
	if (pfInd==INT_ID_UNDEFINED) {
		ConsoleMessage ("have a <connect> without a :protoField: parameter");
		return;
	}

	/* worry about generating routes to/from this field */
	/*printf ("parseConnect, currentProto is %d\n",currentProtoInstance[curProtoInsStackInd]); */
	/* printf ("parseConnect, currentProto is %d\n",curProtoInsStackInd); //curProtoInsStackInd */
	/* printf ("	and, we have %s and %s\n",atts[pfInd+1],atts[nfInd+1]);  */
	matched = FALSE;
	myObj = p->PROTONames[p->currentProtoInstance[p->curProtoInsStackInd]].fieldDefs; 
	//myObj = PROTONames[curProtoInsStackInd].fieldDefs; 
	for (ind=0; ind<vectorSize(myObj->fields); ind++) { 
		struct ScriptFieldDecl* field; 
		field = vector_get(struct ScriptFieldDecl*, myObj->fields, ind); 
		/*  printf ("ind %d name %s value %s\n", ind, fieldDecl_getShaderScriptName(field->fieldDecl),  field->ASCIIvalue); */

		if (strcmp(fieldDecl_getShaderScriptName(field->fieldDecl),atts[pfInd+1])==0) {
			/* printf ("parseConnect, routing, have match, value is %s\n",atts[nfInd+1]); */
			matched=TRUE;
			generateRoute(ud, myLexer, field, (char *) atts[nfInd+1]);
		}
	} 

	/* if we did not get a match, we have an error... */
	if (!matched) {
		ConsoleMessage ("<connect> problem, no field %s in ProtoDeclare\n",atts[pfInd+1]);
		return;
	}



	/* ok, we have a good connection here. Lets go through and find the protoField 
	   in the protoInvocation. */

	/* go through the current ProtoInstance, and see if we can do a match */
	/* printf ("parseConnect, curProtoInsStackInd %d\n",curProtoInsStackInd);  */
	for (i=0; i<CPI.paircount; i++) 
	{
		/* dug9 July 18, 2010 added nvp->fieldType and CPI.type to those two structs 
		   to help handle SFNode and MFNode fields
		   Here, just copy over nvp.fieldType = CPI.type. 
		   type/fieldType: 
			0-normal string .value (not MFNode or SFNode)
			1-value=atoi(DEF index of SFNode) reserved not implemented - some related code in EAI
			11-(FIELDTYPE_MFNode) sprintf(value,%p,pointer to malloced anyVrml holding MFNode) for cases like:
			   <fieldValue name="Buildings">
				<Transform USE="House1"/>
				<Transform USE="House2"/>
			   </fieldValue>
		   and we use 11 for SFNodes coming from Instance fieldValue, since we don't know they type from the fieldValue.
		   we use FIELDTYPE_MFNode elsewhere to be general, and here we pass it along, somewhere else, 
		   it figures out from the Script ISd field type that it should be SF or MF.
		*/
		/* printf ("	index %d is name :%s: value :%s: type :%d:\n",i,CPI.name[i],CPI.value[i],CPI.type[i]);  
		 printf ("... comparing %s and %s\n",CPI.name[i],atts[pfInd+1]); */
		if (CPI.name[i] && strcmp(CPI.name[i],atts[pfInd+1])==0) 
		{
			/* printf ("parseConnect, have match, value is %s\n",CPI.value[i]); */
			/* if there is no value here, just return, as some accessMethods do not have value */
			if (CPI.value[i]==NULL) 
				return;
			///BIGPUSH BIGPOP
			REPLACE_CONNECT_VALUE(CPI.value[i],CPI.type[i])
			/*printf ("parseConnect, match completed\n"); */
		}
	}

	/* did not find it in the ProtoInstance fields, lets look in the ProtoDeclare */
	/* printf ("parseConnect, did not find %s in the ProtoInstance, looking for it in ProtoDeclare\n",atts[pfInd+1]); */
	
	myObj = p->PROTONames[p->currentProtoInstance[p->curProtoInsStackInd]].fieldDefs; 
	//myObj = PROTONames[curProtoInsStackInd].fieldDefs; 
	for (ind=0; ind<vectorSize(myObj->fields); ind++) { 
		struct ScriptFieldDecl* field; 
		field = vector_get(struct ScriptFieldDecl*, myObj->fields, ind); 
		/* printf ("ind %d name %s value %s\n", ind, fieldDecl_getShaderScriptName(field->fieldDecl),  field->ASCIIvalue);  */

		if (strcmp(fieldDecl_getShaderScriptName(field->fieldDecl),atts[pfInd+1])==0) {
			/* printf ("parseConnect, have match, value is %s\n",field->ASCIIvalue); */
			/* if there is no value here, just return, as some accessMethods do not have value */
			if (field->ASCIIvalue==NULL)
			{
				if((field->fieldDecl->fieldType == FIELDTYPE_MFNode || field->fieldDecl->fieldType == FIELDTYPE_SFNode) 
					&& field->valueSet)
				{
					///BIGPUSH BIGPOP
					/* assume we're getting an mfnode */
					union anyVrml *av;
					char *val = MALLOC(char *, 20);
					av = MALLOC(union anyVrml *, sizeof(union anyVrml));
					memcpy(av,&field->value,sizeof(union anyVrml)); /* av malloced and holding MFNode/Multi_Node/anyVrml */
					sprintf (val, "%p",av);
					REPLACE_CONNECT_VALUE(val,field->fieldDecl->fieldType);
				}
				//else if(field->fieldDecl->fieldType == FIELDTYPE_SFNode && field->valueSet)
				//{
				//	//reserved for SFNode handling
				//}
				else
					return;
			}else{
				REPLACE_CONNECT_VALUE(field->ASCIIvalue,0)
			}
			/* printf ("parseConnect, match completed\n"); */
		}
	} 
}

void endConnect(void *ud) {
	#ifdef X3DPARSERVERBOSE
	printf ("endConnect mode is %s\n",parserModeStrings[getMode(ud,TOP)]);
	#endif
	popMode(ud);
}


void parseProtoInstanceFields(void *ud, const char *name, char **atts) {
	int count;
	ppX3DProtoScript p = (ppX3DProtoScript)gglobal()->X3DProtoScript.prv;

	#define INDEX p->ProtoInstanceTable[p->curProtoInsStackInd].paircount	
	#define ZERO_NAME_VALUE_PAIR \
		p->ProtoInstanceTable[p->curProtoInsStackInd].name[INDEX] = NULL; \
		p->ProtoInstanceTable[p->curProtoInsStackInd].value[INDEX] = NULL;\
		p->ProtoInstanceTable[p->curProtoInsStackInd].type[INDEX] = 0;\


	#ifdef X3DPARSERVERBOSE
	printf ("parsing PRotoInstanceFields for %s at level %d\n",name,curProtoInsStackInd);
	#endif

	/* this should be a <fieldValue...> tag here */
	/* eg: <fieldValue name='imageName' value="helpers/brick.png"/> */
	if (strcmp(name,"fieldValue") == 0) {
		ZERO_NAME_VALUE_PAIR
		for (count = 0; atts[count]; count += 2) {
			#ifdef X3DPARSERVERBOSE
			printf ("ProtoInstanceFields: %s=\"%s\"\n",atts[count], atts[count+1]);
			#endif

			/* add this to our instance tables */
			/* is this the name field? */
			if (strcmp("name",atts[count])==0) 
				p->ProtoInstanceTable[p->curProtoInsStackInd].name[INDEX] = STRDUP(atts[count+1]);
			if (strcmp("value",atts[count])==0) 
				p->ProtoInstanceTable[p->curProtoInsStackInd].value[INDEX] = STRDUP(atts[count+1]);


			if (INDEX>=PROTOINSTANCE_MAX_PARAMS) {
				ConsoleMessage ("too many parameters for ProtoInstance, sorry...\n");
				INDEX=0;
			}
		}
		/* did we get both a name and a value? */
		if ((p->ProtoInstanceTable[p->curProtoInsStackInd].name[INDEX] != NULL) &&
		    (p->ProtoInstanceTable[p->curProtoInsStackInd].value[INDEX] != NULL)) {
			INDEX++;
			ZERO_NAME_VALUE_PAIR
			p->fieldNodeParsingStateA[p->curProtoInsStackInd].parsingMFSFNode = 0;
		}
		else if( (p->ProtoInstanceTable[p->curProtoInsStackInd].value[INDEX] == NULL) && (p->ProtoInstanceTable[p->curProtoInsStackInd].name[INDEX] != NULL))
		{
			/* name but no value - could be like this:
			  <fieldValue name='Buildings'>
				<Transform USE='House1'/>    <<< trick parser into putting these in a Group node's children field, 
				<Transform USE='House2'/>    <<< we'll fetch on end element </fieldValue>
			  </fieldValue>
			  or like this:
              <fieldValue name='relay'> <Script USE='CameraRelay'/> </fieldValue>
			*/
			//ttglobal tg = gglobal();
			////BIGPUSH  added by dug9 July 18,2010
			p->fieldNodeParsingStateA[p->curProtoInsStackInd].parsingMFSFNode = 1;
			if(!p->fieldNodeParsingStateA[p->curProtoInsStackInd].fieldHolderInitialized)
				p->fieldNodeParsingStateA[p->curProtoInsStackInd].fieldHolder = (struct X3D_Node*)createNewX3DNode(NODE_Group);
			pushMode(ud,PARSING_NODES);
			//INCREMENT_PARENTINDEX //parentIndex++;
			//tg->X3DParser.parentStack[gglobal()->X3DParser.parentIndex] = p->fieldNodeParsingStateA[p->curProtoInsStackInd].fieldHolder;
			pushNode(ud,p->fieldNodeParsingStateA[p->curProtoInsStackInd].fieldHolder);
			//if (getChildAttributes(gglobal()->X3DParser.parentIndex)!=NULL) deleteChildAttributes(gglobal()->X3DParser.parentIndex); 
			//setChildAttributes(gglobal()->X3DParser.parentIndex,NULL);
			//setChild(ud,TOP,NULL);
		}
		else  /* no name or no name and value */
		{
			ZERO_NAME_VALUE_PAIR
		}
	} else if (strcmp(name,"ProtoInstance") != 0) {
		/*  dug9 July 18, 2010 - should not come in here now for SFNode/MFNode fieldValues, which are handled above in big push */
		ZERO_NAME_VALUE_PAIR
	}
	#undef INDEX
}
void endProtoInstanceFieldTypeNode(void *ud, const char *name)
{
	ppX3DProtoScript p = (ppX3DProtoScript)gglobal()->X3DProtoScript.prv;
	///BIGPOP
	if(p->fieldNodeParsingStateA[p->curProtoInsStackInd].parsingMFSFNode == 1)
	{
		#define INDEX p->ProtoInstanceTable[p->curProtoInsStackInd].paircount	

		//DECREMENT_PARENTINDEX; //parentIndex--;
		popNode(ud);
		if(X3D_GROUP(p->fieldNodeParsingStateA[p->curProtoInsStackInd].fieldHolder)->children.n > 0)
		{ 
			union anyVrml v;
			int n; 
			int j;
			int myValueType;
			struct Multi_Node *kids;
			kids = &X3D_GROUP(p->fieldNodeParsingStateA[p->curProtoInsStackInd].fieldHolder)->children; 
			n = kids->n;
			/*we don't know from the ProtoInstance fieldValue if it's supposed to be SFNode, MFNode or parsing error 
			  so we'll be most general here -assume MFNode- and when we get to connecting to the
			  expanded proto fields, we can get the proper field type then, and down-convert to SFNode 
			  or -if it's neither SFNode nor MFNode- error out then
			*/
			myValueType = FIELDTYPE_MFNode; 
			if(myValueType ==  FIELDTYPE_MFNode)
			{
				/*struct Multi_Node { int n; void * *p; };*/
				((struct Multi_Node *)&v)->n=n;
				((struct Multi_Node *)&v)->p=MALLOC(struct X3D_Node **, upper_power_of_two(n)*sizeof(struct X3D_Node*));
				for(j=0;j<n;j++)
				{
					((struct Multi_Node *)&v)->p[j] = kids->p[j];
				}
			}
			/*else if(myValueType ==  FIELDTYPE_SFNode)
			{
				memcpy(&v,kids->p[0],sizeof(struct X3D_Node*)); 

			}*/
			/*
			OK we now have an MFNode v. Where to store it? 
			we'll malloc a spot for it, convert its pointer/address to a string,
			leave it floating in space with only the string address being passed around 
			*/
			{
				union anyVrml *av;
				char *val = MALLOC(char *, 20);
				av = MALLOC(union anyVrml *, sizeof(union anyVrml));
				memcpy(av,&v,sizeof(union anyVrml)); /* av malloced and holding MFNode/Multi_Node/anyVrml */
				sprintf (val, "%p",av);
				p->ProtoInstanceTable[p->curProtoInsStackInd].value[INDEX] = val; /* address of MFnode stored in a string[20] ie "025A23C8" */
				p->ProtoInstanceTable[p->curProtoInsStackInd].type[INDEX] = FIELDTYPE_MFNode; 
			}
			/*{
			//test code for address recovery
			//should get n nodes back
			union anyVrml *av2;
			char *val2 = ProtoInstanceTable[curProtoInsStackInd].value[INDEX]; //address stored in a string[20] ie "025A23C8"
			sscanf(val2,"%p",&av2);
			printf("got %d mf fields back from pointer %s type %d\n",((struct Multi_Node *)av2)->n,val2,ProtoInstanceTable[curProtoInsStackInd].type[INDEX]);
			}*/
			INDEX++;
			ZERO_NAME_VALUE_PAIR

			/* clean up holder for next time */
			X3D_GROUP(p->fieldNodeParsingStateA[p->curProtoInsStackInd].fieldHolder)->children.n = 0;
		}
		else
		{
			ZERO_NAME_VALUE_PAIR
		}
		p->fieldNodeParsingStateA[p->curProtoInsStackInd].parsingMFSFNode = 0;
		popMode(ud);
	}
}

/***********************************************************************************/


void dumpProtoBody (const char *name, char **atts) {
	int count;
	int inRoute;
	ppX3DProtoScript p = (ppX3DProtoScript)gglobal()->X3DProtoScript.prv;

	/* initialization */
	inRoute = FALSE;

	#ifdef X3DPARSERVERBOSE
	TTY_SPACE
	printf ("dumping ProtoBody for %s\n",name);
	#endif

	if (CPD.fileOpen) {
		inRoute = strcmp(name,"ROUTE") == 0;

		CPD.charLen += fprintf (CPD.fileDescriptor, "<%s",name);

		/* if we are in a ROUTE statement, encode the fromNode and toNode names */
		if (inRoute) {
		    for (count = 0; atts[count]; count += 2) {
			/* printf ("dumpProtoBody - do we need to worry about quotes in :%s: \n",atts[count+1]); */
			if ((strcmp("fromNode",atts[count])==0) || (strcmp("toNode",atts[count])==0)) {
				CPD.charLen +=
				    fprintf (CPD.fileDescriptor," %s='%s_%s_%s'\n",
					atts[count],
					atts[count+1],
					FREEWRL_SPECIFIC,
					UNIQUE_NUMBER_HOLDER);

			} else {
			    CPD.charLen += 
				fprintf (CPD.fileDescriptor," %s=\"%s\"\n",atts[count],atts[count+1]);
			}
		    }

		/* this is a non-route statement, but look for DEF and USE names */
		} else {
		    for (count = 0; atts[count]; count += 2) {
			/* printf ("dumpProtoBody - do we need to worry about quotes in :%s: \n",atts[count+1]); */
			if ((strcmp("DEF",atts[count])==0) || (strcmp("USE",atts[count])==0)) {
				CPD.charLen +=
				    fprintf (CPD.fileDescriptor," %s='%s_%s_%s'\n",
					atts[count],
					atts[count+1],
					FREEWRL_SPECIFIC,
					UNIQUE_NUMBER_HOLDER);

			} else {
			  if (atts[count+1][0] == '"') {
			    /* put single quotes around this one */
			    CPD.charLen += 
				fprintf (CPD.fileDescriptor," %s='%s'\n",atts[count],atts[count+1]);
			  } else {
			    CPD.charLen += 
				fprintf (CPD.fileDescriptor," %s=\"%s\"\n",atts[count],atts[count+1]);
			  }
			}
		    }
		}
		CPD.charLen += fprintf (CPD.fileDescriptor,">\n");
	}
}

void dumpCDATAtoProtoBody (char *str) {
	ppX3DProtoScript p = (ppX3DProtoScript)gglobal()->X3DProtoScript.prv;

	if (CPD.fileOpen) {
		CPD.charLen += 
			fprintf (CPD.fileDescriptor,"<![CDATA[%s]]>",str);
	}
}

void endDumpProtoBody (const char *name) {
	/* we are at the end of the ProtoBody, close our tempory file */
	ppX3DProtoScript p = (ppX3DProtoScript)gglobal()->X3DProtoScript.prv;

	if (CPD.fileOpen) {
		fclose (CPD.fileDescriptor);
		CPD.fileOpen = FALSE;

		#ifdef X3DPARSERVERBOSE
			TTY_SPACE
			printf ("endDumpProtoBody, just closed %s, it is %d characters long\n",
				CPD.fileName, CPD.charLen);
		#endif
	}
}

/* we have an ExternProtoDeclare and the represented ProtoDeclare. Make sure they match! */
static void verifyExternAndProtoFields(void) {
	int extPro;
	int curPro;
	int curInd, extInd;
	struct Shader_Script *extProF;
	struct Shader_Script *curProF;
	struct ScriptFieldDecl* curField;
	struct ScriptFieldDecl* extField;
	struct CRjsnameStruct *JSparamnames = getJSparamnames();
	ppX3DProtoScript p = (ppX3DProtoScript)gglobal()->X3DProtoScript.prv;

	/* we will have in the stack, ExternProtoDeclare then the CurrentProto */
	if (p->currentProtoDeclare <1) {
		return;
	}

	/* do comparison of fields */
	extPro = p->currentProtoDeclare-1;
	curPro = p->currentProtoDeclare;

	/* printf ("we compare names %s and %s\n",PROTONames[extPro].definedProtoName,PROTONames[curPro].definedProtoName); */
	if (strcmp(p->PROTONames[extPro].definedProtoName,p->PROTONames[curPro].definedProtoName) != 0) {
		ConsoleMessage ("<ExternProtoDeclare> :%s: does not match found <ProtoDeclare> %s",
			p->PROTONames[extPro].definedProtoName,p->PROTONames[curPro].definedProtoName);
	}

	/* go through the fields - these should match */
	extProF = p->PROTONames[extPro].fieldDefs;
	curProF = p->PROTONames[curPro].fieldDefs;

        for (curInd=0; curInd<vectorSize(curProF->fields); curInd++) { 
		int matched=FALSE;
                curField = vector_get(struct ScriptFieldDecl*, curProF->fields, curInd); 
        	for (extInd=0; extInd<vectorSize(extProF->fields); extInd++) { 
                	extField = vector_get(struct ScriptFieldDecl*, extProF->fields, extInd); 
			if ((fieldDecl_getAccessType(curField->fieldDecl) == 
				fieldDecl_getAccessType(extField->fieldDecl))  &&
			(fieldDecl_getType(curField->fieldDecl) == fieldDecl_getType(extField->fieldDecl)) &&
			(fieldDecl_getIndexName(curField->fieldDecl) == fieldDecl_getIndexName(extField->fieldDecl))) {
				matched=TRUE;
				
				/* now, we flagged that this ExternProtoDeclare field is "ok", we signal this by
				   REMOVING its ASCII name and field as it will not be used again */
				FREE_IF_NZ(extField->ASCIIvalue);
			}
		}
		if (!matched) {
			ConsoleMessage ("<ExternProtoDeclare> and embedded <ProtoDeclare> field mismatch, could not match up <ProtoDeclare> field :%s: in <ExternProtoDeclare>",fieldDecl_getShaderScriptName(curField->fieldDecl));
		}
	}

	/* do we have any ExternProtoDeclare fields that are NOT matched by the ProtoDeclare? */
       	for (extInd=0; extInd<vectorSize(extProF->fields); extInd++) { 
               	extField = vector_get(struct ScriptFieldDecl*, extProF->fields, extInd); 
		if (fieldDecl_getShaderScriptName(extField->fieldDecl) != NULL) {
			ConsoleMessage ("<ExternProtoDeclare> field :%s: not matched in embedded Proto",
				fieldDecl_getShaderScriptName(extField->fieldDecl));
			FREE_IF_NZ(extField->ASCIIvalue);
		}
	}
}


/* take the text of an ExternProtoDeclare, and make it into a ProtoDeclare */
void compareExternProtoDeclareWithProto(char *buffer,char *pound) {
	char *startProtoDeclare;
	char *endProtoDeclare;
	char *startScene;
	char *endScene;
	struct X3D_Group *myGroup;
	ppX3DProtoScript p = (ppX3DProtoScript)gglobal()->X3DProtoScript.prv;

	if (buffer==NULL) return;

	/* printf ("compareExternProtoDeclareWithProto starting\n");
	  printf ("	name %s\n	fileName %s\n",CPD.definedProtoName, CPD.fileName);
 	  printf ("	buffer %s\n",buffer);
	*/

	startScene = strstr(buffer,"<Scene>");
	if (startScene == NULL) {
		ConsoleMessage ("ExternProtoDeclare: did not find <Scene> in retrieved file %s",CPD.definedProtoName);
		return;
	}
	startScene += strlen ("<Scene>");


	startProtoDeclare = strstr(startScene,"<ProtoDeclare");
	if (startProtoDeclare == NULL) {
		ConsoleMessage ("ExternProtoDeclare: did not find <ProtoDeclare> in retrieved file");
		return;
	}
	/* just for the fun of it, do we have a null ProtoDeclare? */
	if (strncmp(startProtoDeclare,"</ProtoDeclare>",15) == 0) {
		ConsoleMessage ("ExternProtoDeclare: <ProtoDeclare/> in retrieved file - Proto is empty");
		return;
	}

	endProtoDeclare = strstr(startProtoDeclare,"</ProtoDeclare>");
	if (endProtoDeclare == NULL) {
		ConsoleMessage ("ExternProtoDeclare: did not find </ProtoDeclare> in retrieved file");
		return;
	}
	endProtoDeclare += strlen ("</EndProtoDeclare>");

	endScene = strstr(endProtoDeclare,"</Scene>");
	if (endScene == NULL) {
		ConsoleMessage ("ExternProtoDeclare: did not find </Scene> in retrieved file %s",CPD.definedProtoName);

		return;
	}

	/* get rid of everything between the <Scene> and the <ProtoDeclare> */
	while (startScene != startProtoDeclare) {
		*startScene = ' '; startScene++;
	}

	/* get rid of everything between the </ProtoDeclare> and the </Scene> */
	while (endProtoDeclare != endScene) {
		*endProtoDeclare = ' '; endProtoDeclare++; 
	}
		
	/* printf ("ok, so we have the Protodeclare as\n %s\n",buffer); */

	/* parse this text now */
	myGroup = createNewX3DNode (NODE_Group);
	if (X3DParse(X3D_NODE(myGroup),X3D_NODE(myGroup),buffer)) {
		/* printf ("ExternProto parsed OK\n"); */
	} else {
		ConsoleMessage ("ExternProto retrieval of :%s: did not parse correctly",CPD.definedProtoName);
	}

	/* do the fields match up? */
	verifyExternAndProtoFields();
}
void broto_store_DEF(struct X3D_Proto* proto,struct X3D_Node* node, char *name);
struct X3D_Proto *brotoInstance(struct X3D_Proto* proto, BOOL ideep);
void add_node_to_broto_context(struct X3D_Proto *context,struct X3D_Node *node);
void linkNodeIn_B(void *ud);
void parseProtoInstance_B(void *ud, char **atts) {
	/*broto version
		1. lookup the user (proto) type in current and parent context protoDeclare and externProtoDeclare tables
		2. brotoInstance()
		3. parse att and any <fieldValue> and IS
		4. on end, deep_copy_broto_body2 applying the initial field values parsed.
	*/
	int i;
	int nameIndex;
	//int containerIndex;
	//int containerField;
	int defNameIndex;
	//int protoTableIndex;
	char *protoname;
	struct X3D_Proto *currentContext;
	struct X3D_Node *node = NULL;
	char pflagdepth;

	/* initialization */
	nameIndex = INT_ID_UNDEFINED;
	//containerIndex = INT_ID_UNDEFINED;
	//containerField = INT_ID_UNDEFINED;
	defNameIndex = INT_ID_UNDEFINED;
	//protoTableIndex = 0;
	if(0) printf("parseProtoInstance\n");

	for (i = 0; atts[i]; i += 2) {
		if (strcmp("name",atts[i]) == 0) {
			nameIndex=i+1;
		} else if (strcmp("containerField",atts[i]) == 0) {
			//containerIndex = i+1;
		} else if (strcmp("DEF",atts[i]) == 0) {
			defNameIndex = i+1;
		} else if (strcmp("class",atts[i]) == 0) {
			ConsoleMessage ("field \"class\" not currently used in a ProtoInstance parse... sorry");
		} else if (strcmp("USE",atts[i]) == 0) {
			ConsoleMessage ("field \"USE\" not currently used in a ProtoInstance parse... sorry");
		}
	}


	currentContext = getContext(ud,TOP);
	pflagdepth = ciflag_get(currentContext->__protoFlags,0); //depth 0 we are deep inside protodeclare, depth 1 we are instancing live scenery

	/* did we find the name? */
	protoname = NULL;
	if (nameIndex != INT_ID_UNDEFINED) {
		protoname = atts[nameIndex];
	} else {
		ConsoleMessage ("\"ProtoInstance\" found, but field \"name\" not found!\n");
	}	
	if(protoname){
		struct X3D_Proto *proto;
		if( isAvailableBroto(protoname, currentContext , &proto))
		{
			//struct X3D_Node *parent;
			char* containerfield;
			/* its a binary proto, new in 2013 */
			int idepth = 0; //if its old brotos (2013) don't do depth until sceneInstance. If 2014 broto2, don't do depth here if we're in a protoDeclare or externProtoDeclare
			if(usingBrotos() ) idepth = pflagdepth == 1; //2014 broto2: if we're parsing a scene (or Inline) then deepcopy proto to instance it, else shallow
			node=X3D_NODE(brotoInstance(proto,idepth));
			node->_executionContext = X3D_NODE(proto);
			if (defNameIndex != INT_ID_UNDEFINED){
				char * defname = STRDUP(atts[defNameIndex]);
				broto_store_DEF(currentContext,node, defname);
			}
			add_node_to_broto_context(currentContext,node);

			pushNode(ud,node);
			containerfield = NULL;
			for (i = 0; atts[i]; i += 2) {
				if(!strcmp(atts[i],"containerField")) containerfield = atts[i+1];
			}
			if(containerfield) {
				int builtinField = findFieldInFIELDNAMES(containerfield);
				if(builtinField > INT_ID_UNDEFINED){
					node->_defaultContainer = builtinField;
				}
			}
			//linkNodeIn_B(ud);
			//parseAttributes_B(ud,atts); //PI uses FieldValue
		}else{
			pushNode(ud,NULL);
			ConsoleMessage ("Attempt to instance undefined prototype typename %s\n",protoname);
		}
	}
	pushField(ud,NULL); //no particular default field
	pushMode(ud,PARSING_PROTOINSTANCE);

}

/* handle a <ProtoInstance> tag */
void parseProtoInstance (void *ud, char **atts) {
	int count;
	int nameIndex;
	int containerIndex;
	int containerField;
	int defNameIndex;
	int protoTableIndex;
	ppX3DProtoScript p = (ppX3DProtoScript)gglobal()->X3DProtoScript.prv;

	/* initialization */
	nameIndex = INT_ID_UNDEFINED;
	containerIndex = INT_ID_UNDEFINED;
	containerField = INT_ID_UNDEFINED;
	defNameIndex = INT_ID_UNDEFINED;
	protoTableIndex = 0;

	pushMode(ud,PARSING_PROTOINSTANCE);
	p->curProtoInsStackInd++;

	#ifdef X3DPARSERVERBOSE
	printf ("parseProtoInstance, incremented curProtoInsStackInd to %d\n",curProtoInsStackInd);
	#endif

	p->currentProtoInstance[p->curProtoInsStackInd] = INT_ID_UNDEFINED;
	

	for (count = 0; atts[count]; count += 2) {
		#ifdef X3DPARSERVERBOSE
		TTY_SPACE
		printf ("parseProtoInstance: field %d , :%s=%s\n", count, atts[count], atts[count + 1]);
		#endif

		if (strcmp("name",atts[count]) == 0) {
			nameIndex=count+1;
		} else if (strcmp("containerField",atts[count]) == 0) {
			containerIndex = count+1;
		} else if (strcmp("DEF",atts[count]) == 0) {
			defNameIndex = count+1;

		} else if (strcmp("class",atts[count]) == 0) {
			ConsoleMessage ("field \"class\" not currently used in a ProtoInstance parse... sorry");
		} else if (strcmp("USE",atts[count]) == 0) {
			ConsoleMessage ("field \"USE\" not currently used in a ProtoInstance parse... sorry");
		}
		
	}
	#ifdef X3DPARSERVERBOSE
	printf ("...end of attributes\n");
	#endif

	/* did we have a containerField? */
	if (containerIndex != INT_ID_UNDEFINED) {
		containerField = findFieldInFIELDNAMES(atts[containerIndex]);
		/* printf ("parseProtoInstance, found a containerField of %s, id is %d\n",atts[containerIndex],containerField); */
	}

	/* so, the container will either be -1, or will have a valid FIELDNAMES index */
	p->ProtoInstanceTable[p->curProtoInsStackInd].container = containerField;

	/* did we have a DEF in the instance? */
	if (defNameIndex == INT_ID_UNDEFINED) p->ProtoInstanceTable[p->curProtoInsStackInd].defName = NULL;
	else p->ProtoInstanceTable[p->curProtoInsStackInd].defName = STRDUP(atts[defNameIndex]);

	/* did we find the name? */
	if (nameIndex != INT_ID_UNDEFINED) {
		#ifdef X3DPARSERVERBOSE
		printf ("parseProtoInstance, found name :%s:\n",atts[nameIndex]);
		#endif

		/* go through the PROTO table, and find the match, if it exists */
		for (protoTableIndex = 0; protoTableIndex <= p->currentProtoDeclare; protoTableIndex ++) {
			if (!p->PROTONames[protoTableIndex].isExternProto){
				if (strcmp(atts[nameIndex],p->PROTONames[protoTableIndex].definedProtoName) == 0) {
					p->currentProtoInstance[p->curProtoInsStackInd] = protoTableIndex;
					/* printf ("expanding proto %d\n",currentProtoInstance[curProtoInsStackInd]); */
					return;
				}
			}
		}
	
	} else {
		ConsoleMessage ("\"ProtoInstance\" found, but field \"name\" not found!\n");
	}

	/* initialize this call level */
	/* printf ("getProtoValue, curProtoInsStackInd %d, MAX %d\n",curProtoInsStackInd, PROTOINSTANCE_MAX_LEVELS); */
	if ((p->curProtoInsStackInd < 0) || (p->curProtoInsStackInd >= PROTOINSTANCE_MAX_LEVELS)) {
		ConsoleMessage ("too many levels of ProtoInstances, recompile with PROTOINSTANCE_MAX_LEVELS higher ");
		p->curProtoInsStackInd = 0;
	}

	p->ProtoInstanceTable[p->curProtoInsStackInd].paircount = 0;
	#ifdef X3DPARSERVERBOSE
	printf("unsuccessful end parseProtoInstance\n");
	#endif
}



	#define OPEN_AND_READ_PROTO \
		p->PROTONames[p->currentProtoInstance[p->curProtoInsStackInd]].fileDescriptor = fopen (p->PROTONames[p->currentProtoInstance[p->curProtoInsStackInd]].fileName,"r"); \
		rs = (int) fread(protoInString, 1, p->PROTONames[p->currentProtoInstance[p->curProtoInsStackInd]].charLen, p->PROTONames[p->currentProtoInstance[p->curProtoInsStackInd]].fileDescriptor); \
		protoInString[rs] = '\0'; /* ensure termination */ \
		fclose (p->PROTONames[p->currentProtoInstance[p->curProtoInsStackInd]].fileDescriptor); \
		/* printf ("OPEN AND READ %s returns:%s\n:\n",PROTONames[currentProtoInstance].fileName, protoInString); */ \
		if (rs != p->PROTONames[p->currentProtoInstance[p->curProtoInsStackInd]].charLen) { \
			ConsoleMessage ("protoInstance :%s:, expected to read %d, actually read %d\n",p->PROTONames[p->currentProtoInstance[p->curProtoInsStackInd]].definedProtoName,  \
				p->PROTONames[p->currentProtoInstance[p->curProtoInsStackInd]].charLen,rs); \
		} 


	#define CHANGE_UNIQUE_TO_SPECIFIC \
	{char *cp; while ((cp=strstr(curProtoPtr,UNIQUE_NUMBER_HOLDER))!=NULL) { \
		char *cp2; char *cp3; \
		char endch; \
		/* get the terminating quote - single or double quote */ \
		cp2 = cp; \
		while ((*cp2 != '"') && (*cp2 != '\'')) { \
			*cp2 = ' '; cp2++; \
		} \
		endch = *cp2;  \
		*cp2 = ' '; /* take away the ending character */ \
	 \
		/* now, go through and copy over the proto invocation number */ \
		cp2 = cp; \
		cp3 = uniqueIDstring; \
		while (*cp3 != '\0') {*cp2=*cp3; cp2++; cp3++;} *cp2 = endch; \
	} \
	} 
	

	#define INITIATE_SCENE \
	{ \
		fdl += fprintf (fileDescriptor, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<X3D><Scene><Group FreeWRL__protoDef='%d'> <!-- INITIATE SCENE -->\n",CPI.uniqueNumber); \
	}

#ifdef OLDCODE
//OLDCODE JAS - text of macro is embedded in actual code - grep for "MAKE_PROTO_COPY_FIELDS" to find it.
OLDCODE 	#define MAKE_PROTO_COPY_FIELDS \
OLDCODE 	myObj = PROTONames[currentProtoInstance[curProtoInsStackInd]].fieldDefs; \
OLDCODE 	fdl += fprintf (fileDescriptor, "<!--\nProtoInterface fields has %d fields -->\n",vectorSize(myObj->fields)); \
OLDCODE 	for (ind=0; ind<vectorSize(myObj->fields); ind++) { \
OLDCODE 		int i; struct ScriptFieldDecl* field; char *fv; \
OLDCODE 		field = vector_get(struct ScriptFieldDecl*, myObj->fields, ind); \
OLDCODE 		fv = field->ASCIIvalue;   /* pointer to ProtoDef value - might be replaced in loop below */ \
OLDCODE 		for (i=0; i<CPI.paircount; i++) { \
OLDCODE 			/* printf ("CPI has %s and %s\n",CPI.name[i],CPI.value[i]); */ \
OLDCODE 			if (CPI.name[i] && strcmp(CPI.name[i],fieldDecl_getShaderScriptName(field->fieldDecl))==0) {\
OLDCODE 				/* use the value passed in on invocation */ \
OLDCODE 				fv=CPI.value[i];} \
OLDCODE 		} \
OLDCODE 		/* JAS if (field->fieldDecl->mode != PKW_initializeOnly) { */ \
OLDCODE 		if (fv != NULL) { \
OLDCODE 		fdl += fprintf (fileDescriptor,"\t<Metadata%s DEF='%s_%s_%d' value='%s'/>\n", \
OLDCODE 			stringFieldtypeType(fieldDecl_getType(field->fieldDecl)), \
OLDCODE 			fieldDecl_getShaderScriptName(field->fieldDecl),  \
OLDCODE 			FREEWRL_SPECIFIC,  \
OLDCODE 			CPI.uniqueNumber, \
OLDCODE 			fv); \
OLDCODE 		} else { \
OLDCODE 		fdl += fprintf (fileDescriptor,"\t<Metadata%s DEF='%s_%s_%d' />\n", \
OLDCODE 			stringFieldtypeType(fieldDecl_getType(field->fieldDecl)), \
OLDCODE 			fieldDecl_getShaderScriptName(field->fieldDecl),  \
OLDCODE 			FREEWRL_SPECIFIC,  \
OLDCODE 			CPI.uniqueNumber); \
OLDCODE 		/* }  */  } \
OLDCODE 	} \
OLDCODE 	fdl += fprintf (fileDescriptor, "<!-- end of MAKE_PROTO_COPY_FIELDS --> \n");
#endif //OLDCODE 

static char fixchar [] = {'\'', '"', '&', '<', '>'};
static char* fixtable [] = {"&apos;", "&quot;", "&amp;", "&lt;", "&gt;"};
static void fixXmlString(char *fvout, char* fvin)
{
	//xml reader is expecting to see &amp; for &, &lt; for <, &gt; for >
	//and -special for x3d- for non-SF-delimiting quotes and apostrophies, backslashed: \&apos; for \', \&quot; for \"
	//so when we write xml, we need to translate
	//for mfstrings already concatonated from SFs, we'll have fvin=["\"" "\'" "&" "<" ">"\0]
	//and we want fvout=["\&quot;" "\&apos;" "&amp;" "&lt;" "&gt;"\0]
	//assume the ' and " have already been properly backslashed by the scene author ie \&quot; and \&apos;
	//Then our job is to search for &,\',\",<,> and substitute &amp; \&apos; \&quot; &lt; &gt;
	int i,j,k,len,translated,nadd;
	char *c = fvout;
	len = (int)strlen(fvin);
	nadd = 0;
	for(i=0;i<len;i++)
	{
		*c = fvin[i];
		translated = 0;
		for(j=0;j<5;j++)
			if(*c == fixchar[j]) 
			{
				if( (j < 2 && i >0 && *(c-1) == '\\') || j > 1)
				{
					translated = 1;
					nadd += strlen(fixtable[j]) -1;
					if(nadd > 1000) 
					{
						fvout = realloc(fvout,strlen(fvout)+1000);
						nadd = 0;
					}
					for(k=0;k<(int)strlen(fixtable[j]);k++)
					{
						*c = fixtable[j][k];
						c++;
					}
				}
			}
		if(!translated)
			c++;
	}
	*c = '\0';
}

/* have a </ProtoInstance> so should have valid name and fieldValues */
void expandProtoInstance(void *ud, struct VRMLLexer *myLexer, struct X3D_Group *myGroup) {
	int i;
	char *protoInString;
	int psSize;
	int rs;
	int pf;
	char *curProtoPtr;
	struct Shader_Script *myObj;
	indexT ind;
	char *tmpf;
	FILE *fileDescriptor;
	int fdl;
	size_t readSizeThrowAway;
	char uniqueIDstring[20];
	struct CRjsnameStruct *JSparamnames = getJSparamnames();
	ppX3DProtoScript p = (ppX3DProtoScript)gglobal()->X3DProtoScript.prv;

	UNUSED(readSizeThrowAway); // compiler warning mitigation

	/* initialization */
	curProtoPtr=  NULL;
	myObj = NULL;
	tmpf = gglobal()->Mainloop.tmpFileLocation;
	tmpf = TEMPNAM(gglobal()->Mainloop.tmpFileLocation,"freewrl_proto");
	fdl = 0;

	/* we tie a unique number to the currentProto */
	CPI.uniqueNumber = newProtoDefinitionPointer(NULL,p->currentProtoInstance[p->curProtoInsStackInd]);

	/* first, do we actually have a valid proto here? */
	if (p->currentProtoInstance[p->curProtoInsStackInd] == INT_ID_UNDEFINED) 
		return;

	#ifdef X3DPARSERVERBOSE
	printf ("\n*****************\nok, expandProtoInstance, have a valid protoInstance of %d\n",currentProtoInstance);
	#endif

	fileDescriptor = fopen(tmpf,"w");
	if (fileDescriptor == NULL) {
		printf ("wierd problem opening proto expansion file\n"); 
		return;
	}

	/* make up a string here; this will replace the UNIQUE_NUMBER_HOLDER string */
	sprintf (uniqueIDstring,"%d",CPI.uniqueNumber);

	/* step 0. Does this one contain a DEF? */
	if (p->ProtoInstanceTable[p->curProtoInsStackInd].defName != NULL) {
		#ifdef X3DPARSERVERBOSE
			printf ("ProtoInstance, have a DEF, defining :%s: for node %u\n",
			ProtoInstanceTable[curProtoInsStackInd].defName,(unsigned int) myGroup);
		#endif
		//DEFNameIndex is not idempotent: it changes the table of defnames if the given name isn't found
		//struct X3D_Node *me = 
		DEFNameIndex(p->ProtoInstanceTable[p->curProtoInsStackInd].defName,X3D_NODE(myGroup),FALSE);

		FREE_IF_NZ(p->ProtoInstanceTable[p->curProtoInsStackInd].defName);
	}

	/* step 1. read in the PROTO text. */
	psSize = p->PROTONames[p->currentProtoInstance[p->curProtoInsStackInd]].charLen * 10;

	if (psSize < 0) {
		ConsoleMessage ("problem with psSize in expandProtoInstance");
		return;
	}

	protoInString = MALLOC(char *, p->PROTONames[p->currentProtoInstance[p->curProtoInsStackInd]].charLen+1);
	protoInString[0] = '\0';
	curProtoPtr = protoInString;

	/* read in the PROTO into the "protoInString" */
	OPEN_AND_READ_PROTO

	/* change any of the UNIQUE_NUMBER_HOLDERS to this one */
	CHANGE_UNIQUE_TO_SPECIFIC


	#ifdef X3DPARSERVERBOSE
	printf ("expandProtoInstance, now, we have in memory:\n%s:\n", protoInString);
	#endif

	/* dump in a Group containing any routable fields */
	INITIATE_SCENE

	/* make that group for routing to/from this proto invocation. We might use the
	   default value as presented in the ProtoDeclare, or we might use a overwritten
	   value as passed in in the ProtoInstance */

	#ifdef X3DPARSERVERBOSE
	printf ("before MAKE_PROTO_COPY_FIELDS\n");
	printf ("ProtoInstance pair count %d\n",CPI.paircount);
	for (i=0; i<CPI.paircount; i++) {
		printf ("name %s\n",CPI.name[i]);
		printf ("value %s\n",CPI.value[i]);
	}
	#endif

	//MAKE_PROTO_COPY_FIELDS
//MAKE_PROTO_COPY_FIELDS macro >>>>>
	//#define MAKE_PROTO_COPY_FIELDS 
	myObj = p->PROTONames[p->currentProtoInstance[p->curProtoInsStackInd]].fieldDefs; 
	fdl += fprintf (fileDescriptor, "<!--\nProtoInterface fields has %d fields -->\n",vectorSize(myObj->fields)); 
	for (ind=0; ind<vectorSize(myObj->fields); ind++) { 
		int i; struct ScriptFieldDecl* field; char *fv; 
		field = vector_get(struct ScriptFieldDecl*, myObj->fields, ind); 
		fv = field->ASCIIvalue;   /* pointer to ProtoDef value - might be replaced in loop below */ 
		for (i=0; i<CPI.paircount; i++) { 
			/* printf ("CPI has %s and %s\n",CPI.name[i],CPI.value[i]); */ \
			if (CPI.name[i] && strcmp(CPI.name[i],fieldDecl_getShaderScriptName(field->fieldDecl))==0) {
				/* use the value passed in on invocation */ 
				if(field->valueSet)
					fv=CPI.value[i];
				else
					fv=NULL;
			} 
		} 
		/* JAS if (field->fieldDecl->mode != PKW_initializeOnly) { */ 
		if (fv != NULL) { 
			char* fv2 = (char *) malloc(strlen(fv)+1000);
			fixXmlString(fv2,fv);
		fdl += fprintf (fileDescriptor,"\t<Metadata%s DEF='%s_%s_%d' value='%s'/>\n", 
			stringFieldtypeType(fieldDecl_getType(field->fieldDecl)), 
			fieldDecl_getShaderScriptName(field->fieldDecl),  
			FREEWRL_SPECIFIC,  
			CPI.uniqueNumber, 
			fv2); //fv); 
			free(fv2);
		} else { 
		fdl += fprintf (fileDescriptor,"\t<Metadata%s DEF='%s_%s_%d' />\n", 
			stringFieldtypeType(fieldDecl_getType(field->fieldDecl)), 
			fieldDecl_getShaderScriptName(field->fieldDecl),  
			FREEWRL_SPECIFIC,  
			CPI.uniqueNumber); 
		/* }  */  } 
	} 
	fdl += fprintf (fileDescriptor, "<!-- end of MAKE_PROTO_COPY_FIELDS --> \n");
//<<<MAKE_PROTO_COPY_FIELDS macro


	/* dump the modified string down here */
	fdl += fprintf(fileDescriptor,"%s",curProtoPtr);
	fdl += fprintf (fileDescriptor, "</Group></Scene></X3D>\n");


	fclose(fileDescriptor);
	FREE_IF_NZ(protoInString);
	fileDescriptor = fopen (tmpf,"r");
	if (fileDescriptor == NULL) {
		printf ("wierd problem opening proto expansion file\n"); 
		return;
	}

	protoInString = MALLOC(char *, fdl+1);
	readSizeThrowAway = fread(protoInString, 1, fdl, fileDescriptor);
	protoInString[fdl] = '\0';
	fclose(fileDescriptor);
	UNLINK(tmpf);


	#ifdef X3DPARSERVERBOSE
	printf ("PROTO EXPANSION IS:\n%s\n:\n",protoInString);
	#endif

	/* parse this string */
	if (X3DParse (X3D_NODE(myGroup),X3D_NODE(myGroup),protoInString)) {
		#ifdef X3DPARSERVERBOSE
		printf ("PARSED OK\n");
		#endif
		if (p->ProtoInstanceTable[p->curProtoInsStackInd].container == INT_ID_UNDEFINED) 
			pf = FIELDNAMES_children;
		else
			pf = p->ProtoInstanceTable[p->curProtoInsStackInd].container;
		myGroup->_defaultContainer = pf;
	
		#ifdef X3DPARSERVERBOSE
		printf ("expandProtoInstance cpsi %d, the proto's container is %s and as an id %d\n", curProtoInsStackInd, 
			FIELDNAMES[pf],myGroup->_defaultContainer);
		#endif
	} else {
		#ifdef X3DPARSERVERBOSE
		printf ("DID NOT PARSE THAT WELL:\n%s\n:\n",protoInString);
		#endif
	}

	/* remove the ProtoInstance calls from this stack */
	for (i=0; i<p->ProtoInstanceTable[p->curProtoInsStackInd].paircount; i++) {
		FREE_IF_NZ (p->ProtoInstanceTable[p->curProtoInsStackInd].name[i]);
		FREE_IF_NZ (p->ProtoInstanceTable[p->curProtoInsStackInd].value[i]);
	}
	p->ProtoInstanceTable[p->curProtoInsStackInd].paircount = 0;

	#ifdef X3DPARSERVERBOSE
	printf ("expandProtoInstance: decrementing curProtoInsStackInd from %d\n",curProtoInsStackInd);
	#endif

	linkNodeIn(ud,__FILE__,__LINE__); 
	//DECREMENT_PARENTINDEX
	popNode(ud);
	p->curProtoInsStackInd--;
        FREE_IF_NZ(protoInString);



	/* ok, now, what happens is that we have Group->Group; and the second group has all the interesting stuff relating
	   to the PROTO expansion. Lets move this info to the parent Group. */
	{
		struct X3D_Group *par;
		struct X3D_Group *chi;

		par = myGroup;
		/* printf ("doing Group-Group stuff, par %u, chi %u\n",par,chi); */

		if (par->children.n==1) {
			chi = X3D_GROUP(par->children.p[0]);

			/* unlink the child node */
			AddRemoveChildren(X3D_NODE(par), 
				offsetPointer_deref(struct Multi_Node *,par,offsetof(struct X3D_Group,children)),
					(struct X3D_Node * *)&chi,
					1,2,__FILE__,__LINE__);


			/* copy the original children from the chi node, to the par node */
			while (chi->children.n>0) {
				struct X3D_Node *offspring;

				/* printf ("now, parent has %d children...\n",par->children.n);
				printf ("and chi has %d children\n",chi->children.n);
				*/

				offspring = X3D_NODE(chi->children.p[0]);
				/* printf ("offspring %d is %u, type %s\n",ind,offspring,stringNodeType(offspring->_nodeType)); */

				AddRemoveChildren(X3D_NODE(chi),offsetPointer_deref(struct Multi_Node *,chi,offsetof(struct X3D_Group,children)),
					&offspring, 1, 2, __FILE__, __LINE__);

				AddRemoveChildren(X3D_NODE(par),offsetPointer_deref(struct Multi_Node *,par,offsetof(struct X3D_Group,children)),
					&offspring, 1, 1, __FILE__, __LINE__);

				/* 
				printf ("offspring type is still %s\n",stringNodeType(offspring->_nodeType));
				printf ("moved offspring has %d parents \n",offspring->_nparents);
				printf ("and it is %u and parent is %u and chi is %u\n",
					offspring->_parents[0],par,chi);
				*/
			}
			

			/* copy the protoDef pointer over */
			/* printf ("par->protoDef was %d, now %d\n",par->FreeWRL__protoDef, chi->FreeWRL__protoDef); */
			par->FreeWRL__protoDef = chi->FreeWRL__protoDef; 

			/* move the FreeWRL_PROTOInterfaceNodes nodes to the parent */
			/* this is the same code as for the "children" field, above */
			while (chi->FreeWRL_PROTOInterfaceNodes.n>0) {
				struct X3D_Node *offspring;
				offspring = X3D_NODE(chi->FreeWRL_PROTOInterfaceNodes.p[0]);

				AddRemoveChildren(X3D_NODE(chi),offsetPointer_deref(struct Multi_Node *,chi,offsetof(struct X3D_Group,FreeWRL_PROTOInterfaceNodes)),
					&offspring, 1, 2, __FILE__, __LINE__);

				AddRemoveChildren(X3D_NODE(par),offsetPointer_deref(struct Multi_Node *,par,offsetof(struct X3D_Group,FreeWRL_PROTOInterfaceNodes)),
					&offspring, 1, 1, __FILE__, __LINE__);
			}

		
		}

	}
	/* NOTE: the "chi" node is now an empty group node, we could dispose of it */

	#ifdef X3DPARSERVERBOSE
{
	struct X3D_Group *myg;
	int i;
	myg = myGroup;
		printf ("expandProto, group %u has %d children, and %d FreeWRL_PROTOInterfaceNodes and freewrldefptr %d\n",
			myg,
			myg->children.n, myg->FreeWRL_PROTOInterfaceNodes.n,myg->FreeWRL__protoDef);
	for (i=0; i<myg->children.n; i++) {
		printf ("child %d is %s\n",i,stringNodeType(X3D_NODE(myg->children.p[i])->_nodeType));
	}

	if (myg->children.n > 0) {
	
	myg = X3D_GROUP(myg->children.p[0]);

		printf ("children/children; expandProto, parent group has %d children, and %d FreeWRL_PROTOInterfaceNodes defpyr %d\n",
			myg->children.n, myg->FreeWRL_PROTOInterfaceNodes.n,myg->FreeWRL__protoDef);
	for (i=0; i<myg->children.n; i++) {
		printf ("child %d is %s\n",i,stringNodeType(X3D_NODE(myg->children.p[i])->_nodeType));
	}
	}
}
	#endif
#undef X3DPARSERVERBOSE
	/* printf ("end of expandProtoInstance\n"); */
}

void parseProtoBody_B (void *ud, char **atts) {
	//push proto node on context stack
	pushContext(ud,getNode(ud,TOP));
	pushMode(ud,PARSING_PROTOBODY);
}
void parseProtoBody (void *ud, char **atts) {
	#ifdef X3DPARSERVERBOSE
	TTY_SPACE
	printf ("start of parseProtoBody\n");
	#endif

	//setParserMode(PARSING_PROTOBODY);
	pushMode(ud,PARSING_PROTOBODY);
}

#define X3DPARSERVERBOSE

void parseProtoDeclare_B (void *ud, char **atts) {
	/*	1.create a new proto but not registered node
		2.get user type name from atts
		3.set flag for shallow/declare
		4.add to current context's protoDeclare array
		5.push on node stack awaiting interface and body
	*/
	int i;
	struct X3D_Proto* proto;
	char *type_name, *appinfo, *documentation, *containerfield;
	struct ProtoDefinition* obj;
	struct X3D_Proto* parent;

	type_name = appinfo = documentation = containerfield = NULL;
	if(0) printf("in start protoDeclare\n");

	proto = createNewX3DNode0(NODE_Proto);
	for (i = 0; atts[i]; i += 2) {
		#ifdef X3DPARSERVERBOSE
		TTY_SPACE
		printf ("parseProtoDeclare: field:%s=%s\n", atts[i], atts[i+1]);
		#endif

		if (!strcmp("name",atts[i]) ) type_name = atts[i+1];
		else if(!strcmp("containerField",atts[i])) containerfield = atts[i+1];
		else if(!strcmp("appInfo",atts[i])) appinfo = atts[i+1];
		else if(!strcmp("documentation",atts[i])) documentation = atts[i+1];
	}

	parent = (struct X3D_Proto*)getContext(ud,TOP);
	obj=newProtoDefinition();

	/* did we find the name? */
	if (type_name) {
		obj->protoName = STRDUP(type_name);
	} else {
		printf ("warning - have proto but no name, so just copying a default string in\n");
		obj->protoName = STRDUP("noProtoNameDefined");
	}
	type_name = obj->protoName;

	if(parent->__protoDeclares == NULL)
		parent->__protoDeclares = newVector(struct X3D_Proto*,4);
	vector_pushBack(struct X3D_Proto*,parent->__protoDeclares,proto);
	proto->__parentProto = X3D_NODE(parent); //me->ptr; //link back to parent proto, for isAvailableProto search
	proto->__protoFlags = parent->__protoFlags;
	proto->__protoFlags = ciflag_set(proto->__protoFlags,0,0); //((char*)(&proto->__protoFlags))[0] = 0; //shallow instancing of protoInstances inside a protoDeclare 
	///[1] leave parent's the oldway flag if set
	proto->__protoFlags = ciflag_set(proto->__protoFlags,0,2); //((char*)(&proto->__protoFlags))[2] = 0; //this is a protoDeclare we are parsing
	proto->__protoFlags = ciflag_set(proto->__protoFlags,0,3); //((char*)(&proto->__protoFlags))[3] = 0; //not an externProtoDeclare
	//set ProtoDefinition *obj
	proto->__protoDef = obj;
	proto->__prototype = X3D_NODE(proto); //point to self, so shallow and deep instances will inherit this value
	proto->__typename = strdup(obj->protoName);
	if(containerfield){
		int builtinField = findFieldInFIELDNAMES(containerfield);
		if(builtinField > -1){
			proto->_defaultContainer = builtinField;
		}
	}

	pushMode(ud,PARSING_PROTODECLARE);
	pushNode(ud,X3D_NODE(proto));
	pushField(ud,"__children");
}

void parseProtoDeclare (void *ud, char **atts) {
	int count;
	int nameIndex;
	ppX3DProtoScript p = (ppX3DProtoScript)gglobal()->X3DProtoScript.prv;

	/* initialization */
	nameIndex = INT_ID_UNDEFINED;

	/* increment the currentProtoDeclare field. Check to see how many PROTOS we (bounds check) */
	p->currentProtoDeclare++;
	p->curProDecStackInd++;

	//setParserMode(PARSING_PROTODECLARE);
	pushMode(ud,PARSING_PROTODECLARE);
	
	for (count = 0; atts[count]; count += 2) {
		#ifdef X3DPARSERVERBOSE
		TTY_SPACE
		printf ("parseProtoDeclare: field:%s=%s\n", atts[count], atts[count + 1]);
		#endif

		if (strcmp("name",atts[count]) == 0) {nameIndex=count+1;}
		else if ((strcmp("appinfo", atts[count]) != 0)  ||
			(strcmp("documentation",atts[count]) != 0)) {
			#ifdef X3DPARSERVERBOSE
			ConsoleMessage ("found field :%s: in a ProtoDeclare -skipping",atts[count]);
			#endif
		}
	}

	/* did we find the name? */
	if (nameIndex != INT_ID_UNDEFINED) {
		/* found it, lets open a new PROTO for this one */
		registerProto(atts[nameIndex],NULL);
	} else {
		ConsoleMessage ("\"ProtoDeclare\" found, but field \"name\" not found!\n");
	}
}
#undef X3DPARSERVERBOSE
void Parser_scanStringValueToMem_B(union anyVrml* any, indexT ctype, char *value, int isXML);

void parseExternProtoDeclare_B (void *ud, char **atts) {
	/*	1.create a new proto but not registered node
		2.get user type name from atts
		3.set flag for shallow/declare
		4.add to current context's externProtoDeclare array
		5.push on node stack awaiting interface (with no initial values)
	*/
	int i;
	char *type_name, *appinfo, *documentation, *containerfield, *url;
	struct ProtoDefinition* obj;
	struct X3D_Proto* proto;
	struct X3D_Proto* parent;
	type_name = appinfo = documentation = containerfield = url = NULL;
	if(0) printf("in parseExternProtoDeclare_B\n");

	proto = createNewX3DNode0(NODE_Proto);
	for (i = 0; atts[i]; i += 2) {
		#ifdef X3DPARSERVERBOSE
		TTY_SPACE
		printf ("parseProtoDeclare: field:%s=%s\n", atts[i], atts[i+1]);
		#endif

		if (!strcmp("name",atts[i]) ) type_name = atts[i+1];
		else if(!strcmp("containerField",atts[i])) containerfield = atts[i+1];
		else if(!strcmp("appInfo",atts[i])) appinfo = atts[i+1];
		else if(!strcmp("documentation",atts[i])) documentation = atts[i+1];
		else if(!strcmp("url",atts[i])) url = atts[i+1];
	}

	parent = (struct X3D_Proto*)getContext(ud,TOP);
	obj=newProtoDefinition();

	/* did we find the name? */
	if (type_name) {
		obj->protoName = STRDUP(type_name);
	} else {
		printf ("warning - have proto but no name, so just copying a default string in\n");
		obj->protoName = STRDUP("noProtoNameDefined");
	}
	type_name = obj->protoName;

	if(parent->__externProtoDeclares == NULL)
		parent->__externProtoDeclares = newVector(struct X3D_Proto*,4);
	vector_pushBack(struct X3D_Proto*,parent->__externProtoDeclares,proto);
	proto->__parentProto = X3D_NODE(parent); //me->ptr; //link back to parent proto, for isAvailableProto search
	proto->__protoFlags = parent->__protoFlags;
	proto->__protoFlags = ciflag_set(proto->__protoFlags,0,0); //((char*)(&proto->__protoFlags))[0] = 0; //shallow instancing of protoInstances inside a protoDeclare 
	///[1] leave parent's the oldway flag if set
	proto->__protoFlags = ciflag_set(proto->__protoFlags,0,2); //((char*)(&proto->__protoFlags))[2] = 0; //this is a protoDeclare we are parsing
	proto->__protoFlags = ciflag_set(proto->__protoFlags,1,3); //((char*)(&proto->__protoFlags))[3] = 1; //an externProtoDeclare
	//set ProtoDefinition *obj
	proto->__protoDef = obj;
	proto->__prototype = X3D_NODE(proto); //point to self, so shallow and deep instances will inherit this value
	proto->__typename = strdup(obj->protoName);
	if(containerfield){
		int builtinField = findFieldInFIELDNAMES(containerfield);
		if(builtinField > -1){
			proto->_defaultContainer = builtinField;
		}
	}
	if(url){
		Parser_scanStringValueToMem_B((union anyVrml*)&proto->url, FIELDTYPE_MFString,url, TRUE);
	}
	proto->__loadstatus = 0; //= LOAD_INITIAL_STATE
	pushMode(ud,PARSING_EXTERNPROTODECLARE);
	pushNode(ud,X3D_NODE(proto));
	pushField(ud,"__children");

}
void parseExternProtoDeclare (void *ud, char **atts) {
	int count;
	int nameIndex;
	int urlIndex;
	ppX3DProtoScript p = (ppX3DProtoScript)gglobal()->X3DProtoScript.prv;

	/* initialization */
	nameIndex = INT_ID_UNDEFINED;
	urlIndex = INT_ID_UNDEFINED;

	/* increment the currentProtoDeclare field. Check to see how many PROTOS we (bounds check) */
	p->currentProtoDeclare++;
	p->curProDecStackInd++;

	//setParserMode(PARSING_EXTERNPROTODECLARE);
	pushMode(ud,PARSING_EXTERNPROTODECLARE);
	
	for (count = 0; atts[count]; count += 2) {
		#ifdef X3DPARSERVERBOSE
		TTY_SPACE
		printf ("parseExternProtoDeclare: field:%s=%s\n", atts[count], atts[count + 1]);
		#endif

		if (strcmp("name",atts[count]) == 0) {nameIndex=count+1;}
		if (strcmp("url",atts[count]) == 0) {urlIndex=count+1;}
		else if ((strcmp("appinfo", atts[count]) != 0)  ||
			(strcmp("documentation",atts[count]) != 0)) {
			#ifdef X3DPARSERVERBOSE
			ConsoleMessage ("found field :%s: in a ProtoDeclare -skipping",atts[count]);
			#endif
		}
	}

 
	/* did we find the name? */
	if (nameIndex != INT_ID_UNDEFINED) {
		/* did we find the url? */
		if (urlIndex != INT_ID_UNDEFINED) {
			/* found it, lets open a new PROTO for this one */
			registerProto(atts[nameIndex],atts[urlIndex]);
		} else {
			ConsoleMessage ("ExternProtoDeclare: no :url: field for ExternProto %s",atts[nameIndex]);
		}
		
	} else {
		ConsoleMessage ("\"ExternProtoDeclare\" found, but field \"name\" not found!\n");
	}
}


/* simple sanity check, and change mode */
void parseProtoInterface (void *ud, char **atts) {
	if (getMode(ud,TOP) != PARSING_PROTODECLARE && getMode(ud,TOP) != PARSING_EXTERNPROTODECLARE) {
		ConsoleMessage ("got a <ProtoInterface>, but not within a <ProtoDeclare>\n");
	}
	//setParserMode(PARSING_PROTOINTERFACE);
	pushMode(ud,PARSING_PROTOINTERFACE);
}


static char *getDefaultValuePointer(int type) {
	if ((type < 0) || (type > FIELDTYPES_COUNT)) return "";
	switch (type) {
		case FIELDTYPE_SFDouble      :
		case FIELDTYPE_MFDouble      :
		case FIELDTYPE_SFTime        :
		case FIELDTYPE_MFTime        :
		case FIELDTYPE_SFInt32      :
		case FIELDTYPE_MFInt32      :
		case FIELDTYPE_SFFloat      : 
		case FIELDTYPE_MFFloat      : return "0";

		case FIELDTYPE_SFVec2d       :
		case FIELDTYPE_MFVec2d       :
		case FIELDTYPE_SFVec2f       :
		case FIELDTYPE_MFVec2f       : return "0 0";

		case FIELDTYPE_SFVec3d       :  
		case FIELDTYPE_MFVec3d       :
		case FIELDTYPE_SFImage       :
		case FIELDTYPE_SFColor       :
		case FIELDTYPE_MFColor       :
		case FIELDTYPE_SFVec3f      : 
		case FIELDTYPE_MFVec3f      : return "0 0 0";

		case FIELDTYPE_SFVec4f       :
		case FIELDTYPE_MFVec4f       :
		case FIELDTYPE_SFVec4d       :
		case FIELDTYPE_MFVec4d       :
		case FIELDTYPE_SFColorRGBA   : 
		case FIELDTYPE_MFColorRGBA   :
		case FIELDTYPE_SFRotation   : 
		case FIELDTYPE_MFRotation   : return "0 0 0 0";

		case FIELDTYPE_SFBool       : 
		case FIELDTYPE_MFBool       : return "false";

		case FIELDTYPE_FreeWRLPTR    :
		case FIELDTYPE_SFNode        :
		case FIELDTYPE_MFNode        : return "NULL";

		case FIELDTYPE_SFString      : 
		case FIELDTYPE_MFString      : return "\"\"";

		case FIELDTYPE_SFMatrix3f    :
		case FIELDTYPE_MFMatrix3f    :
		case FIELDTYPE_SFMatrix3d    :
		case FIELDTYPE_MFMatrix3d    : return "0 0 0 0 0 0 0 0 0";
		case FIELDTYPE_SFMatrix4f    :
		case FIELDTYPE_MFMatrix4f    :
		case FIELDTYPE_SFMatrix4d    :
		case FIELDTYPE_MFMatrix4d    : return "0 0 0 0 0 0 0 0 0 0 0 0 0 0 0";
		default: {
			ConsoleMessage ("getDefaultValuePointer, unhandled type\n");
			return "0";
		}
	}
}
int findFieldInARR(const char* field, const char** arr, size_t cnt);
void parseScriptProtoField_B(void *ud, char **atts) {
	/* new user field definitions -name,type,mode- possibly with fieldvalue anyVrml
		- we will be parsing either:
			a ProtoDeclare or ExternProtoDeclare (extern will lack fieldValue)
			a Script Node or Shader node
		- Script field may have fieldValue as child element, or IS/connect as peer
		- ProtoDeclare may have fieldValue as child element
	*/ 
	//struct X3D_Node *node;
	int mp_name, mp_accesstype, mp_type, mp_value, i;
	int pkwmode, type;
	union anyVrml defaultValue; //, *value;
	char *fname, *cname;
	//value = NULL;
	cname = NULL;
	mp_name = mp_accesstype = mp_type = mp_value = ID_UNDEFINED;
	if(0) printf("start scriptProtoField\n");
	/* have a "key" "value" pairing here. They can be in any order; put them into our order */
	for (i = 0; atts[i]; i += 2) {
		/* skip any "appinfo" or "documentation" fields here */
		if ((strcmp("appinfo", atts[i]) != 0)  &&
			(strcmp("documentation",atts[i]) != 0)) {
			if (strcmp(atts[i],"name") == 0) { mp_name = i+1;
			} else if (strcmp(atts[i],"accessType") == 0) { mp_accesstype = i+1;
			} else if (strcmp(atts[i],"type") == 0) { mp_type = i+1;
			} else if (strcmp(atts[i],"value") == 0) { mp_value = i+1;
			} else {
				ConsoleMessage ("X3D Proto/Script parsing line %d: unknown field type %s",LINE,atts[i]);
				return;
			}
		}
	}
	if(mp_accesstype > -1 && mp_type > -1 && mp_name > -1){
		int valueSet;
		pkwmode = findFieldInARR(atts[mp_accesstype], PROTOKEYWORDS, PROTOKEYWORDS_COUNT);
		pkwmode = pkwmode > -1? X3DMODE(pkwmode) : pkwmode;
		type = findFieldInARR(atts[mp_type],FIELDTYPES,FIELDTYPES_COUNT);
		fname = atts[mp_name];
		cname = NULL;
		//memset(&defaultValue,0,sizeof(union anyVrml));
		bzero(&defaultValue, sizeof (union anyVrml));
		valueSet = FALSE;
		if(mp_value > -1){
			Parser_scanStringValueToMem_B(&defaultValue, type, atts[mp_value], TRUE);
			valueSet = TRUE;
		}
		if(pkwmode > -1 && type > -1){
			struct X3D_Node * node = getNode(ud,TOP);
			if(node->_nodeType == NODE_Proto){
				struct X3D_Proto *pnode;
				struct ProtoFieldDecl* pfield;
				struct ProtoDefinition* pstruct;
				pnode = X3D_PROTO(node);
				pstruct = (struct ProtoDefinition*) pnode->__protoDef;
				pfield = newProtoFieldDecl(pkwmode,type,0);
				pfield->cname = strdup(fname);
				cname = pfield->cname;
				memcpy(&pfield->defaultVal,&defaultValue,sizeof(union anyVrml));
				vector_pushBack(struct ProtoFieldDecl*, pstruct->iface, pfield);
				//value = &pfield->defaultVal;
			}else{
				struct Shader_Script* shader = NULL;
				struct ScriptFieldDecl* sfield;
				int jsname;
				switch(node->_nodeType) 
				{ 
					case NODE_Script:         shader =(struct Shader_Script *)(X3D_SCRIPT(node)->__scriptObj); break;
					case NODE_ComposedShader: shader =(struct Shader_Script *)(X3D_COMPOSEDSHADER(node)->_shaderUserDefinedFields); break;
					case NODE_ShaderProgram:  shader =(struct Shader_Script *)(X3D_SHADERPROGRAM(node)->_shaderUserDefinedFields); break;
					case NODE_PackagedShader: shader =(struct Shader_Script *)(X3D_PACKAGEDSHADER(node)->_shaderUserDefinedFields); break;
				}
				jsname = JSparamIndex (fname, atts[mp_type]);
				cname = getJSparamnames()[jsname].name;
				//sfield = newScriptFieldDecl() // too hard to fathom, I'll break it out:
				sfield = MALLOC(struct ScriptFieldDecl *, sizeof(struct ScriptFieldDecl));
				bzero(sfield,sizeof(struct ScriptFieldDecl));
				sfield->fieldDecl = newFieldDecl(pkwmode,type,0,jsname,0); //not using a lexer
				memcpy(&sfield->value,&defaultValue,sizeof(union anyVrml));
				sfield->valueSet = valueSet; //=(mod!=PKW_initializeOnly);
				sfield->eventInSet = FALSE; //flag used for directOutput
				vector_pushBack(struct ScriptFieldDecl*, shader->fields, sfield);
				//value = &sfield->value;
			}
		}
	}
	pushField(ud,cname); //strdup(fname)); //strong recommendation
	pushMode(ud,PARSING_FIELD);
}

/* parse a script or proto field. Note that they are in essence the same, just used differently */
void parseScriptProtoField(void *ud, struct VRMLLexer* myLexer, char **atts) {
	int i;
	int myparams[MPFIELDS];
	int which;
	int myFieldNumber;
	const char *myValueString;
	int myAccessType;
	int myValueType;
	struct Shader_Script *myObj;
	struct ScriptFieldDecl* sdecl;
	indexT name;
	union anyVrml defaultVal;
	ttglobal tg = gglobal();
	ppX3DProtoScript p = (ppX3DProtoScript)tg->X3DProtoScript.prv;

	/* initialization */
	myValueString = NULL;
	myObj = NULL;
	sdecl = NULL;
	name = ID_UNDEFINED;
	bzero(&defaultVal, sizeof (union anyVrml));

	#ifdef X3DPARSERVERBOSE
	printf ("start of parseScriptProtoField, parentStack is of type %s\n",stringNodeType(parentStack[parentIndex]->_nodeType));
	printf("parentIndex=%d\n",parentIndex);
	printf ("	mode is %s, currentProtoDeclare is %d\n",
				parserModeStrings[getMode(ud,TOP)],
				currentProtoDeclare);
	if (PROTONames[currentProtoDeclare].isExternProto) printf ("	it IS an ExternProto\n"); else printf ("	it is NOT an ExternProto\n");
	for (i = 0; atts[i]; i += 2) printf("%s %s\n",atts[i],atts[i+1]);

	#endif

	/* are we parsing an EXTERNPROTO? If so, we should... */
	if (getMode(ud,TOP) == PARSING_EXTERNPROTODECLARE) {
		if (p->currentProtoDeclare != INT_ID_UNDEFINED) {
			myObj = CPD.fieldDefs;
		}
	} else if( getMode(ud,TOP) == PARSING_PROTOINTERFACE) {
		/* make sure we are really in a proto declare. 
		  dug9 july27,2010: There's no wrapper Group for a ProtoDeclare - that comes later at the Instance stage
		  therefore we test by getMode(ud,TOP) */
		if ((p->currentProtoDeclare > INT_ID_UNDEFINED) && (p->currentProtoDeclare < p->MAXProtos))
		{
			myObj = CPD.fieldDefs;
		}
		else
		{
			ConsoleMessage("got an error on parseScriptProtoField currentProtodeclare out of range: %d\n",p->currentProtoDeclare);
			return;
		}
	} else {
		/* configure internal variables, and check sanity for top of stack This should be a Script node */
		struct X3D_Node *node = getNode(ud,TOP);
		switch (node->_nodeType) {
			case NODE_Script: {
				struct X3D_Script* myScr = NULL;
				myScr = X3D_SCRIPT(node);
				myObj = (struct Shader_Script *) myScr->__scriptObj;
				break; }
			case NODE_ComposedShader: {
				struct X3D_ComposedShader* myScr = NULL;
				myScr = X3D_COMPOSEDSHADER(node);
				myObj = (struct Shader_Script *) myScr->_shaderUserDefinedFields;
				break; }
			case NODE_ShaderProgram: {
				struct X3D_ShaderProgram* myScr = NULL;
				myScr = X3D_SHADERPROGRAM(node);
				myObj = (struct Shader_Script *) myScr->_shaderUserDefinedFields;
				break; }
			case NODE_PackagedShader: {
				struct X3D_PackagedShader* myScr = NULL;
				myScr = X3D_PACKAGEDSHADER(node);
				myObj = (struct Shader_Script *) myScr->_shaderUserDefinedFields;
				break; }
			default: {
				ConsoleMessage("got an error on parseScriptProtoField, do not know how to handle field for parent type %s",
					stringNodeType(node->_nodeType));
				return;
			}

		}
	}

	/* error checking */
	if (myObj == NULL) {
		ConsoleMessage ("parsing a <field> but not really in a PROTO or Script");
		return;
	}

	/* set up defaults for field parsing */
	for (i=0;i<MPFIELDS;i++) myparams[i] = INT_ID_UNDEFINED;
	

	/* copy the fields over */
	/* have a "key" "value" pairing here. They can be in any order; put them into our order */
	for (i = 0; atts[i]; i += 2) {
		#ifdef X3DPARSERVERBOSE
		TTY_SPACE
		printf ("parseScriptProtoField, field %d, looking at %s=\"%s\"\n",i,atts[i],atts[i+1]);
		#endif

		/* skip any "appinfo" or "documentation" fields here */
		if ((strcmp("appinfo", atts[i]) != 0)  &&
			(strcmp("documentation",atts[i]) != 0)) {
			if (strcmp(atts[i],"name") == 0) { which = MP_NAME;
			} else if (strcmp(atts[i],"accessType") == 0) { which = MP_ACCESSTYPE;
			} else if (strcmp(atts[i],"type") == 0) { which = MP_TYPE;
			} else if (strcmp(atts[i],"value") == 0) { which = MP_VALUE;
			} else {
				ConsoleMessage ("X3D Proto/Script parsing line %d: unknown field type %s",LINE,atts[i]);
				return;
			}
			if (myparams[which] != INT_ID_UNDEFINED) {
				ConsoleMessage ("X3DScriptParsing line %d, field %s already defined in this Script/Proto",
					LINE,atts[i],atts[i+1]);
			}
	
			/* record the index for the value of this field */
			myparams[which] = i+1;
		}
	}

	#ifdef X3DPARSERVERBOSE
	printf ("ok, fields copied over\n");
	printf ("myparams:name ind: %d accessType %d type: %d value %d\n",myparams[MP_NAME],myparams[MP_ACCESSTYPE],myparams[MP_TYPE], myparams[MP_VALUE]);
	printf ("and the values are: ");
	if (myparams[MP_NAME] != ID_UNDEFINED) printf ("name:%s ", atts[myparams[MP_NAME]]);
	if (myparams[MP_ACCESSTYPE] != ID_UNDEFINED) printf ("access:%s ", atts[myparams[MP_ACCESSTYPE]]);
	if (myparams[MP_TYPE] != ID_UNDEFINED) printf ("type:%s ", atts[myparams[MP_TYPE]]);
	if (myparams[MP_VALUE] != ID_UNDEFINED) printf ("value:%s ", atts[myparams[MP_VALUE]]);
	printf ("\n");
	#endif

	/* ok now, we have a couple of checks to make here. Do we have all the parameters required? */
	if (myparams[MP_NAME] == INT_ID_UNDEFINED) {
		ConsoleMessage("have a Script/PROTO at line %d with no parameter name",LINE);
		return;
	}
	if (myparams[MP_TYPE] == INT_ID_UNDEFINED) {
		ConsoleMessage("have a Script/PROTO at line %d with no parameter type",LINE);
		return;
	}

	if (myparams[MP_ACCESSTYPE] == INT_ID_UNDEFINED) {
		ConsoleMessage ("have a Script/PROTO at line %d with no paramater accessType ",LINE);
		return;
	}

	if (myparams[MP_VALUE] != INT_ID_UNDEFINED) myValueString = atts[myparams[MP_VALUE]];

	/* register this field with the Javascript Field Indexer */
	myFieldNumber = JSparamIndex(atts[myparams[MP_NAME]],atts[myparams[MP_TYPE]]);
	UNUSED(myFieldNumber); // compiler warning mitigation- JSparamIndex call actually does something



	/* convert eventIn, eventOut, field, and exposedField to new names */
	myAccessType = findFieldInPROTOKEYWORDS(atts[myparams[MP_ACCESSTYPE]]);
	switch (myAccessType) {
		case PKW_eventIn: myAccessType = PKW_inputOnly; break;
		case PKW_eventOut: myAccessType = PKW_outputOnly; break;
		case PKW_exposedField: myAccessType = PKW_inputOutput; break;
		case PKW_field: myAccessType = PKW_initializeOnly; break;
		default: {}
	}
	
	/* printf ("field parsing, so we have  accessType %d fieldnumber %d, need to get field value\n",
		myAccessType, myFieldNumber);  */

	/* get the name index */
       	lexer_fromString(myLexer,STRDUP(atts[myparams[MP_NAME]]));

	/* put it on the right Vector in myLexer */
    	switch(myAccessType) {
		#define LEX_DEFINE_FIELDID(suff) \
		   case PKW_##suff: \
		    if(!lexer_define_##suff(myLexer, &name)) \
		     ConsoleMessage ("Expected fieldNameId after field type!"); \
		    break;

        LEX_DEFINE_FIELDID(initializeOnly)
        LEX_DEFINE_FIELDID(inputOnly)
        LEX_DEFINE_FIELDID(outputOnly)
        LEX_DEFINE_FIELDID(inputOutput)
               default:
		ConsoleMessage ("define fieldID, unknown access type, %d\n",myAccessType);
		return;
    	}

	myValueType = findFieldInFIELDTYPES(atts[myparams[MP_TYPE]]);
	/* so, inputOnlys and outputOnlys DO NOT have initialValues, inputOutput and initializeOnly DO */
	if ((myAccessType == PKW_initializeOnly) || (myAccessType == PKW_inputOutput)) {
		if (myValueString == NULL) {
			if(myValueType==FIELDTYPE_SFNode) 
			{
				//vrmlNodeT nulval = NULL;
				struct X3D_Node* nulval = NULL;
				//struct X3D_Node sfnode;
				//static vrmlNodeT nulval = NULL;
				//if(nulval == NULL) nulval = (struct X3D_Node*)createNewX3DNode(NODE_Color);
				memcpy(&defaultVal,&nulval,sizeof(struct X3D_Node*)); //struct X3D_Node));
				//((struct Multi_Node *)&defaultVal)->n = 0;
				//((struct Multi_Node *)&defaultVal)->p = NULL;
			}
			else if(myValueType==FIELDTYPE_MFNode)
			{
				((struct Multi_Node *)&defaultVal)->n = 0;
				((struct Multi_Node *)&defaultVal)->p = NULL;
			}
			else
			{
				myValueString = getDefaultValuePointer(myValueType); /* myparams[MP_TYPE]); */
			}
		}
	}
				

	/* create a new scriptFieldDecl */
	sdecl = newScriptFieldDecl(myLexer,(indexT) myAccessType, myValueType, name);
		/* findFieldInFIELDTYPES(atts[myparams[MP_TYPE]]),  name); */
#ifdef X3DPARSERVERBOSE
	printf ("created a new script field declaration, it is %u\n",sdecl);
#endif


	/* for now, set the value  -either the default, or not... */
	if (myValueString != NULL) {
/* 
void Parser_scanStringValueToMem(struct X3D_Node *node, size_t coffset, int ctype, char *value, int isXML);
*/
		Parser_scanStringValueToMem(X3D_NODE(&defaultVal), 0, fieldDecl_getType(sdecl->fieldDecl), (char *)myValueString, TRUE);
	}
	scriptFieldDecl_setFieldValue(sdecl, defaultVal);

		
	/* fill in the string name and type */
	/* sdecl->ASCIIname=STRDUP(atts[myparams[MP_NAME]]); */

	/* if we are parsing a PROTO interface, we might as well save the value as a string, because we will need it later */
	if (getMode(ud,TOP) == PARSING_PROTOINTERFACE) {
		if (myValueString != NULL)
			scriptFieldDecl_setFieldASCIIValue(sdecl, STRDUP(myValueString));
	}

	/* add this field to the script */
	script_addField(myObj,sdecl);

	///BIGPUSH added by dug9 July 18,2010
	/* In Script and ProtoInterface fields, what if the field type is SFNode or MFNode, 
	   and InitializeOnly or InputOutput? Then child elements of <field> need to be captured -recursively:
	   <field name='Buildings' ... >
	      <Transform USE='House1'/>
		  <Transform>
			<Shape>
				<Box/>
			<Shape>
		  </Transform>
		  <Script ...>
		    <field ...>  <<<<<---------- recursive field within a field
			  <Transform USE='House2'/>
		    </field>
		  </Script>
		  <ProtoInstance ...>
		    <fieldValue ..>
			  <Transform USE="House3"/>
		    </fieldValue>
		  </ProtoInstance>
	   </field>
	  
	  Method: we fool the regular PARSING_NODES to put the field values into the children of a Group node
	   then look in the group node later -on pop/end element (see below)- to see if we got anything
      To manage recursion, we keep the Group node in an array 
	  And because a proto can have many MFNode fields, we malloc a data structure when we find a field
    */
	p->fieldNodeParsingStateB[getNodeTop(ud)].parsingMFSFNode = 0;
	if( ((getMode(ud,TOP) == PARSING_NODES)||(getMode(ud,TOP) == PARSING_PROTOINTERFACE)) && (myValueType == FIELDTYPE_SFNode || myValueType == FIELDTYPE_MFNode)  )
	{
		if ((myAccessType == PKW_initializeOnly) || (myAccessType == PKW_inputOutput)) 
		{
			p->fieldNodeParsingStateB[getNodeTop(ud)].parsingMFSFNode = 1;
			if(!p->fieldNodeParsingStateB[getNodeTop(ud)].fieldHolderInitialized)
				p->fieldNodeParsingStateB[getNodeTop(ud)].fieldHolder = (struct X3D_Node*)createNewX3DNode(NODE_Group);
			X3D_GROUP(p->fieldNodeParsingStateB[getNodeTop(ud)].fieldHolder)->children.n = 0;
			pushMode(ud,PARSING_NODES);
			//INCREMENT_PARENTINDEX //parentIndex++;
			//getNode(ud,TOP) = p->fieldNodeParsingStateB[getNodeTop(ud)-1].fieldHolder;
			pushNode(ud,p->fieldNodeParsingStateB[getNodeTop(ud)-1].fieldHolder);
			//if (getChildAttributes(getNodeTop(ud))!=NULL) deleteChildAttributes(getNodeTop(ud)); //deleteVector (struct nameValuePairs*, childAttributes[parentIndex]);
			//setChildAttributes(getNodeTop(ud),NULL);
			//setChild(ud,TOP,NULL);
			setAtt(ud,TOP,NULL);
			/* saving sdecl and script index for convenience when we pop/end element- see below */
			p->fieldNodeParsingStateB[getNodeTop(ud)-1].mfnodeSdecl = sdecl;
			p->fieldNodeParsingStateB[getNodeTop(ud)-1].myObj_num = myObj->num;
			p->fieldNodeParsingStateB[getNodeTop(ud)-1].myObj = myObj;
		}
	}
#undef X3DPARSERVERBOSE
}
void scriptFieldDecl_jsFieldInit(struct ScriptFieldDecl* me, int num);
void endScriptProtoField_B(void *ud) {
	if(0) printf("end scriptprotofield\n");
	popField(ud);
	popMode(ud); //PARSING_FIELD);

}
void endScriptProtoField(void *ud)  
{
	ttglobal tg = gglobal();
	ppX3DProtoScript p = (ppX3DProtoScript)tg->X3DProtoScript.prv;

	///BIGPOP 
	if(p->fieldNodeParsingStateB[getNodeTop(ud)-1].parsingMFSFNode == 1)
	{
		//DECREMENT_PARENTINDEX //parentIndex--;
		popNode(ud);
        if(X3D_GROUP(p->fieldNodeParsingStateB[getNodeTop(ud)].fieldHolder)->children.n > 0)
		{ 
			/* we got something */
			union anyVrml v;
			int n; 
			int j;
			int myValueType;
			struct Multi_Node *kids;
			kids = &X3D_GROUP(p->fieldNodeParsingStateB[getNodeTop(ud)].fieldHolder)->children; 
			n = kids->n;
			myValueType = fieldDecl_getType(p->fieldNodeParsingStateB[getNodeTop(ud)].mfnodeSdecl->fieldDecl);
			//myValueType = FIELDTYPE_MFNode;
			if(myValueType ==  FIELDTYPE_MFNode)
			{
				/*struct Multi_Node { int n; void * *p; };*/
				((struct Multi_Node *)&v)->n=n;
				((struct Multi_Node *)&v)->p=MALLOC(struct X3D_Node **, n*sizeof(struct X3D_Node*));
				for(j=0;j<n;j++)
				{
					((struct Multi_Node *)&v)->p[j] = kids->p[j];
					//remove_parent((struct X3D_Node *)kids->p[j], fieldNodeParsingState[curProtoInsStackInd].fieldHolder);
				}
			}
			else if(myValueType ==  FIELDTYPE_SFNode)
			{
				//struct X3D_Node* tmp;
				//struct X3D_Transform* tmp2;
				/* take the first child as an SFNode */
				/* basically the anyVrml holds a pointer to the node (and that's all it holds) */
				//printf("sizeof union anyvrml=%d\n",sizeof(union anyVrml));
				//printf("sizeof struct x3d_node=%d\n",sizeof(struct X3D_Node));
				//printf("sizeof struct x3d_transform=%d\n",sizeof(struct X3D_Transform));
				//printf("element length FIELDTYPE_SFNODE=%d\n", returnElementLength(FIELDTYPE_SFNode));

				//tmp2 = (struct X3D_Transform*)(kids->p[0]);
				memcpy(&v,&kids->p[0],sizeof(struct X3D_Node*));
				//tmp = (struct X3D_Node*)*(unsigned int *)&v;
				//tmp2 = (struct X3D_Transform*)*(unsigned int *)&v;
				//printf("%d\n",tmp->_nodeType);
				//remove_parent((struct X3D_Node *)kids->p[0], fieldNodeParsingState[curProtoInsStackInd].fieldHolder);
				/* if we were ambitious we would FREE any extra children here */
			}
			scriptFieldDecl_setFieldValue(p->fieldNodeParsingStateB[getNodeTop(ud)].mfnodeSdecl, v);
			//		script_addField(myObj,sdecl);
		   //if (fieldNodeParsingState[curProtoInsStackInd].myObj->ShaderScriptNode->_nodeType==NODE_Script) 
		   if(p->fieldNodeParsingStateB[getNodeTop(ud)].myObj_num > -1)
			   #ifdef HAVE_JAVASCRIPT
			   scriptFieldDecl_jsFieldInit(p->fieldNodeParsingStateB[getNodeTop(ud)].mfnodeSdecl, p->fieldNodeParsingStateB[tg->X3DParser.parentIndex].myObj->num);
			   #endif

			/* clean up holder for next time */
			X3D_GROUP(p->fieldNodeParsingStateB[getNodeTop(ud)].fieldHolder)->children.n = 0;
		}
		else
		{
			//fieldNodeParsingState[curProtoInsStackInd].mfnodeSdecl->value = NULL; default set in start element above
			p->fieldNodeParsingStateB[getNodeTop(ud)].mfnodeSdecl->valueSet = FALSE;
		}
		p->fieldNodeParsingStateB[getNodeTop(ud)].parsingMFSFNode = 0;
		popMode(ud);
	}
}


#ifdef HAVE_JAVASCRIPT
/* we get script text from a number of sources; from the URL field, from CDATA, from a file
   pointed to by the URL field, etc... handle the data in one place */

void initScriptWithScript(void *ud) {
	struct X3D_Script * me;
	char *myText;
	struct Shader_Script *myObj;
	ttglobal tg = gglobal();
	/* initialization */
	myText = NULL;

	/* semantic checking... */
	me = X3D_SCRIPT(getNode(ud,TOP));
	myObj = (struct Shader_Script *) me->__scriptObj;

	if (me->_nodeType != NODE_Script) {
		ConsoleMessage ("initScriptWithScript - Expected to find a NODE_Script, got a %s\n",
		stringNodeType(me->_nodeType));
		return;
	}

	#ifdef X3DPARSERVERBOSE
	printf ("endElement: CDATA_Text is %s\n",CDATA_Text);
	#endif

	/* did the script text come from a CDATA node?? */
	if (tg->X3DParser.CDATA_Text != NULL) if (tg->X3DParser.CDATA_Text[0] != '\0') myText = tg->X3DParser.CDATA_Text;


	/* is this CDATA text? */
	if (myText != NULL) {
		struct Multi_String strHolder;

		strHolder.p = MALLOC (struct Uni_String **, sizeof(struct Uni_String)*1);
		strHolder.p[0] = newASCIIString(myText);
		strHolder.n=1; 
		script_initCodeFromMFUri(myObj, &strHolder);
		FREE_IF_NZ(strHolder.p[0]->strptr);
		FREE_IF_NZ(strHolder.p);
	} else {
		script_initCodeFromMFUri(myObj, &X3D_SCRIPT(me)->url);
	}

	/* finish up here; if we used the CDATA area, set its length to zero */
	if (myText != NULL) tg->X3DParser.CDATA_Text_curlen=0;

	//supposed to be matched setParserMode(PARSING_NODES);
	#ifdef X3DPARSERVERBOSE
	printf ("endElement: got END of script - script should be registered\n");
	#endif
}
#endif /* HAVE_JAVASCRIPT */

void addToProtoCode(const char *name) {
	ppX3DProtoScript p = (ppX3DProtoScript)gglobal()->X3DProtoScript.prv;

        if (CPD.fileOpen) 
                CPD.charLen += fprintf (CPD.fileDescriptor,"</%s>\n",name);
}


void endExternProtoDeclare(void *ud) {
	char *pound;
	char *buffer;
	int foundOk;
	resource_item_t *res;
	ppX3DProtoScript p = (ppX3DProtoScript)gglobal()->X3DProtoScript.prv;

	#ifdef X3DPARSERVERBOSE
	TTY_SPACE
	printf ("endElement, end of ExternProtoDeclare %d stack %d\n",currentExternProtoDeclare,curProDecStackInd);
	#endif

	foundOk = FALSE;

	/* printf ("endExternProtoDeclare - now the rubber hits the road\n");  */

	/* printf ("externProtoName %s\nexternProtoName.url %s \n",CPD.definedProtoName,CPD.url); */
	if (CPD.url != NULL) {
		struct Multi_String url;

		pound = NULL;
		buffer = NULL;

		/* change the string of strings into an array of strings */
		Parser_scanStringValueToMem(X3D_NODE(&url),0,FIELDTYPE_MFString,CPD.url,TRUE);
		/* printf ("just scanned %d strings...\n",url.n); */

		res = resource_create_multi(&url);
		resource_identify(gglobal()->resources.root_res, res);
		if (res->type != rest_invalid) {
#ifdef HAD_RESOURCE_LOAD    //dug9 broke extern text protos here aug 2014 and reimplemented with different code as brotos in fall 2014
			if (resource_fetch(res)) {
				if (resource_load(res)) {
					s_list_t *l;
					openned_file_t *of;
					l = res->openned_files;
					of = ml_elem(l);
					buffer = (char *)of->fileData;
/* 				pound = strchr(buffer, '#'); */
/* 				embedEXTERNPROTO(me, myName, buffer, pound); */
				/*	printf("**** X3D EXTERNPROTO:\n%s\n", buffer); */
				}
			}
#else
			res->status = ress_failed;
			printf("externProto not currently supported\n");
#endif
		}
		
		
		if (res->status == ress_loaded) {
			/* ok - we are replacing EXTERNPROTO with PROTO */
			res->status = ress_parsed;
			res->complete = TRUE;
			foundOk = TRUE;
		} else {
			printf("Ouch ress not loaded\n");
			/* failure, FIXME: remove res from root_res... */
/* 		resource_destroy(res); */
		}

		/*
		 if (buffer != NULL) { 
			printf ("EPD: just read in %s\n",buffer);
		}
		if (pound != NULL) {
			printf ("EPD, pound is %s\n",pound);
		} 
		*/

		/* were we successful? */
		if (!foundOk) {
			ConsoleMessage ("<ExternProtoDeclare> of name %s not found",CPD.definedProtoName);
		} else {
			/* printf ("finding and expanding ExternProtoDeclare %s\n",CPD.definedProtoName);
			printf ("	pound %s\n",pound);
			printf ("	currentProtoDeclare %d\n",currentProtoDeclare); */
			
			pushMode(ud,PARSING_NODES);

			compareExternProtoDeclareWithProto(buffer,pound);
			popMode(ud); //setParserMode(PARSING_EXTERNPROTODECLARE);
		}

		/* decrement the protoDeclare stack count. If we are nested, get out of the nesting */
		#ifdef X3DPARSERVERBOSE
		printf ("endExternProtoDeclare, stack was %d, decrementing it\n",curProDecStackInd);
		{
		int i;
		printf ("endExternProtoDeclare, PROTONames: \n");
		for (i=0; i<=currentProtoDeclare; i++) {
		printf ("	cpd %d of %d\n",i,currentProtoDeclare);
		printf ("	definedProtoName: %s\n",PROTONames[i].definedProtoName);
		printf ("	url	        : %s\n",PROTONames[i].url);
		printf ("	charLen         : %d\n",PROTONames[i].charLen);
		printf ("	fileName	: %s\n",PROTONames[i].fileName);
		if (PROTONames[i].isExternProto)
		printf ("	isExternProto	: TRUE\n");
		else printf ("	isExternProto	: FALSE\n");
		printf ("\n");
		}
		}
		#endif

		p->curProDecStackInd--;

		/* now, a ExternProtoDeclare should be within normal nodes; make the expected mode PARSING_NODES, assuming
		   we don't have nested ExternProtoDeclares  */
		//if (curProDecStackInd == 0) setParserMode(PARSING_NODES);
		//popMode(ud);

		if (p->curProDecStackInd < 0) {
			ConsoleMessage ("X3D_Parser found too many end ExternProtoDeclares at line %d\n",LINE);
			p->curProDecStackInd = 0; /* reset it */
		}

	}
}

void endProtoDeclare(void *ud) {
	ppX3DProtoScript p = (ppX3DProtoScript)gglobal()->X3DProtoScript.prv;

		#ifdef X3DPARSERVERBOSE
		TTY_SPACE
		printf ("endElement, end of ProtoDeclare %d stack %d\n",currentProtoDeclare,curProDecStackInd);
		{
		int i;
		printf ("endProtoDeclare, PROTONames: \n");
		for (i=0; i<=currentProtoDeclare; i++) {
		printf ("	cpd %d of %d\n",i,currentProtoDeclare);
		printf ("	definedProtoName: %s\n",PROTONames[i].definedProtoName);
		printf ("	url	        : %s\n",PROTONames[i].url);
		printf ("	charLen         : %d\n",PROTONames[i].charLen);
		printf ("	fileName	: %s\n",PROTONames[i].fileName);
		if (PROTONames[i].isExternProto)
		printf ("	isExternProto	: TRUE\n");
		else printf ("	isExternProto	: FALSE\n");
		printf ("\n");
		}
		}
		#endif

		/* decrement the protoDeclare stack count. If we are nested, get out of the nesting */
		p->curProDecStackInd--;

		/* now, a ProtoDeclare should be within normal nodes; make the expected mode PARSING_NODES, assuming
		   we don't have nested ProtoDeclares  */
		//if (curProDecStackInd == 0) setParserMode(PARSING_NODES);
		//popMode(ud); done in x3dparser.c end tag

		if (p->curProDecStackInd < 0) {
			ConsoleMessage ("X3D_Parser found too many end ProtoDeclares at line %d\n",LINE);
			p->curProDecStackInd = 0; /* reset it */
		}

		/* if we are in an ExternProtoDeclare, we need to decrement here to keep things sane */
		if (p->currentProtoDeclare >=1) {
			if (p->PROTONames[p->currentProtoDeclare-1].isExternProto) {
				//DECREMENT_PARENTINDEX
				popNode(ud);
			}
		}
}


#endif
