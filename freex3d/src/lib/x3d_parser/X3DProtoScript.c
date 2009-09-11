/*
=INSERT_TEMPLATE_HERE=

$Id$

???

*/


#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>


#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../scenegraph/Vector.h"
#include "../vrml_parser/CFieldDecls.h"
#include "../world_script/CScripts.h"
#include "../world_script/fieldSet.h"
#include "../vrml_parser/CParseParser.h"
#include "../vrml_parser/CParseLexer.h"
#include "../vrml_parser/CProto.h"
#include "../vrml_parser/CParse.h"
#include "../input/EAIHelpers.h"

#include "X3DParser.h"
#include "X3DProtoScript.h"


static int currentProtoDeclare  = INT_ID_UNDEFINED;
static int MAXProtos = 0;
static int curProDecStackInd = 0;
static int currentProtoInstance = INT_ID_UNDEFINED;
static int getFieldAccessMethodFromProtoInterface (struct VRMLLexer *myLexer, char *fieldName, int protono);

/* for parsing fields in PROTO expansions */
/* FIELD_END is an ascii string that will pass the XML parser, but should not be found in a field value */
#define FIELD_END "\t \t\n" 
#define strIS "<IXXS>"
#define strNOTIS "</IXXS>"
#define strCONNECT  "<connect"
#define strNODEFIELD "nodeField"
#define strPROTOFIELD "protoField"
#define MAX_ID_SIZE 1000
#define NODEFIELD_EQUALS "nodeField=\""
#define PROTOFIELD_EQUALS "protoField=\""

#define CPI ProtoInstanceTable[curProtoInsStackInd]

/* for parsing script initial fields */
#define MPFIELDS 4
#define MP_NAME 0
#define MP_ACCESSTYPE 1
#define MP_TYPE 2
#define MP_VALUE 3

#define UNIQUE_NUMBER_HOLDER "-fReeWrl-UniqueNumH"

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
                "unused high"};

/* ProtoInstance table This table is a dynamic table that is used for keeping track of ProtoInstance field values... */
static int curProtoInsStackInd = -1;

struct PROTOInstanceEntry {
	char *name[PROTOINSTANCE_MAX_PARAMS];
	char *value[PROTOINSTANCE_MAX_PARAMS];
	char *defName;
	int container;
	int paircount;
	int uniqueNumber;
};
static struct PROTOInstanceEntry ProtoInstanceTable[PROTOINSTANCE_MAX_LEVELS];

/* PROTO table */
struct PROTOnameStruct {
	char *definedProtoName;
	FILE *fileDescriptor;
	char *fileName;
	int charLen;
	int fileOpen;
	struct Shader_Script *fieldDefs;
};
static struct PROTOnameStruct *PROTONames = NULL;

#ifdef OLDCODE
/****************************** PROTOS ***************************************************/

/* we are closing off a parse of an XML file. Lets go through and free/unlink/cleanup. */
void freeProtoMemory () {
	int i;

	
	#ifdef X3DPARSERVERBOSE
	printf ("freeProtoMemory, currentProtoDeclare is %d PROTONames = %d \n",currentProtoDeclare,(int) PROTONames);
	#endif

	if (PROTONames != NULL) {
		for (i=0; i<= currentProtoDeclare; i++) {
			if (PROTONames[i].fileOpen) fclose(PROTONames[i].fileDescriptor); /* should never happen... */

			FREE_IF_NZ (PROTONames[i].definedProtoName);
			if (PROTONames[i].fileName != NULL) UNLINK (PROTONames[i].fileName);
			free (PROTONames[i].fileName); /* can not FREE_IF_NZ this one as it's memory is not kept track of by MALLOC */

		}
		FREE_IF_NZ(PROTONames);
	}

	currentProtoDeclare  = INT_ID_UNDEFINED;
	MAXProtos = 0;

	#ifdef X3DPARSERVERBOSE
/* error: `ScriptFieldNames' undeclared (first use in this function) */
/* 	printf ("freeProtoMemory,ScriptFieldNames is %d ScriptFieldTableSize %d, MAXScriptFieldParams %d\n",ScriptFieldNames, ScriptFieldTableSize, MAXScriptFieldParams); */
	#endif

}


#endif

/* record each field of each script - the type, kind, name, and associated script */
static void registerProto(const char *name) {
	#ifdef X3DPARSERVERBOSE
	TTY_SPACE
	printf ("registerProto for currentProtoDeclare %d name %s\n",currentProtoDeclare, name);
	#endif


	/* ok, we got a name and a type */
	if (currentProtoDeclare >= MAXProtos) {
		/* oooh! not enough room at the table */
		MAXProtos += 10; /* arbitrary number */
		PROTONames = (struct PROTOnameStruct*)REALLOC (PROTONames, sizeof(*PROTONames) * MAXProtos);
	}

	PROTONames[currentProtoDeclare].definedProtoName = STRDUP((char *)name);
	PROTONames[currentProtoDeclare].fileName = TEMPNAM("/tmp","freewrl_proto");
	PROTONames[currentProtoDeclare].fileDescriptor = fopen(PROTONames[currentProtoDeclare].fileName,"w");
	PROTONames[currentProtoDeclare].charLen =  0;
	PROTONames[currentProtoDeclare].fileOpen = TRUE;
	PROTONames[currentProtoDeclare].fieldDefs = new_Shader_Script(NULL);

	#ifdef X3DPARSERVERBOSE
	TTY_SPACE
	printf ("opened name %s for PROTO name %s id %d\n",PROTONames[currentProtoDeclare].fileName,
		PROTONames[currentProtoDeclare].definedProtoName,currentProtoDeclare);
	#endif
}



