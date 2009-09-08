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
static int getFieldValueFromProtoInterface (struct VRMLLexer *myLexer, char *fieldName, int protono, char **value);
static int getFieldAccessMethodFromProtoInterface (struct VRMLLexer *myLexer, char *fieldName, int protono);

/* for parsing fields in PROTO expansions */
/* FIELD_END is an ascii string that will pass the XML parser, but should not be found in a field value */
#define FIELD_END "\t \t\n" 
#define strIS "<IS>"
#define strNOTIS "</IS>"
#define strCONNECT  "<connect"
#define strNODEFIELD "nodeField"
#define strPROTOFIELD "protoField"
#define MAX_ID_SIZE 1000
#define NODEFIELD_EQUALS "nodeField=\""
#define PROTOFIELD_EQUALS "protoField=\""

/* for parsing script initial fields */
#define MPFIELDS 4
#define MP_NAME 0
#define MP_ACCESSTYPE 1
#define MP_TYPE 2
#define MP_VALUE 3

#define FREEWRL_SPECIFIC "FrEEWrL_pRotto"
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


/* find a value for the proto field on invocation. First look at the ProtoInstance, if not there, then look
   at the ProtoDeclare for the field. */
static char *getProtoValue(struct VRMLLexer *myLexer, int ProtoInvoc, char *id) {
	char *retptr;
	int i;

	#ifdef X3DPARSERVERBOSE
	printf ("getProtoValue for proto %d, char :%s:\n",ProtoInvoc, id);
	#endif

	/* get the start/end value pairs, and copy them into the id field. */
	if ((curProtoInsStackInd < 0) || (curProtoInsStackInd >= PROTOINSTANCE_MAX_LEVELS)) {
		return "";
	} else {
		/* is this to be matched in the ProtoInstance fields? */
		for (i=0; i<ProtoInstanceTable[curProtoInsStackInd].paircount; i++) {
			#ifdef X3DPARSERVERBOSE
				printf (" 	getProtoValue - ProtoInstance - comparing ind %d, :%s:  :%s:\n",
					i,id,ProtoInstanceTable[curProtoInsStackInd].name[i]);
			#endif
			if (strcmp(id,ProtoInstanceTable[curProtoInsStackInd].name[i]) == 0) {
				/* printf ("getProtoValue, found name!\n"); */
				return ProtoInstanceTable[curProtoInsStackInd].value[i];
			}
		}

		/* no, maybe the field is in the ProtoInterface definitions? */
		#ifdef X3DPARSERVERBOSE
		printf ("have to look for id :%s: in ProtoInterface\n",id);
		#endif

		if (getFieldValueFromProtoInterface (myLexer, id, ProtoInvoc, &retptr)) {
			return retptr;
		}
	}

	/* if we are here, we did not find that parameter */
	ConsoleMessage ("ProtoInstance <%s>, could not find parameter <%s>",
			PROTONames[ProtoInvoc].definedProtoName, id);

	return "";
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

/* go back through this proto string, and find an associated DEF with a Script/Shader, ok? */
static void getDefFromScriptShader(char *protoInString, char *foundNameEquals, char *myDefName) {
	char *myn;

	myDefName[0] = 0;
	/* so, we have this proto, protoInString; and we have somewhere in it, a "name='field'", lets
	   go and find the associated DEF for this. */
	while (foundNameEquals != protoInString) {
		foundNameEquals--;
		if (*foundNameEquals == '<') {
			/* printf ("possible ShaderScript at :%s:\n",foundNameEquals); */
			if ((strncmp("<Script",foundNameEquals,strlen("<Script")) == 0) ||
			   (strncmp("<ComposedShader",foundNameEquals,strlen("<ComposedShader")) == 0) ||
			   (strncmp("<PackagedShader",foundNameEquals,strlen("<PackagedShader")) == 0) ||
			   (strncmp("<ShaderProgram",foundNameEquals,strlen("<ShaderProgram")) == 0)) {
				char tmp;
				char *cb;
				char *def;

				cb = strchr(foundNameEquals,'>');
				if (cb==NULL) return;  /* should always find a close brace, but... */
				tmp = *cb; *cb= '\0';  /* make this null for finding the DEF name... */

				def = strstr(foundNameEquals,"DEF=");
				if (def ==NULL) {*cb=tmp; return;}

				/* found a "DEF=", skip forward to start of DEF string */
				while ((*def != '\'') && (*def != '"') && (*def != '\0')) def++;

				/* copy the name over */
				if (*def != '\0') {
					def++;
					while ((*def != '\'') && (*def != '"') && (*def != '\0')) {
						*myDefName = *def;
						myDefName++; def++;
					}
					*myDefName = '\0';
				}

				/* restore this end marker */
				*cb = tmp;
			}
		}
	}
}

/* pushes onto a Vector a XML string for routing to a PROTO internal data structure, used for "fanning" i/o to/from a PROTO */
static void generateRoutes(char *myDefName, char *nodeFieldID, char *protoFieldID,int myFieldKind, struct Vector **routVec, int uniqueExpandedID){
	char *routeStr;
	int rsl;

	/* approximate string length for one route line */
	rsl = strlen(myDefName) + strlen(nodeFieldID) + strlen(protoFieldID) + 100;


	switch (myFieldKind) {
		case PKW_inputOnly: 
			routeStr = MALLOC(sizeof (char *) + rsl);
			/* 1 */sprintf (routeStr,"<ROUTE fromNode='%s_%s_%d' fromField='valueChanged' toNode='%s' toField='%s'/>\n",
					protoFieldID,FREEWRL_SPECIFIC,uniqueExpandedID,myDefName,nodeFieldID);
			vector_pushBack(char *, *routVec,routeStr);
			break;
		case PKW_outputOnly: 
			routeStr = MALLOC(sizeof (char *) + rsl);
			/* 2 */sprintf (routeStr,"<ROUTE toNode='%s_%s_%d' toField='setValue' fromNode='%s' fromField='%s'/>\n",
					protoFieldID,FREEWRL_SPECIFIC,uniqueExpandedID,myDefName,nodeFieldID);
			vector_pushBack(char *, *routVec,routeStr);
			break;
		case PKW_inputOutput: 
			routeStr = MALLOC(sizeof (char *) + rsl);
			/* 1 */sprintf (routeStr,"<ROUTE fromNode='%s_%s_%d' fromField='valueChanged' toNode='%s' toField='%s'/>\n",
					protoFieldID,FREEWRL_SPECIFIC,uniqueExpandedID,myDefName,nodeFieldID);
			vector_pushBack(char *, *routVec,routeStr);
			routeStr = MALLOC(sizeof (char *) + rsl);
			/* 2 */sprintf (routeStr,"<ROUTE toNode='%s_%s_%d' toField='setValue' fromNode='%s' fromField='%s'/>\n",
					protoFieldID,FREEWRL_SPECIFIC,uniqueExpandedID,myDefName,nodeFieldID);
			vector_pushBack(char *, *routVec,routeStr);
			break;
		case PKW_initializeOnly: 
			/* printf ("PKW_initializeOnly - do nothing\n"); */
			break;
		default :
			printf ("generateRoutes unknown proto type - ignoring\n");
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

/*******************************************************************************************/


/* for IS routing - get a DEF name, if one exists in this code snippet; if not, make one up for later inclusion */
static unsigned int uniqueDefNameNumber = 31567;
static void findThisDefName(int *hasDef, char *myDefName, char *openBrace) {
	char *foundDEF;
	char singleDoubleQuote;

	foundDEF = strstr (openBrace,"DEF=");
	if (foundDEF == NULL) {
#ifdef X3DPARSERVERBOSE
		printf ("findThisDefName - have to generate a DEF name\n");
#endif
		*hasDef = FALSE;
		/* just make up a name; use the current memptr for the DEF name */
		sprintf (myDefName,"FrEEWrL_pRot_%u",uniqueDefNameNumber);
		uniqueDefNameNumber++;
	} else {
#ifdef X3DPARSERVERBOSE
		printf ("findThisDefName - have a DEF in here :%s:\n",foundDEF);
#endif
		*hasDef = TRUE;
		while ((*foundDEF != '"') && (*foundDEF != '\'') && (*foundDEF != '\0')) foundDEF++;
		printf ("start of DEF now is :%s:\n",foundDEF);
		if (*foundDEF == '\0') return; /* hmmm - an error! */

		singleDoubleQuote = *foundDEF; foundDEF++;
		while ((*foundDEF != singleDoubleQuote) && (*foundDEF != '\0')) {
			*myDefName = *foundDEF;
			myDefName++; foundDEF++;
		}
		*myDefName = '\0';
	}


}


#define FIND_THE_CONNECT(mystr)  \
	connect = strstr(mystr,strCONNECT);  \
	if (connect != NULL) {  \
	 mystr = connect + strlen (strCONNECT); }

#define FIND_NODE_PROTO_FIELD(mystr,ns,ps) \
	ns = strstr(mystr, NODEFIELD_EQUALS); \
	ps = strstr(mystr, PROTOFIELD_EQUALS); \
	if (ns!=NULL) ns += strlen(NODEFIELD_EQUALS); \
	if (ps!=NULL) ps += strlen(PROTOFIELD_EQUALS);

#define FILL_ID(mystr,myid) \
	{ char *dq;   \
		myid[0]='\0'; \
		if (mystr!=NULL) { \
			dq = strchr(mystr,'"'); \
			if (dq != NULL) { \
				*dq = '\0'; \
				strcpy(myid,mystr); \
				*dq = '"'; \
			} \
		} \
	}


/* substitute the IS connects in the protoInString with the values from the IS, and return the string */
/* protoInString = original string, with the <IS> copied, and spaced out */
/* IS - just a pointer into protoInString of where the IS was, before it was spaced out */
/* isString - the IS string that we have to substitute for */

static char* doISsubs(struct VRMLLexer *myLexer, char *protoInString, char *IS, char *isString,struct Vector ** routVec,int uniqueExpandedID) {
	char *connect;
	char *ns;
	char *ps;
	char nodeFieldID[200];
	char protoFieldID[200];
	char tmp[200];
	char *newProtoInString;
	char *closeBrace;
	char *openBrace;
	char ctmp;

	/* for finding IS node DEF name */
	int hasDef;
	char myDefName[255];


	/* initialization */
	connect = NULL;
	ns = NULL;
	ps = NULL;
	closeBrace = NULL;
	openBrace = NULL;

	#ifdef X3DPARSERVERBOSE
		printf ("\nstart of doISsubs\n");
		printf ("doISsubs, orig :%s:\n",protoInString);
		printf ("IS pointer is at:%s:\n",IS);
		printf ("and the isString is :%s:\n",isString);
	#endif

	/* find a <connect>, and go through all <connect> strings until we have finished with this IS */
	/* win32 comment: could this kind of parsing be done more reliably with xml parser rather than strstr? 
	   usually that's what the xml doms and parsers are for.
	*/
	FIND_THE_CONNECT(isString)
	while (connect != NULL) {
		char *foundNameEquals = NULL;

		#ifdef X3DPARSERVERBOSE
			printf ("doISsubs loop, find the connect:%s:\n",connect);
		#endif

		FIND_NODE_PROTO_FIELD(connect,ns,ps)
		FILL_ID(ns,nodeFieldID)
		FILL_ID(ps,protoFieldID)

		#ifdef X3DPARSERVERBOSE
			printf ("nodeFieldID :%s:  protoFieldID :%s:\n",nodeFieldID, protoFieldID);
		#endif

		/* What do we attach the nodeFieldID to?? lets go back and see what we can find */
		/* could this be a Script field, like:
			<field accessType="initializeOnly" name="imageName"> </field> ? */

		/* first, try it with double quotes */
		strcpy (tmp,"name=\""); strcat (tmp,nodeFieldID); strcat (tmp,"\"");

		#ifdef X3DPARSERVERBOSE
			printf ("trying to find the following :%s:\n",tmp);
		#endif

		foundNameEquals = strstr(protoInString,tmp);
		if (foundNameEquals == NULL) {
			/* try it with single quotes */
			strcpy (tmp,"name='"); strcat (tmp,nodeFieldID); strcat (tmp,"'");

			#ifdef X3DPARSERVERBOSE
				printf ("now trying to find the following :%s:\n",tmp);
			#endif

			foundNameEquals = strstr(protoInString,tmp);
		}

		/* ok, did we find the name=value; if so, this is part of a script */		
		if (foundNameEquals != NULL) {
#ifdef X3DPARSERVERBOSE
			printf ("found this at offset %d probably script field\n",(int)foundNameEquals - (int)protoInString);
			printf ("ok, probably in a Script, lets see if we can find a start of Script, and go nuts with this.\n");
			printf ("nodeFieldID :%s:  protoFieldID :%s:\n",nodeFieldID, protoFieldID);
#endif

			getDefFromScriptShader(protoInString,foundNameEquals,myDefName);
#ifdef X3DPARSERVERBOSE
printf ("well, myDefName should be :%s:\n",myDefName);
#endif

			/* lets find the script name, and save this for our use */
			if (myDefName[0] == '\0') {
				ConsoleMessage ("XML PROTO with Script/Shader with <IS>; no DEF Name, can not get handle for it");
			} else {
				generateRoutes(myDefName, nodeFieldID, protoFieldID,getProtoKind(myLexer,currentProtoInstance,protoFieldID),routVec,
					uniqueExpandedID);
			}


		} else {
			#ifdef X3DPARSERVERBOSE	
			printf ("did not find it, lets go back and find node to attach to\n"); 
			#endif

			/* we probably have something like this: 
			<ImageTexture>
                        	<IS>
                        		<connect nodeField='url' protoField='imageName'/>
					<connect nodeField='repeatS' protoField='yes'/>
                        	</IS>
                	</ImageTexture>

			which we want to turn into something like:
			<ImageTexture url=(substituted string) repeatS=(substituted string) />

			*/

			/* so, to do that, we have to take off the trailing ">" from the node name */
			ctmp = *IS; *IS='\0'; /* set current pointer to null */
			openBrace = strrchr(protoInString,'<');
			closeBrace = strrchr(protoInString,'>');

			/* so, hopefully we have a string "<ImageTexture" in openBrace. Do we need to define
			   a DEF name so that we can route to/from this? Can we get the current DEF name if
			   it exists? ie, if we have "<ImageTexture DEF='myTexture'" lets find "myTexture",
			   if not, lets create a name for us. */
			hasDef = FALSE;
			myDefName[0] = '\0';
			findThisDefName(&hasDef, myDefName, openBrace);
			printf ("so, found DEF name is :%s:\n",myDefName); 

			*IS = ctmp; /* restore value */


		
			if (closeBrace != NULL) {
				char *valueStr;
				valueStr = NULL;

				/* ok, we know the location of the last ">", lets go and make a string with all
				   of this in it together */
				generateRoutes(myDefName, nodeFieldID, protoFieldID,getProtoKind(myLexer,currentProtoInstance,protoFieldID),routVec,
					uniqueExpandedID);

				/* the value of the PROTO substitution field: */
				valueStr = getProtoValue(myLexer, currentProtoInstance,protoFieldID);

				/* lets make up a new string long enough for the proto substitution */
				newProtoInString = MALLOC(strlen(valueStr) + strlen(protoInString) + strlen (myDefName) + 20);

				*closeBrace = '\0';
				strcpy (newProtoInString,protoInString);
				strcat (newProtoInString, " ");

				/* do we have to generate a DEF for this node? */
				if (!hasDef) {
					hasDef = TRUE; /* do this once only */
					strcat (newProtoInString,"DEF='");
					strcat (newProtoInString,myDefName);
					strcat (newProtoInString,"' ");
				}
				
				strcat (newProtoInString,nodeFieldID);
				strcat (newProtoInString,"='");

				/* now, is this possibly an MFString? */
				strcat (newProtoInString,valueStr); /* value from proto inv */

				strcat (newProtoInString,"'> ");
				/* any more additions go in here, so... */
				IS = newProtoInString + strlen(newProtoInString);

				*closeBrace=' ';
				strcat (newProtoInString, closeBrace);
				FREE_IF_NZ(protoInString);
				protoInString = newProtoInString;
			}
				
		}
		

		/* lets loop and do the next <connect> */
		connect += strlen (strCONNECT);
		FIND_THE_CONNECT(connect)

	}


	#ifdef X3DPARSERVERBOSE
		printf ("RETURNING :%s:\n",protoInString);
	#endif

	return protoInString;
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
		fdl += fprintf (fileDescriptor, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<X3D><Scene><Group FreeWRL__protoDef='%d'> <!-- INITIATE SCENE -->\n",uniqueExpandedID); \
	}

	#define MAKE_PROTO_COPY_FIELDS \
	myObj = PROTONames[currentProtoInstance].fieldDefs; \
	fdl += fprintf (fileDescriptor, "<!--\nProtoInterface fields has %d fields -->\n",vector_size(myObj->fields)); \
	for (ind=0; ind<vector_size(myObj->fields); ind++) { \
		struct ScriptFieldDecl* field = vector_get(struct ScriptFieldDecl*, myObj->fields, ind); \
		if (field->fieldDecl->mode != PKW_initializeOnly) {\
		if (field->ASCIIvalue != NULL) { \
		fdl += fprintf (fileDescriptor,"\t<Metadata%s DEF='%s_%s_%d' value='%s'/>\n", \
			field->ASCIItype, \
			field->ASCIIname,  \
			FREEWRL_SPECIFIC,  \
			uniqueExpandedID, \
			field->ASCIIvalue); \
		} else { \
		fdl += fprintf (fileDescriptor,"\t<Metadata%s DEF='%s_%s_%d' />\n", \
			field->ASCIItype, \
			field->ASCIIname,  \
			FREEWRL_SPECIFIC,  \
			uniqueExpandedID); \
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
	struct Vector *protoInternalRouting;
	int uniqueExpandedID;


	/* initialization */
	IS = NULL;
	endIS = NULL;
	curProtoPtr=  NULL;
	myObj = NULL;
	tmpf = tempnam("/tmp","freewrl_proto");
	fdl = 0;
	protoInternalRouting = newVector(char *, 16);

	/* we tie a unique number to the currentProto */
	uniqueExpandedID = newProtoDefinitionPointer(NULL,currentProtoInstance);


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
	sprintf (uniqueIDstring,"%d",currentProtoInstance);

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

	/* make that group for routing to/from this proto invocation */
	MAKE_PROTO_COPY_FIELDS

	/* loop through, and replace any IS'd fields with our PROTO expansion stuff... */
	FIND_THE_IS

	while (IS != NULL) {
		char *isString;
		unsigned int islen;

		FIND_THE_END_OF_IS(IS)

		if (endIS == NULL) {
			ConsoleMessage ("did not find </IS> in PROTO expansion");
			return;
		}

		/* copy the IS text over */
		islen =  (endIS - IS);
		isString = MALLOC(islen + 10);
		memcpy (isString,IS,islen);
		isString[islen] = '\0';
		
		#ifdef X3DPARSERVERBOSE
			printf ("is text is :%s:\n",isString);
		#endif

		/* zero the original data between the IS and the ENDIS */
		ZERO_IS_TEXT_IN_ORIG

		/* do IS substitutions */
		curProtoPtr = doISsubs(myLexer, curProtoPtr,IS,isString, &protoInternalRouting,uniqueExpandedID);

		/* and keep doing this, until we have not more <IS> fields */
		FIND_THE_IS		
	}

	/* dump the modified string down here */
	fdl += fprintf(fileDescriptor,"%s",curProtoPtr);

	/* ROUTES and final scene */
	fdl += fprintf (fileDescriptor,"<!-- Internal Metadata routes go here -->\n");
	for (ind=0 ;ind<vector_size(protoInternalRouting); ind++) {
		char *str = vector_get(char *,protoInternalRouting,ind);
		/* printf ("dumping routing :%s:\n",str); */
		if (str != NULL) {
			fdl += fprintf (fileDescriptor, "%s\n",str);
			FREE_IF_NZ(str);
		}
	}
	FREE_IF_NZ(protoInternalRouting);

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

	linkNodeIn(); 
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
	} else if (getParserMode() == PARSING_PROTOINTERFACE) {
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
		printf ("there seems to be an error here, expecting PARSING_SCRIPT or PARSING_PROTOINTERFACE, got %s\n",
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

/* look through the script fields for this field, and return the values. */
int getFieldFromScript (struct VRMLLexer *myLexer, char *fieldName, struct Shader_Script *me, int *offs, int *type, int *accessType) {
	struct ScriptFieldDecl* myField;
	const char** userArr;
	size_t userCnt;
	indexT retUO;

	/* initialize */
	myField = NULL;
	retUO = ID_UNDEFINED;

	#ifdef X3DPARSERVERBOSE
	printf ("getFieldFromScript, looking for %s\n",fieldName);
	#endif

	/* go through the user arrays in this lexer, and see if we have a match */


	#ifdef CPROTOVERBOSE
	printf ("getProtoFieldDeclaration, for field :%s:\n",thisID);
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