static int getFieldAccessMethodFromProtoInterface (struct VRMLLexer *myLexer, char *fieldName, int protono) {
	const char** userArr;
	size_t userCnt;
	struct Shader_Script* myObj;
	indexT retUO;

	#ifdef X3DPARSERVERBOSE
	printf ("getFieldValueFromProtoInterface, lexer %u looking for :%s: in proto %d\n",myLexer, fieldName, protono);
	#endif
	
	myObj = PROTONames[protono].fieldDefs;

        
#define LOOK_FOR_FIELD_IN(whicharr) \
        userArr=&vector_get(const char*, myLexer->user_##whicharr, 0); \
        userCnt=vector_size(myLexer->user_##whicharr);\
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
	char *retptr;
	int i;

	#ifdef X3DPARSERVERBOSE
	printf ("getProtoKind for proto %d, char :%s:\n",ProtoInvoc, id);
	#endif

	/* get the start/end value pairs, and copy them into the id field. */
	if ((curProtoInsStackInd < 0) || (curProtoInsStackInd >= PROTOINSTANCE_MAX_LEVELS)) {
		return INT_ID_UNDEFINED;
	}

	return getFieldAccessMethodFromProtoInterface (myLexer, id, ProtoInvoc);
}

#define TOD parentStack[parentIndex]

/* lets see if this node has a routed field  fromTo  = 0 = from node, anything else = to node */
#define ROUTE_FROM_META_TO_ISD \
	getRoutingInfo (myLexer, TOD, &nodeOffs, &type, &accessType, nodeField, 1); \
	if (nodeOffs != INT_ID_UNDEFINED) CRoutes_RegisterSimple(metaNode, metaFromOffs, TOD, nodeOffs, returnRoutingElementLength(type)); 


#define ROUTE_FROM_ISD_TO_META \
	getRoutingInfo (myLexer, TOD, &nodeOffs, &type, &accessType, nodeField, 0); \
	if (nodeOffs != INT_ID_UNDEFINED) CRoutes_RegisterSimple(TOD, nodeOffs, metaNode, metaToOffs, returnRoutingElementLength(type)); 


static unsigned int uniqueDefNameNumber = 31567;
static void generateRoute (struct VRMLLexer *myLexer, struct ScriptFieldDecl* protoField, char *nodeField) {
	int myFieldKind;
	char *protoFieldID = "pfID";
	char *myDefName="(top of stack node)";
	char newname[1000];
	struct X3D_Node *newn;
	struct X3D_Node *metaNode;
	int fromOffset, toOffset, fromType;
	int fieldInt;
	int metaToOffs;
	int metaFromOffs;
	int type;
	int accessType;
	int nodeOffs;


	if (TOD==NULL) {
		printf ("generateRoute, internal problem, TOD==NULL\n");
		return;
	}

	
	/* we have names and fields, lets generate routes for this */
	myFieldKind = getProtoKind(myLexer,currentProtoInstance,protoField->ASCIIname);

	/* find the MetaMF/MetaSF node to route to/from to: */
	sprintf(newname,"%s_%s_%d",protoField->ASCIIname,FREEWRL_SPECIFIC,CPI.uniqueNumber);
	metaNode = DEFNameIndex ((const char *)newname,NULL,FALSE);

	/* set up these offsets for routing to/from the Meta node */
	fieldInt = findRoutedFieldInFIELDNAMES (metaNode, "valueChanged", FALSE);
	if (fieldInt >=0) findFieldInOFFSETS(metaNode->_nodeType, fieldInt, &metaFromOffs, &type, &accessType);

	fieldInt = findRoutedFieldInFIELDNAMES (metaNode, "setValue", TRUE);
	if (fieldInt >=0) findFieldInOFFSETS(metaNode->_nodeType, fieldInt, &metaToOffs, &type, &accessType);

printf (" should generate a route or two here to route from the internal Meta node to the ISd node, kind %d\n",myFieldKind);




	switch (myFieldKind) {
		case PKW_inputOnly: 
			/* 1 */ printf ("<ROUTE fromNode='%s_%s_%d' fromField='valueChanged' toNode='%s' toField='%s'/>\n",
					protoField->ASCIIname,FREEWRL_SPECIFIC,CPI.uniqueNumber,myDefName,nodeField);
				ROUTE_FROM_META_TO_ISD

			break;
		case PKW_outputOnly: 
			/* 2 */printf ("<ROUTE toNode='%s_%s_%d' toField='setValue' fromNode='%s' fromField='%s'/>\n",
					protoField->ASCIIname,FREEWRL_SPECIFIC,CPI.uniqueNumber,myDefName,nodeField);
				ROUTE_FROM_ISD_TO_META
			break;
		case PKW_inputOutput: 
			/* 1 */ printf ("<ROUTE fromNode='%s_%s_%d' fromField='valueChanged' toNode='%s' toField='%s'/>\n",
					protoField->ASCIIname,FREEWRL_SPECIFIC,CPI.uniqueNumber,myDefName,nodeField);
				ROUTE_FROM_META_TO_ISD
			/* 2 */printf ("<ROUTE toNode='%s_%s_%d' toField='setValue' fromNode='%s' fromField='%s'/>\n",
					protoField->ASCIIname,FREEWRL_SPECIFIC,CPI.uniqueNumber,myDefName,nodeField);
				ROUTE_FROM_ISD_TO_META
			break;
		case PKW_initializeOnly: 
			/* printf ("PKW_initializeOnly - do nothing\n"); */
			break;
		default :
			printf ("generateRoute unknown proto type - ignoring\n");
	}
printf ("end generate internal nodes\n");
}


#define REPLACE_CONNECT_VALUE(value) \
			/* now, we have a match, replace (or push) this onto the params */ \
       			for (nodeind=0; nodeind<vector_size(tos); nodeind++) { \
               			nvp = vector_get(struct nameValuePairs*, tos,nodeind); \
				printf ("      nvp %d, fieldName:%s fieldValue:%s\n",ind,nvp->fieldName,nvp->fieldValue); \
				if (nvp!=NULL) { \
					if (strcmp(nvp->fieldName,atts[nfInd+1])==0) { \
						/* we have a field already of this name, replace the value */ \
						printf ("we had %s, now have %s\n",nvp->fieldValue, value); \
						FREE_IF_NZ(nvp->fieldValue); \
						nvp->fieldValue=STRDUP(value); \
						return; \
					} \
				} \
			} \
			printf ("no match, have to create new entty and push it\n"); \
			nvp = MALLOC(sizeof (struct nameValuePairs *)); \
			nvp->fieldName=STRDUP(atts[nfInd+1]); \
			nvp->fieldValue=STRDUP(value); \
			vector_pushBack(struct nameValuePairs*,tos,nvp); \
			printf ("pushed %s=%s to tos\n",nvp->fieldName, nvp->fieldValue); \
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
void parseConnect(struct VRMLLexer *myLexer, const char **atts, struct Vector *tos) {
	int i;
	struct nameValuePairs *nvp;
	size_t ind;
	size_t nodeind;
	int nfInd;
	int pfInd;
	struct Shader_Script* myObj;
	int matched;

	nfInd=INT_ID_UNDEFINED;
	pfInd=INT_ID_UNDEFINED;

	/* printf ("parseConnect mode is %s\n",parserModeStrings[getParserMode()]);  */

	if (getParserMode() != PARSING_IS) {
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
	/* printf ("parseConnect, currentProto is %d\n",currentProtoInstance); 
	printf ("	and, we have %s and %s\n",atts[pfInd+1],atts[nfInd+1]);*/
	matched = FALSE;
	myObj = PROTONames[currentProtoInstance].fieldDefs; 
	for (ind=0; ind<vector_size(myObj->fields); ind++) { 
		struct ScriptFieldDecl* field; 
		field = vector_get(struct ScriptFieldDecl*, myObj->fields, ind); 
		/* printf ("ind %d name %s value %s\n", ind, field->ASCIIname,  field->ASCIIvalue);  */

		if (strcmp(field->ASCIIname,atts[pfInd+1])==0) {
			/* printf ("parseConnect, have match, value is %s\n",field->ASCIIvalue); */
			matched=TRUE;
			generateRoute(myLexer, field, atts[nfInd+1]);
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
	/* printf ("parseConnect, curProtoInsStackInd %d\n",curProtoInsStackInd); */
	for (i=0; i<CPI.paircount; i++) {
		/* printf ("	index %d is name :%s: value :%s:\n",i,CPI.name[i],
			CPI.value[i]);

		printf ("... comparing %s and %s\n",CPI.name[i],atts[pfInd+1]); */
		if (strcmp(CPI.name[i],atts[pfInd+1])==0) {
			/* printf ("parseConnect, have match, value is %s\n",CPI.value[i]);   */
			/* if there is no value here, just return, as some accessMethods do not have value */
			if (CPI.value[i]==NULL) return;

			REPLACE_CONNECT_VALUE(CPI.value[i])
			/* printf ("parseConnect, match completed\n"); */
		}
	}

	/* did not find it in the ProtoInstance fields, lets look in the ProtoDeclare */
	/* printf ("parseConnect, did not find %s in the ProtoInstance, looking for it in ProtoDeclare\n",atts[pfInd+1]); */
	
	myObj = PROTONames[currentProtoInstance].fieldDefs; 
	for (ind=0; ind<vector_size(myObj->fields); ind++) { 
		struct ScriptFieldDecl* field; 
		field = vector_get(struct ScriptFieldDecl*, myObj->fields, ind); 
		/* printf ("ind %d name %s value %s\n", ind, field->ASCIIname,  field->ASCIIvalue);  */

		if (strcmp(field->ASCIIname,atts[pfInd+1])==0) {
			/* printf ("parseConnect, have match, value is %s\n",field->ASCIIvalue); */
			/* if there is no value here, just return, as some accessMethods do not have value */
			if (field->ASCIIvalue==NULL) return;

			REPLACE_CONNECT_VALUE(field->ASCIIvalue)
			/* printf ("parseConnect, match completed\n"); */
		}
	} 
}

void endConnect() {
	printf ("endConnect mode is %s\n",parserModeStrings[getParserMode()]); 
	setParserMode(PARSING_NODES);
}


void parseProtoInstanceFields(const char *name, const char **atts) {
	int count;
	int picatindex;
	int picatmalloc;

	/* initialization */
	picatindex = 0;
	picatmalloc = 0;

	#define INDEX ProtoInstanceTable[curProtoInsStackInd].paircount	
	#define ZERO_NAME_VALUE_PAIR \
		ProtoInstanceTable[curProtoInsStackInd].name[INDEX] = NULL; \
		ProtoInstanceTable[curProtoInsStackInd].value[INDEX] = NULL;

	#define VERIFY_PCAT_LEN(myLen) \
		if ((myLen + 10) >= picatmalloc) { \
			if (picatmalloc == 0) { \
				picatmalloc = 1024; \
				picatindex = 0; \
			} \
			while ((picatmalloc + 20) < myLen) picatmalloc *= 2; \
			ProtoInstanceTable[curProtoInsStackInd].value[INDEX] = REALLOC(ProtoInstanceTable[curProtoInsStackInd].value[INDEX], picatmalloc); \
		} \

	#define PICAT_CAT(myStr,myLen) \
		memcpy(&ProtoInstanceTable[curProtoInsStackInd].value[INDEX][picatindex], myStr,myLen); \
		picatindex += myLen; \
		ProtoInstanceTable[curProtoInsStackInd].value[INDEX][picatindex+1] = '\0'; 
		
	#define PICAT(myStr,myLen) \
		VERIFY_PCAT_LEN(myLen) \
		PICAT_CAT(myStr,myLen)


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
				ProtoInstanceTable[curProtoInsStackInd].name[INDEX] = STRDUP(atts[count+1]);
			if (strcmp("value",atts[count])==0) 
				ProtoInstanceTable[curProtoInsStackInd].value[INDEX] = STRDUP(atts[count+1]);

			/* did we get both a name and a value? */
			if ((ProtoInstanceTable[curProtoInsStackInd].name[INDEX] != NULL) &&
			    (ProtoInstanceTable[curProtoInsStackInd].value[INDEX] != NULL)) {
				INDEX++;
				ZERO_NAME_VALUE_PAIR
			}

			if (INDEX>=PROTOINSTANCE_MAX_PARAMS) {
				ConsoleMessage ("too many parameters for ProtoInstance, sorry...\n");
				INDEX=0;
			}
		}
	} else if (strcmp(name,"ProtoInstance") != 0) {
		/* maybe this is a SFNode value, as in: 
                    <fieldValue name='relay'> <Script USE='CameraRelay'/> </fieldValue>

		  if this IS the case, we will be down here where the atts parameter will be the "USE=CameraRelay" pair
		*/
#ifdef WIN32
		/* MUFTI points out http://www.web3d.org/x3d/specifications/ISO-IEC-FDIS-19776-1.2-X3DEncodings-XML/Part01/Examples/ShuttlesAndPendulums.x3d
		   and it bombs in here parsing the ProtoInstance fields with name=Shape and atts=<bad pointer>
         <fieldValue name='children'>
           <Shape>
             <Cylinder height='5'/>
             <Appearance>
               <Material diffuseColor='0.8 0.1 0'/>
              </Appearance>
            </Shape>
          </fieldValue>
		  this could be refactored into something like the Script USE= scenario. When I do that, the scene parsing 
		  does not complete all nodes normally - I get a blank screen.
		  My guess: in general this should recurse - we should be back parsing nodes to build the value attribute.
		  June15: John sent the follwoing line
int i;printf ("X3D_ node name :%s:\n",name);for (i = 0; atts[i]; i += 2) {printf("field:%s=%s\n", atts[i], atts[i + 1]);}
		  PROBLEM > atts char** is never NULL. But atts[0] is.
		*/
#endif 
		
		/* printf ("ProtoInstance, looking for fieldValue, found string:%s: current name=%s value=%s, index %d\n",name,
			ProtoInstanceTable[curProtoInsStackInd].name[INDEX], ProtoInstanceTable[curProtoInsStackInd].value[INDEX],INDEX); */
		{
		}
		/* allow ONLY USEs here, at least for now? */
/* #ifdef WIN32 */
/* 		if(atts[0] != NULL ){ */
/* #else */
		if (atts != NULL) {
/* #endif */
			if (strcmp("USE",atts[0]) == 0) {
				struct X3D_Node *rv;
				char *val = MALLOC(20);

				rv = DEFNameIndex(atts[1],X3D_NODE(NULL),FALSE);
				/* printf ("found USE in proto expansion for SFNode, is %u\n",rv); */
				sprintf (val, "%u",(unsigned int) rv);
				ProtoInstanceTable[curProtoInsStackInd].value[INDEX] = val;
			} else {
				/* NOT SURE THE FOLLOWING IS A GOOD IDEA */ 
				/* NOT SURE THE FOLLOWING IS A GOOD IDEA */ 
				/* NOT SURE THE FOLLOWING IS A GOOD IDEA */ 
				/* NOT SURE THE FOLLOWING IS A GOOD IDEA */ 
				/* NOT SURE THE FOLLOWING IS A GOOD IDEA */ 
				/* NOT SURE THE FOLLOWING IS A GOOD IDEA */ 

				/* just append this, and hope for the best */
				for (count = 0; atts[count]; count += 2) {
					PICAT(atts[count],strlen(atts[count]))
					PICAT("=",1)
					PICAT(atts[count+1],strlen(atts[count+1]))
				}
			}
		}
		/* printf ("NOW ProtoInstance, looking for fieldValue, found string:%s: current name=%s value=%s, index %d\n",name,
			ProtoInstanceTable[curProtoInsStackInd].name[INDEX], ProtoInstanceTable[curProtoInsStackInd].value[INDEX],INDEX); */
		INDEX++;
		ZERO_NAME_VALUE_PAIR
	}
	#undef INDEX
}


/***********************************************************************************/

#ifdef OLDCODE
#endif

void dumpProtoBody (const char *name, const char **atts) {
	int count;
	int inRoute;

	/* initialization */
	inRoute = FALSE;

	#ifdef X3DPARSERVERBOSE
	TTY_SPACE
	printf ("dumping ProtoBody for %s\n",name);
	#endif

	if (PROTONames[currentProtoDeclare].fileOpen) {
		inRoute = strcmp(name,"ROUTE") == 0;

		PROTONames[currentProtoDeclare].charLen += fprintf (PROTONames[currentProtoDeclare].fileDescriptor, "<%s",name);

		/* if we are in a ROUTE statement, encode the fromNode and toNode names */
		if (inRoute) {
		    for (count = 0; atts[count]; count += 2) {
			/* printf ("dumpProtoBody - do we need to worry about quotes in :%s: \n",atts[count+1]); */
			if ((strcmp("fromNode",atts[count])==0) || (strcmp("toNode",atts[count])==0)) {
				PROTONames[currentProtoDeclare].charLen +=
				    fprintf (PROTONames[currentProtoDeclare].fileDescriptor," %s='%s_%s_%s' %s",
					atts[count],
					atts[count+1],
					FREEWRL_SPECIFIC,
					UNIQUE_NUMBER_HOLDER,
					FIELD_END);

			} else {
			    PROTONames[currentProtoDeclare].charLen += 
				fprintf (PROTONames[currentProtoDeclare].fileDescriptor," %s=\"%s\" %s",atts[count],atts[count+1],FIELD_END);
			}
		    }

		/* this is a non-route statement, but look for DEF and USE names */
		} else {
		    for (count = 0; atts[count]; count += 2) {
			/* printf ("dumpProtoBody - do we need to worry about quotes in :%s: \n",atts[count+1]); */
			if ((strcmp("DEF",atts[count])==0) || (strcmp("USE",atts[count])==0)) {
				PROTONames[currentProtoDeclare].charLen +=
				    fprintf (PROTONames[currentProtoDeclare].fileDescriptor," %s='%s_%s_%s' %s",
					atts[count],
					atts[count+1],
					FREEWRL_SPECIFIC,
					UNIQUE_NUMBER_HOLDER,
					FIELD_END);

			} else {
			  if (atts[count+1][0] == '"') {
			    /* put single quotes around this one */
			    PROTONames[currentProtoDeclare].charLen += 
				fprintf (PROTONames[currentProtoDeclare].fileDescriptor," %s='%s' %s",atts[count],atts[count+1],FIELD_END);
			  } else {
			    PROTONames[currentProtoDeclare].charLen += 
				fprintf (PROTONames[currentProtoDeclare].fileDescriptor," %s=\"%s\" %s",atts[count],atts[count+1],FIELD_END);
			  }
			}
		    }
		}
		PROTONames[currentProtoDeclare].charLen += fprintf (PROTONames[currentProtoDeclare].fileDescriptor,">\n");
	}
}

void dumpCDATAtoProtoBody (char *str) {
	if (PROTONames[currentProtoDeclare].fileOpen) {
		PROTONames[currentProtoDeclare].charLen += 
			fprintf (PROTONames[currentProtoDeclare].fileDescriptor,"<![CDATA[%s]]>",str);
	}
}

void endDumpProtoBody (const char *name) {
	/* we are at the end of the ProtoBody, close our tempory file */
	if (PROTONames[currentProtoDeclare].fileOpen) {
		fclose (PROTONames[currentProtoDeclare].fileDescriptor);
		PROTONames[currentProtoDeclare].fileOpen = FALSE;

		#ifdef X3DPARSERVERBOSE
			TTY_SPACE
			printf ("endDumpProtoBody, just closed %s, it is %d characters long\n",
				PROTONames[currentProtoDeclare].fileName, PROTONames[currentProtoDeclare].charLen);
		#endif
	}
}


/* handle a <ProtoInstance> tag */
void parseProtoInstance (const char **atts) {
	int count;
	int nameIndex;
	int containerIndex;
	int containerField;
	int defNameIndex;
	int protoTableIndex;

	/* initialization */
	nameIndex = INT_ID_UNDEFINED;
	containerIndex = INT_ID_UNDEFINED;
	containerField = INT_ID_UNDEFINED;
	defNameIndex = INT_ID_UNDEFINED;
	protoTableIndex = 0;

	setParserMode(PARSING_PROTOINSTANCE);
	curProtoInsStackInd++;

	#ifdef X3DPARSERVERBOSE
	printf ("parseProtoInstance, incremented curProtoInsStackInd to %d\n",curProtoInsStackInd);
	#endif

	currentProtoInstance = INT_ID_UNDEFINED;
	

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
	ProtoInstanceTable[curProtoInsStackInd].container = containerField;

	/* did we have a DEF in the instance? */
	if (defNameIndex == INT_ID_UNDEFINED) ProtoInstanceTable[curProtoInsStackInd].defName = NULL;
	else ProtoInstanceTable[curProtoInsStackInd].defName = STRDUP(atts[defNameIndex]);

	/* did we find the name? */
	if (nameIndex != INT_ID_UNDEFINED) {
		#ifdef X3DPARSERVERBOSE
		printf ("parseProtoInstance, found name :%s:\n",atts[nameIndex]);
		#endif

		/* go through the PROTO table, and find the match, if it exists */
		for (protoTableIndex = 0; protoTableIndex <= currentProtoDeclare; protoTableIndex ++) {
			if (strcmp(atts[nameIndex],PROTONames[protoTableIndex].definedProtoName) == 0) {
#ifdef X3DPARSERVERBOSE
				printf("successfully matched :%s: to :%s: protoDeclare to protoInstance\n",PROTONames[protoTableIndex].definedProtoName,atts[nameIndex]);
#endif
				currentProtoInstance = protoTableIndex;
				return;
			}
		}
	
	} else {
		ConsoleMessage ("\"ProtoInstance\" found, but field \"name\" not found!\n");
	}

	/* initialize this call level */
printf ("getProtoValue, curProtoInsStackInd %d, MAX %d\n",curProtoInsStackInd, PROTOINSTANCE_MAX_LEVELS);
	if ((curProtoInsStackInd < 0) || (curProtoInsStackInd >= PROTOINSTANCE_MAX_LEVELS)) {
		ConsoleMessage ("too many levels of ProtoInstances, recompile with PROTOINSTANCE_MAX_LEVELS higher ");
		curProtoInsStackInd = 0;
	}

	ProtoInstanceTable[curProtoInsStackInd].paircount = 0;
	#ifdef X3DPARSERVERBOSE
	printf("unsuccessful end parseProtoInstance\n");
	#endif
}



	#define OPEN_AND_READ_PROTO \
		PROTONames[currentProtoInstance].fileDescriptor = fopen (PROTONames[currentProtoInstance].fileName,"r"); \
		rs = fread(protoInString, 1, PROTONames[currentProtoInstance].charLen, PROTONames[currentProtoInstance].fileDescriptor); \
		protoInString[rs] = '\0'; /* ensure termination */ \
		fclose (PROTONames[currentProtoInstance].fileDescriptor); \
		/* printf ("OPEN AND READ %s returns:%s\n:\n",PROTONames[currentProtoInstance].fileName, protoInString); */ \
		if (rs != PROTONames[currentProtoInstance].charLen) { \
			ConsoleMessage ("protoInstance :%s:, expected to read %d, actually read %d\n",PROTONames[currentProtoInstance].definedProtoName,  \
				PROTONames[currentProtoInstance].charLen,rs); \
		} 



	#define FIND_THE_END_OF_IS(mystr) \
		/* printf ("FIND_THE_END_OF_IS, input str :%s:\n",mystr); */ \
		endIS = strstr(mystr,strNOTIS); \
		/* printf ("	FIND_THE_END_OF_IS: endIS string is :%s:\n",endIS); */ \
		if (endIS == NULL) { \
			ConsoleMessage ("did not find an </IS> for ProtoInstance %s\n",PROTONames[currentProtoInstance].definedProtoName); \
			FREE_IF_NZ(protoInString); \
			return; \
		} \
		endIS += strlen(strNOTIS); 

	#define FIND_THE_IS \
		 IS = strstr(curProtoPtr,strIS); \
		/* printf ("	FIND_THE_IS: IS string is :%s:\n",IS); */ 

	#define ZERO_IS_TEXT_IN_ORIG \
		{ char *is; is = IS; \
		while ((is != endIS) && (*is != '\0')) { *is = ' '; is++; }}

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

	#define MAKE_PROTO_COPY_FIELDS \
	myObj = PROTONames[currentProtoInstance].fieldDefs; \
	fdl += fprintf (fileDescriptor, "<!--\nProtoInterface fields has %d fields -->\n",vector_size(myObj->fields)); \
	for (ind=0; ind<vector_size(myObj->fields); ind++) { \
		int i; struct ScriptFieldDecl* field; char *fv; \
		field = vector_get(struct ScriptFieldDecl*, myObj->fields, ind); \
		fv = field->ASCIIvalue; /* pointer to ProtoDef value - might be replaced in loop below */ \
		for (i=0; i<CPI.paircount; i++) { \
			/* printf ("CPI has %s and %s\n",CPI.name[i],CPI.value[i]); */ \
			if (strcmp(CPI.name[i],field->ASCIIname)==0) {/* use the value passed in on invocation */ fv=(char *)CPI.value[i];} \
		} \
		if (field->fieldDecl->mode != PKW_initializeOnly) {\
		if (fv != NULL) { \
		fdl += fprintf (fileDescriptor,"\t<Metadata%s DEF='%s_%s_%d' value='%s'/>\n", \
			field->ASCIItype, \
			field->ASCIIname,  \
			FREEWRL_SPECIFIC,  \
			CPI.uniqueNumber, \
			fv); \
		} else { \
		fdl += fprintf (fileDescriptor,"\t<Metadata%s DEF='%s_%s_%d' />\n", \
			field->ASCIItype, \
			field->ASCIIname,  \
			FREEWRL_SPECIFIC,  \
			CPI.uniqueNumber); \
		}} \
	} \
	fdl += fprintf (fileDescriptor, "<!-- end of MAKE_PROTO_COPY_FIELDS --> \n");


/* have a </ProtoInstance> so should have valid name and fieldValues */
void expandProtoInstance(struct VRMLLexer *myLexer, struct X3D_Group *myGroup) {
	int i;
	char *protoInString;
	int psSize;
	int rs;
	char *IS;
	char *endIS;
	int pf;
	char *curProtoPtr;
	struct Shader_Script *myObj;
	indexT ind;
	char *tmpf;
	FILE *fileDescriptor;
	int fdl;
	char uniqueIDstring[20];


	/* initialization */
	IS = NULL;
	endIS = NULL;
	curProtoPtr=  NULL;
	myObj = NULL;
	tmpf = tempnam("/tmp","freewrl_proto");
	fdl = 0;

	/* we tie a unique number to the currentProto */
	CPI.uniqueNumber = newProtoDefinitionPointer(NULL,currentProtoInstance);

printf ("expandProtoInstance, CPI.uniqueNumber %d, curProtoStackInd %d\n",CPI.uniqueNumber,curProtoInsStackInd);


	/* first, do we actually have a valid proto here? */
	if (currentProtoInstance == INT_ID_UNDEFINED) 
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
#ifdef OLDCODE
	sprintf (uniqueIDstring,"%d",currentProtoInstance);
#endif
	sprintf (uniqueIDstring,"%d",CPI.uniqueNumber);

	/* step 0. Does this one contain a DEF? */
	if (ProtoInstanceTable[curProtoInsStackInd].defName != NULL) {
		struct X3D_Node * me; 
		#ifdef X3DPARSERVERBOSE
			printf ("ProtoInstance, have a DEF, defining :%s: for node %u\n",
			ProtoInstanceTable[curProtoInsStackInd].defName,(unsigned int) myGroup);
		#endif

		me = DEFNameIndex(ProtoInstanceTable[curProtoInsStackInd].defName,X3D_NODE(myGroup),FALSE);
		FREE_IF_NZ(ProtoInstanceTable[curProtoInsStackInd].defName);
	}

	/* step 1. read in the PROTO text. */
/* #ifndef WIN32  */
	/* I don't see why malloc twice - looks like leak */
	psSize = PROTONames[currentProtoInstance].charLen * 10;

	if (psSize < 0) {
		ConsoleMessage ("problem with psSize in expandProtoInstance");
		return;
	}

	protoInString = MALLOC(PROTONames[currentProtoInstance].charLen+1);
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

	MAKE_PROTO_COPY_FIELDS

	/* dump the modified string down here */
	fdl += fprintf(fileDescriptor,"%s",curProtoPtr);
	fdl += fprintf (fileDescriptor, "</Group></Scene></X3D>\n");


	fclose(fileDescriptor);
	fileDescriptor = fopen (tmpf,"r");
	if (fileDescriptor == NULL) {
		printf ("wierd problem opening proto expansion file\n"); 
		return;
	}

	protoInString = MALLOC(fdl+1);
	fread(protoInString, 1, fdl, fileDescriptor);
	protoInString[fdl] = '\0';


	#ifdef X3DPARSERVERBOSE
	printf ("PROTO EXPANSION IS:\n%s\n:\n",protoInString);
	#endif

	/* parse this string */
	if (X3DParse (myGroup,protoInString)) {
		#ifdef X3DPARSERVERBOSE
		printf ("PARSED OK\n");
		#endif
		if (ProtoInstanceTable[curProtoInsStackInd].container == INT_ID_UNDEFINED) 
			pf = FIELDNAMES_children;
		else
			pf = ProtoInstanceTable[curProtoInsStackInd].container;
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
	for (i=0; i<ProtoInstanceTable[curProtoInsStackInd].paircount; i++) {
		FREE_IF_NZ (ProtoInstanceTable[curProtoInsStackInd].name[i]);
		FREE_IF_NZ (ProtoInstanceTable[curProtoInsStackInd].value[i]);
	}
	ProtoInstanceTable[curProtoInsStackInd].paircount = 0;

	#ifdef X3DPARSERVERBOSE
	printf ("expandProtoInstance: decrementing curProtoInsStackInd from %d\n",curProtoInsStackInd);
	#endif

	linkNodeIn(__FILE__,__LINE__); 
	DECREMENT_PARENTINDEX
	curProtoInsStackInd--;
        FREE_IF_NZ(protoInString);



	/* ok, now, what happens is that we have Group->Group; and the second group has all the interesting stuff relating
	   to the PROTO expansion. Lets move this info to the parent Group. */
	{
		struct X3D_Group *par;
		struct X3D_Group *chi;
		int ind;

		par = myGroup;

		if (par->children.n==1) {
			chi = X3D_GROUP(par->children.p[0]);

			/* unlink the child node */
			AddRemoveChildren(X3D_NODE(par), 
				offsetPointer_deref(struct Multi_Node *,par,offsetof(struct X3D_Group,children)),
					(uintptr_t*) &chi,
					1,2,__FILE__,__LINE__);


			/* copy the original children from the chi node, to the par node */
			while (chi->children.n>0) {
				struct X3D_Node *offspring;

				/* printf ("now, parent has %d children...\n",par->children.n);
				printf ("and chi has %d children\n",chi->children.n);
				*/

				offspring = X3D_NODE(chi->children.p[0]);
				/* printf ("offspring %d is %u, type %s\n",ind,offspring,stringNodeType(offspring->_nodeType)); */

				AddRemoveChildren(chi,offsetPointer_deref(struct Multi_Node *,chi,offsetof(struct X3D_Group,children)),
					(uintptr_t*) &offspring, 1, 2, __FILE__, __LINE__);

				AddRemoveChildren(par,offsetPointer_deref(struct Multi_Node *,par,offsetof(struct X3D_Group,children)),
					(uintptr_t*) &offspring, 1, 1, __FILE__, __LINE__);

				/* 
				printf ("offspring type is still %s\n",stringNodeType(offspring->_nodeType));
				printf ("moved offspring has %d parents \n",offspring->_nparents);
				printf ("and it is %u and parent is %u and chi is %u\n",
					offspring->_parents[0],par,chi);
				*/
			}
			

			/* copy the protoDef pointer over */
			par->FreeWRL__protoDef = chi->FreeWRL__protoDef;

			/* move the FreeWRL_PROTOInterfaceNodes nodes to the parent */
			/* this is the same code as for the "children" field, above */
			while (chi->FreeWRL_PROTOInterfaceNodes.n>0) {
				struct X3D_Node *offspring;
				offspring = X3D_NODE(chi->FreeWRL_PROTOInterfaceNodes.p[0]);

				AddRemoveChildren(chi,offsetPointer_deref(struct Multi_Node *,chi,offsetof(struct X3D_Group,FreeWRL_PROTOInterfaceNodes)),
					(uintptr_t*) &offspring, 1, 2, __FILE__, __LINE__);

				AddRemoveChildren(par,offsetPointer_deref(struct Multi_Node *,par,offsetof(struct X3D_Group,FreeWRL_PROTOInterfaceNodes)),
					(uintptr_t*) &offspring, 1, 1, __FILE__, __LINE__);
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

	/* printf ("end of expandProtoInstance\n"); */
}

void parseProtoBody (const char **atts) {
	#ifdef X3DPARSERVERBOSE
	TTY_SPACE
	printf ("start of parseProtoBody\n");
	#endif

	setParserMode(PARSING_PROTOBODY);
}

void parseProtoDeclare (const char **atts) {
	int count;
	int nameIndex;

	/* initialization */
	nameIndex = INT_ID_UNDEFINED;

	/* increment the currentProtoDeclare field. Check to see how many PROTOS we (bounds check) */
	currentProtoDeclare++;
	curProDecStackInd++;

	setParserMode(PARSING_PROTODECLARE);
	
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
		registerProto(atts[nameIndex]);
	} else {
		ConsoleMessage ("\"ProtoDeclare\" found, but field \"name\" not found!\n");
	}
}


/* simple sanity check, and change mode */
void parseProtoInterface (const char **atts) {
	if (getParserMode() != PARSING_PROTODECLARE) {
		ConsoleMessage ("got a <ProtoInterface>, but not within a <ProtoDeclare>\n");
	}
	setParserMode(PARSING_PROTOINTERFACE);
}


/* parse a script or proto field. Note that they are in essence the same, just used differently */
void parseScriptProtoField(struct VRMLLexer* myLexer, const char **atts) {
	int i;
	uintptr_t myScriptNumber;
	int myparams[MPFIELDS];
	int which;
	int myFieldNumber;
	const char *myValueString;
	int myAccessType;
	struct Shader_Script *myObj;
	struct ScriptFieldDecl* sdecl;
	indexT name;
	union anyVrml defaultVal;

	/* initialization */
	myScriptNumber = 0;
	myValueString = NULL;
	myObj = NULL;
	sdecl = NULL;
	name = ID_UNDEFINED;

	#ifdef X3DPARSERVERBOSE
	printf ("start of parseScriptProtoField\n");
	#endif

	/* configure internal variables, and check sanity for top of stack This should be a Script node */
	if (getParserMode() == PARSING_SCRIPT) {
		#ifdef X3DPARSERVERBOSE
		printf ("verifying: parseScriptProtoField, expecting a scriptish, got a %s\n",
			stringNodeType(parentStack[parentIndex]->_nodeType));
		#endif

		switch (parentStack[parentIndex]->_nodeType) {
			case NODE_Script: {
				struct X3D_Script* myScr = NULL;
				myScr = X3D_SCRIPT(parentStack[parentIndex]);
				myObj = (struct Shader_Script *) myScr->__scriptObj;
				myScriptNumber = myObj->num;
				break; }
			case NODE_ComposedShader: {
				struct X3D_ComposedShader* myScr = NULL;
				myScr = X3D_COMPOSEDSHADER(parentStack[parentIndex]);
				myObj = (struct Shader_Script *) myScr->__shaderObj;
				break; }
			case NODE_ShaderProgram: {
				struct X3D_ShaderProgram* myScr = NULL;
				myScr = X3D_SHADERPROGRAM(parentStack[parentIndex]);
				myObj = (struct Shader_Script *) myScr->__shaderObj;
				break; }
			case NODE_PackagedShader: {
				struct X3D_PackagedShader* myScr = NULL;
				myScr = X3D_PACKAGEDSHADER(parentStack[parentIndex]);
				myObj = (struct Shader_Script *) myScr->__shaderObj;
				break; }

			default: {

				ConsoleMessage("got an error on parseScriptProtoField, do not know how to handle a %s",
					stringNodeType(parentStack[parentIndex]->_nodeType));
				return;
			}
		}
	} else if ((getParserMode() == PARSING_PROTOINTERFACE) || (getParserMode() == PARSING_NODES)){
		#ifdef X3DPARSERVERBOSE
		printf ("start of parseScriptProtoField, parserMode %s\n",parserModeStrings[getParserMode()]);
		#endif

		myScriptNumber = currentProtoDeclare;
		myObj = PROTONames[currentProtoDeclare].fieldDefs;

		#ifdef X3DPARSERVERBOSE
		TTY_SPACE
		printf ("parseScriptProtoField, working on proto idNumber %d\n",myScriptNumber);
		#endif
	} else {
		printf ("there seems to be an error here, not expecting mode to be %s\n",
			parserModeStrings[getParserMode()]);
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
		if ((strcmp("appinfo", atts[i]) != 0)  ||
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


	/* convert eventIn, eventOut, field, and exposedField to new names */
	myAccessType = findFieldInPROTOKEYWORDS(atts[myparams[MP_ACCESSTYPE]]);
	switch (myAccessType) {
		case PKW_eventIn: myAccessType = PKW_inputOnly; break;
		case PKW_eventOut: myAccessType = PKW_outputOnly; break;
		case PKW_exposedField: myAccessType = PKW_inputOutput; break;
		case PKW_field: myAccessType = PKW_initializeOnly; break;
		default: {}
	}
	
	/* printf ("field parsing, so we have script %d, accessType %d fieldnumber %d, need to get field value\n",
		myScriptNumber, myAccessType, myFieldNumber); */

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

	/* so, inputOnlys and outputOnlys DO NOT have initialValues, 
	   inputOutput and initializeOnly DO */
	if ((myAccessType == PKW_initializeOnly) || (myAccessType == PKW_inputOutput)) {
		if (myValueString == NULL) {
			ConsoleMessage ("Field, an initializeOnly or inputOut needs an initialValue");
			return;
		}
	} else {
		if (myValueString != NULL) {
			ConsoleMessage ("Field, an inputOnly or outputOnly can not have an initialValue");
			return;
		}
	}
				

	/* create a new scriptFieldDecl */
	sdecl = newScriptFieldDecl(myLexer,(indexT) myAccessType, 
		findFieldInFIELDTYPES(atts[myparams[MP_TYPE]]),  name);
#ifdef X3DPARSERVERBOSE
	printf ("created a new script field declaration, it is %u\n",sdecl);
#endif


	/* for now, set the value  -either the default, or not... */
	if (myValueString != NULL) {
		Parser_scanStringValueToMem(X3D_NODE(&defaultVal), 0, sdecl->fieldDecl->type, (char *)myValueString, TRUE);
	}
	scriptFieldDecl_setFieldValue(sdecl, defaultVal);

		
	/* fill in the string name and type */
	sdecl->ASCIIname=STRDUP(atts[myparams[MP_NAME]]);
	sdecl->ASCIItype=STRDUP(atts[myparams[MP_TYPE]]);

	/* if we are parsing a PROTO interface, we might as well save the value as a string, because we will need it later */
	if (getParserMode() == PARSING_PROTOINTERFACE) {
		if (myValueString != NULL)
			scriptFieldDecl_setFieldASCIIValue(sdecl, STRDUP(myValueString));
	}

	/* add this field to the script */
	script_addField(myObj,sdecl);
}

/* we get script text from a number of sources; from the URL field, from CDATA, from a file
   pointed to by the URL field, etc... handle the data in one place */

void initScriptWithScript() {
	uintptr_t myScriptNumber;
	struct X3D_Script * me;
	char *myText;
	struct Shader_Script *myObj;

	/* initialization */
	myText = NULL;

	/* semantic checking... */
	me = X3D_SCRIPT(parentStack[parentIndex]);
	myObj = (struct Shader_Script *) me->__scriptObj;

	if (me->_nodeType != NODE_Script) {
		ConsoleMessage ("initScriptWithScript - Expected to find a NODE_Script, got a %s\n",
		stringNodeType(me->_nodeType));
		return;
	}

	#ifdef X3DPARSERVERBOSE
	printf ("endElement: CDATA_Text is %s\n",CDATA_Text);
	#endif

	myScriptNumber = myObj->num;

	/* did the script text come from a CDATA node?? */
	if (CDATA_Text != NULL) if (CDATA_Text[0] != '\0') myText = CDATA_Text;


	/* is this CDATA text? */
	if (myText != NULL) {
		struct Multi_String strHolder;

		strHolder.p = MALLOC (sizeof(struct Uni_String)*1);
		strHolder.p[0] = newASCIIString(myText);
		strHolder.n=1; 
		script_initCodeFromMFUri(myObj, &strHolder);
		FREE_IF_NZ(strHolder.p[0]->strptr);
		FREE_IF_NZ(strHolder.p);
	} else {
		script_initCodeFromMFUri(myObj, &X3D_SCRIPT(me)->url);
	}

	/* finish up here; if we used the CDATA area, set its length to zero */
	if (myText != NULL) CDATA_Text_curlen=0;

	setParserMode(PARSING_NODES);
	#ifdef X3DPARSERVERBOSE
	printf ("endElement: got END of script - script should be registered\n");
	#endif
}

void addToProtoCode(const char *name) {
        if (PROTONames[currentProtoDeclare].fileOpen) 
                PROTONames[currentProtoDeclare].charLen += fprintf (PROTONames[currentProtoDeclare].fileDescriptor,"</%s>\n",name);
}

#ifdef OLDCODE
#endif

void endProtoDeclare(void) {

		#ifdef X3DPARSERVERBOSE
		TTY_SPACE
		printf ("endElement, end of ProtoDeclare %d stack %d\n",currentProtoDeclare,curProDecStackInd);
		#endif

		/* decrement the protoDeclare stack count. If we are nested, get out of the nesting */
		curProDecStackInd--;

		/* now, a ProtoDeclare should be within normal nodes; make the expected mode PARSING_NODES, assuming
		   we don't have nested ProtoDeclares  */
		if (curProDecStackInd == 0) setParserMode(PARSING_NODES);

		if (curProDecStackInd < 0) {
			ConsoleMessage ("X3D_Parser found too many end ProtoDeclares at line %d\n",LINE);
			curProDecStackInd = 0; /* reset it */
		}
}

/* we are doing a ProtoInstance, and we do not have a fieldValue in the Instance for a parameter. See if it is
   available in the ProtoInterface. */
static int getFieldValueFromProtoInterface (struct VRMLLexer *myLexer, char *fieldName, int protono, char **value) {
	struct ScriptFieldDecl* myField;
	const char** userArr;
	size_t userCnt;
	indexT retUO;
	struct Shader_Script* myObj;

	/* initialization */
	myField = NULL;
	retUO = ID_UNDEFINED;
	myObj = NULL;



	#ifdef X3DPARSERVERBOSE
	printf ("getFieldValueFromProtoInterface, lexer %u looking for :%s: in proto %d\n",myLexer, fieldName, protono);
	#endif
	
	myObj = PROTONames[protono].fieldDefs;

        
#define LOOK_FOR_FIELD_IN(whicharr) \
        if (myField == NULL) { \
        userArr=&vector_get(const char*, myLexer->user_##whicharr, 0); \
        userCnt=vector_size(myLexer->user_##whicharr);\
        retUO=findFieldInARR(fieldName, userArr, userCnt);\
        if (retUO != ID_UNDEFINED) { \
                myField=script_getField(myObj,retUO,PKW_##whicharr); \
        }} 
                
        LOOK_FOR_FIELD_IN(initializeOnly);
        LOOK_FOR_FIELD_IN(inputOnly);
        LOOK_FOR_FIELD_IN(outputOnly);
        LOOK_FOR_FIELD_IN(inputOutput);
        
#undef LOOK_FOR_FIELD_IN

	if (myField != NULL) {
		*value = (char *)myField->ASCIIvalue;
		return TRUE;
	}


	/* did not find it. */
	*value = "";
	return FALSE;
}




/* look through the script fields for this field, and return the values. */
int getFieldFromScript (struct VRMLLexer *myLexer, char *fieldName, struct Shader_Script *me, int *offs, int *type, int *accessType) {
	struct ScriptFieldDecl* myField;
	const char** userArr;
	size_t userCnt;
	indexT retUO;

	/* initialize */
	myField = NULL;
	retUO = ID_UNDEFINED;

#define X3DPARSERVERBOSE

	#ifdef X3DPARSERVERBOSE
	printf ("getFieldFromScript, looking for %s\n",fieldName);
	#endif

	/* go through the user arrays in this lexer, and see if we have a match */


	#ifdef CPROTOVERBOSE
	printf ("getProtoFieldDeclaration, for field :%s:\n",fieldName);
	#endif


#define LOOK_FOR_FIELD_IN(whicharr) \
	if (myField == NULL) { \
	userArr=&vector_get(const char*, myLexer->user_##whicharr, 0); \
	userCnt=vector_size(myLexer->user_##whicharr);\
	retUO=findFieldInARR(fieldName, userArr, userCnt);\
	if (retUO != ID_UNDEFINED) { \
		myField=script_getField(me,retUO,PKW_##whicharr); \
	}}

	LOOK_FOR_FIELD_IN(initializeOnly);
	LOOK_FOR_FIELD_IN(inputOnly);
	LOOK_FOR_FIELD_IN(outputOnly);
	LOOK_FOR_FIELD_IN(inputOutput);


	if (myField != NULL) {
		int myFieldNumber;

		/* is this a script? if so, lets do the conversion from our internal lexer name index to
		   the scripting name index. */
		if (me->ShaderScriptNode->_nodeType == NODE_Script) {
			/* wow - have to get the Javascript text string index from this one */
			myFieldNumber = JSparamIndex(fieldName,stringFieldtypeType(myField->fieldDecl->type)); 
			*offs=myFieldNumber;


		} else {
			*offs = myField->fieldDecl->name;
		}
		*type = myField->fieldDecl->type;
		/* go from PKW_xxx to KW_xxx  .... sigh ... */
		*accessType = mapToKEYWORDindex(myField->fieldDecl->mode);
		return TRUE;
	}

	#ifdef X3DPARSERVERBOSE
	printf ("getFieldFromScript, did not find field %s in script\n",fieldName);
	#endif
	
	/* did not find it */
	*offs = INT_ID_UNDEFINED;  *type = 0;
	return FALSE;
}

