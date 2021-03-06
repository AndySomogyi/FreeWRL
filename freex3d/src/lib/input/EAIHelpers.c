/*


Small routines to help with interfacing EAI to Daniel Kraft's parser.

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

#include "../x3d_parser/Bindable.h"
#include "../scenegraph/Collision.h"
#include "../scenegraph/quaternion.h"

#include "EAIHeaders.h"
#include "SensInterps.h"

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../scenegraph/Vector.h"
#include "../vrml_parser/CFieldDecls.h"
#include "../world_script/CScripts.h"
#include "../world_script/fieldSet.h"
#include "../world_script/fieldGet.h"
#include "../vrml_parser/CParseParser.h"
#include "../vrml_parser/CParseLexer.h"
#include "../vrml_parser/CParse.h"
#include "../vrml_parser/CProto.h"

#include "../x3d_parser/X3DParser.h"

#include "EAIHelpers.h"


/*

NOTE - originally, when perl parsing was used, there used to be a perl
NodeNumber, and a memory location. Now, there is only a memory location...

Interface functions, for PROTO field access. From an email from Daniel:

Access fields:
size_t protoDefiniton_getFieldCount(protoDef)
struct ProtoFieldDecl* protoDefinition_getFieldByNum(protoDef, index)

Properties of struct ProtoFieldDecl:
indexT protoFieldDecl_getType(fdecl)
indexT protoFieldDEcl_getAccessType(fdecl)

Get name:
indexT protoFieldDecl_getIndexName(fdecl)
const char* protoFieldDecl_getStringName(lexer, fdecl)

Default value:
union anyVrml protoFieldDecl_getDefaultValue(fdecl)
this union contains fields for every specific type, see CParseGeneral.h for exact structure.

This struct contains both a node and an ofs field.

************************************************************************/

/* responses get sent to the connecting program. The outBuffer is the place where
   these commands are placed */

//char *outBuffer = NULL;
//int outBufferLen = 0;
//static struct Vector *EAINodeIndex = NULL;
typedef struct pEAIHelpers{
	struct Vector *EAINodeIndex;
} * ppEAIHelpers;
void *EAIHelpers_constructor()
{
	void *v = MALLOCV(sizeof(struct pEAIHelpers));
	memset(v,0,sizeof(struct pEAIHelpers));
	return v;
}
void EAIHelpers_init(struct tEAIHelpers* t){
	//public
	t->outBuffer = NULL;
	t->outBufferLen = 0;
	//private
	t->prv = EAIHelpers_constructor();
	{
		ppEAIHelpers p = (ppEAIHelpers)t->prv;
		p->EAINodeIndex = NULL;
	}
}




/************************************************************************************************/
/*												*/
/*			EAI NODE TABLE								*/
/*												*/
/************************************************************************************************/

/* store enough stuff in here so that each field can be read/written, no matter whether it is a 
   PROTO, SCRIPT, or regular node. */



struct EAINodeParams {
	struct X3D_Node* thisFieldNodePointer;	/* ok, if this is a PROTO expansion, points to actual node */
	int fieldOffset;
	int datalen;
	int typeString;
	int scripttype;
	char *invokedPROTOValue; 	/* proto field value on invocation (default, or supplied) */
};

struct EAINodeIndexStruct {
	struct X3D_Node*	actualNodePtr;
	int 			nodeType; /* EAI_NODETYPE_STANDARD =regular node, 
					     EAI_NODETYPE_PROTO = PROTO, 
					     EAI_NODETYPE_SCRIPT = script */
	struct Vector*		nodeParams;
};




/* get an actual memory pointer to field, assumes both node has passed ok check */
char *getEAIMemoryPointer (int node, int field) {
	char *memptr;
	struct EAINodeIndexStruct *me;
	struct EAINodeParams *myParam;
	ppEAIHelpers p = (ppEAIHelpers)gglobal()->EAIHelpers.prv;
	me = vector_get(struct EAINodeIndexStruct *, p->EAINodeIndex,node);
	if (me == NULL) {
		printf ("bad node in getEAIMemoryPointer\n");
		return NULL;
	}

	/* we CAN NOT do this with a script */
	if (me->nodeType == EAI_NODETYPE_SCRIPT) {
		ConsoleMessage ("EAI error - getting EAIMemoryPointer on a Script node");
		return NULL;
	}

	myParam = vector_get(struct EAINodeParams *, me->nodeParams, field);

	if (myParam == NULL) {
		printf ("bad field in getEAIMemoryPointer\n");
		return NULL;
	}


	/* use the memory address associated with this field, because this may have changed for proto invocations */
	memptr = (char *)myParam->thisFieldNodePointer;
	memptr += myParam->fieldOffset;
	/* printf ("getEAIMemoryPointer, nf %d:%d, node %u offset %d total %u\n",
		node,field,getEAINodeFromTable(node,field), myParam->fieldOffset, memptr);
	*/

	return memptr;
}

/* get the parameters during proto invocation. Might not ever have been ISd */
static char * getEAIInvokedValue(int node, int field) {
	/* make sure we are a PROTO */
	struct EAINodeIndexStruct *me;
	struct EAINodeParams *myParam;
	ppEAIHelpers p = (ppEAIHelpers)gglobal()->EAIHelpers.prv;

	me = vector_get(struct EAINodeIndexStruct *, p->EAINodeIndex,node);
	if (me == NULL) {
		printf ("bad node in getEAIInvokedValue\n");
		return NULL;
	}

	myParam = vector_get(struct EAINodeParams *, me->nodeParams, field);

	if (myParam == NULL) {
		printf ("bad field in getEAIInvokedValue\n");
		return NULL;
	}

	if (me->nodeType != EAI_NODETYPE_PROTO) {
		ConsoleMessage ("getting EAIInvokedValue on a node that is NOT a PROTO");
		return NULL;
	}
	
	return myParam->invokedPROTOValue;
}


/* return the actual field offset as defined; change fieldHandle into an actual value */
int getEAIActualOffset(int node, int field) {
	struct EAINodeIndexStruct *me;
	struct EAINodeParams *myParam;
	ppEAIHelpers p = (ppEAIHelpers)gglobal()->EAIHelpers.prv;

	me = vector_get(struct EAINodeIndexStruct *, p->EAINodeIndex,node);
	if (me == NULL) {
		printf ("bad node in getEAIActualOffset\n");
		return 0;
	}

	myParam = vector_get(struct EAINodeParams *, me->nodeParams, field);

	if (myParam == NULL) {
		printf ("bad field in getEAIActualOffset\n");
		return 0;
	}

	return myParam->fieldOffset;
}

/* returns node type - see above for definitions */
int getEAINodeTypeFromTable(int node) {
	struct EAINodeIndexStruct *me;
	ppEAIHelpers p = (ppEAIHelpers)gglobal()->EAIHelpers.prv;

	me = vector_get(struct EAINodeIndexStruct *, p->EAINodeIndex,node);
	if (me == NULL) {
		printf ("bad node in getEAINodeFromTable\n");
		return 0;
	}
	return me->nodeType;
}

/* return a registered node. If index==0, return NULL */
struct X3D_Node *getEAINodeFromTable(int index, int field) {
	struct EAINodeIndexStruct *me;
	struct EAINodeParams *myParam;
	ppEAIHelpers p = (ppEAIHelpers)gglobal()->EAIHelpers.prv;

	me = vector_get(struct EAINodeIndexStruct *, p->EAINodeIndex,index);

	if (me==NULL) {
		printf ("internal EAI error - requesting %d, highest node %d\n",
			index,vectorSize(p->EAINodeIndex) /* lastNodeRequested */);
		return NULL;
	}

	/* do we want the (possibly) PROTO Definition node, or the node that
	   is tied to the exact field? Only in PROTO expansions will they be different,
	   when possibly an IS'd field is within a sub-node of the PROTO */

	if (field <0) return me->actualNodePtr;

	/* go and return the node associated directly with this field */
	/* printf ("getEAINodeFromTable, asking for field %d of node %d\n",field,index); */
	myParam = vector_get(struct EAINodeParams *, me->nodeParams, field);

	if (myParam == NULL) {
		printf ("bad field in getEAINodeFromTable\n");
		return NULL;
	}

	return myParam->thisFieldNodePointer;
}

/* return an index to a node table. return value 0 means ERROR!!! */
int registerEAINodeForAccess(struct X3D_Node* myn) {
	int ctr;
	int mynindex = 0;
	int eaiverbose;
	ppEAIHelpers p;
	ttglobal tg = gglobal();
	eaiverbose = tg->EAI_C_CommonFunctions.eaiverbose;
	p = (ppEAIHelpers)tg->EAIHelpers.prv;

	if (eaiverbose) printf ("registerEAINodeForAccess - myn %lu\n",(unsigned long int) myn);
	if (myn == NULL) return -1;

	if (p->EAINodeIndex == NULL) {
		struct EAINodeIndexStruct *newp = MALLOC (struct EAINodeIndexStruct *, sizeof (struct EAINodeIndexStruct));

		if (eaiverbose) printf ("creating EAINodeIndexVector\n"); 
		p->EAINodeIndex = newVector(struct EAINodeIndexStruct*, 512);
		/* push a dummy one here, as we do not want to return an index of zero */
		vector_pushBack(struct EAINodeIndexStruct *, p->EAINodeIndex, newp);
		
	}

	/* ok, index zero of the EAINodeIndex is invalid, so we look at 1 to (size) -1 */
	for (ctr=1; ctr<=vectorSize(p->EAINodeIndex)-1; ctr++) {
		struct EAINodeIndexStruct *me;

		me = vector_get(struct EAINodeIndexStruct *, p->EAINodeIndex, ctr);
		if (me->actualNodePtr == myn) {
			if (eaiverbose) printf ("registerEAINodeForAccess - already got node\n");
			mynindex = ctr;
			break;
		}
	}

	/* did we find this node already? */
	if (mynindex == 0) {
		struct EAINodeIndexStruct *newp = MALLOC (struct EAINodeIndexStruct *, sizeof (struct EAINodeIndexStruct));

		newp->actualNodePtr = myn;
		newp->nodeParams = NULL;
		/* save the node type; either this is a EAI_NODETYPE_SCRIPT, EAI_NODETYPE_PROTO, or EAI_NODETYPE_STANDARD */
		if (myn->_nodeType == NODE_Script) newp->nodeType = EAI_NODETYPE_SCRIPT;
		//else if ((myn->_nodeType == NODE_Group) & (X3D_GROUP(myn)->FreeWRL__protoDef != INT_ID_UNDEFINED)) 
		else if(isProto(myn))
			newp->nodeType = EAI_NODETYPE_PROTO;
		else newp->nodeType = EAI_NODETYPE_STANDARD;

		
		vector_pushBack(struct EAINodeIndexStruct *, p->EAINodeIndex, newp);

		mynindex = vectorSize(p->EAINodeIndex) -1;

		if (eaiverbose) printf ("registerEAINodeForAccess returning index %d nt %s, internal type %d\n",mynindex,
				stringNodeType(myn->_nodeType),newp->nodeType);
	}
	return mynindex;
}


/***************************************************************************************************/

/* this is like EAI_GetNode, but is just for the rootNode of the scene graph */
int EAI_GetRootNode(void) {
	return registerEAINodeForAccess(X3D_NODE(rootNode()));
}


/* get a node pointer in memory for a node. Return the node pointer, or NULL if it fails */
int EAI_GetNode(const char *str) {

	struct X3D_Node * myNode;
	int eaiverbose = gglobal()->EAI_C_CommonFunctions.eaiverbose;
	if (eaiverbose) {
		printf ("EAI_GetNode - getting %s\n",str);
	}	

	/* Try to get X3D node name */
	myNode = X3DParser_getNodeFromName(str);

	if (myNode == NULL) {
		/* Try to get VRML node name */
		myNode = parser_getNodeFromName(str);
	}

	if (myNode != NULL) 
		return registerEAINodeForAccess(myNode);
	return 0;
}

//saves a vector of registered node addresses in the output parameter parentNodesAdr. The output parameter must be freed by the caller
//returns the number of registered nodes or -1 if an error occurred
int EAI_GetNodeParents(int cNode, int **parentNodesAdr)
{
	int parentAdr;
	int i;
	int parentVectorSize;
	struct X3D_Node * myNode;
	struct X3D_Node * parentNode;
	int* tmp;
	
	//get the node
	myNode = getEAINodeFromTable(cNode, -1);

	if(!myNode)
		return -1;

	parentVectorSize = myNode->_parentVector->n;

	tmp =(int*) calloc(parentVectorSize,sizeof(int));

	//cycle along the parent vector
	for(i=0; i < parentVectorSize; i++)
	{
		//get the "i" parent
		parentNode = vector_get(struct X3D_Node *,myNode->_parentVector,i);

		//register the "i" parent and get the address registration
		parentAdr = registerEAINodeForAccess(parentNode);

		//save the address registration in the int array we passed as output parameter
		tmp[i] = parentAdr;
		
		//if the address registration is "0" an error occurred, we exit returning an error code (-1)
		if(!parentAdr) {
            FREE_IF_NZ(tmp);
			return -1;
        }
	}

	*parentNodesAdr = tmp;
	//all done, we return the count of found parents
	return i;
}


int mapToKEYWORDindex (indexT pkwIndex) {
	if (pkwIndex == PKW_inputOutput) return KW_inputOutput;
	if (pkwIndex == PKW_inputOnly) return KW_inputOnly;
	if (pkwIndex == PKW_outputOnly) return KW_outputOnly;
	if (pkwIndex == PKW_initializeOnly) return KW_initializeOnly;
	return 0;
}

/* in this proto expansion, just go and get the expanded node/field IF POSSIBLE */
static int changeExpandedPROTOtoActualNode(int cNode, struct X3D_Node **np, char **fp, int direction) {
	struct ProtoDefinition *myProtoDecl;
	char thisID[2000];
	int eaiverbose = gglobal()->EAI_C_CommonFunctions.eaiverbose;

	/* first, is this node a PROTO? We look at the actual table to determine if it is special or not */
	if (getEAINodeTypeFromTable(cNode) != EAI_NODETYPE_PROTO) {
		return TRUE;
	}

	/* yes, it is a PROTO */
	if (eaiverbose) {
		printf ("changeExpanded - looking for field %s in node...\n",*fp); 
	}

	myProtoDecl = getVRMLprotoDefinition(X3D_GROUP(*np));
	if (eaiverbose) {
		printf ("and, the proto name is %s\n",myProtoDecl->protoName);
	}

	/* make up the name of the Metadata field associated with this one */
	if (strlen(*fp)>1000) return FALSE;

	sprintf (thisID,"PROTO_%p_%s",myProtoDecl,*fp);

	if (eaiverbose) printf ("looking for name :%s:\n",thisID);

	*np = parser_getNodeFromName(thisID);
	if ((*np) == 0) return FALSE;

	if (eaiverbose) {
		printf ("np is %lu\n",(unsigned long int) *np);
		printf ("and, found node %lu type %s\n",(unsigned long int) *np, stringNodeType((*np)->_nodeType));
	}

	/* change the fieldName, depending on the direction */
        /* see if this is an input or output request from nodes =0, tonodes = 1 */
	if (direction == 0) *fp = "valueChanged"; else *fp = "setValue";

	return TRUE;
}



/* get the type of a node; node must exist in table 
 	input:	
	cNode = handle for node pointer into memory - if not valid, this routine returns everything as zeroes
	fieldString =  - eg, "addChildren"
	accessMethod = "eventIn", "eventOut", "field" or...???

	returns:
	cNodePtr = C node pointer;
	fieldOffset = offset;
	dataLen = data len;
	typeString = mapFieldTypeToEAItype (ctype);
	scripttype = 0 - meaning, not to/from a javascript. (see CRoutes.c for values and more info)
*/

void EAI_GetType (int cNode,  char *inputFieldString, char *accessMethod,
                int *cNodePtr, int *fieldOffset,
                int *dataLen, int *typeString,  int *scripttype, int *accessType) {

	struct EAINodeIndexStruct *me;
	struct EAINodeParams *newp; 
	struct X3D_Node* nodePtr = getEAINodeFromTable(cNode,-1);
	char *fieldString = inputFieldString;
	int myField;
	int ctype;
	int myFieldOffs;
	char *invokedValPtr = NULL;  /* for PROTOs - invocation value */
	int myScriptType = EAI_NODETYPE_STANDARD;
	int direction;
	// struct X3D_Node* protoBaseNode;
	int isProtoExpansion = FALSE;
	int eaiverbose;
	ppEAIHelpers p;
	ttglobal tg = gglobal();
	eaiverbose = tg->EAI_C_CommonFunctions.eaiverbose;
	p = (ppEAIHelpers)tg->EAIHelpers.prv;

	eaiverbose = gglobal()->EAI_C_CommonFunctions.eaiverbose;

	/* see if this is an input or output request from nodes =0, tonodes = 1 */
	direction=0;
	if (strncmp(accessMethod,"in",strlen("in")) == 0) direction=1;
	else if (strncmp(accessMethod,"eventIn",strlen("eventIn"))==0) direction=1;

	/*if (direction==0) {
		printf ("EAI_GetType, this is a FROM route (%s)\n",accessMethod);
	} else {
		printf ("EAI_GetType, this is a TO route (%s)\n",accessMethod);
	} */
		

	if (eaiverbose) {
		printf ("call to EAI_GetType, cNode %d fieldString :%s: accessMethod %s\n",cNode,fieldString,accessMethod);
	}

	/* is this a valid C node? if so, lets just get the info... */
	if ((cNode == 0) || (cNode > vectorSize(p->EAINodeIndex) /* lastNodeRequested */)) {
		printf ("THIS IS AN ERROR! CNode is zero!!!\n");
		*cNodePtr = 0; *fieldOffset = 0; *dataLen = 0; *typeString = 0; *scripttype=0; *accessType=KW_eventIn;
		return;
	}

	if (eaiverbose) {
		printf ("start of EAI_GetType, this is a valid C node %lu\n",(unsigned long int) nodePtr);
		printf ("	of string type %s\n",stringNodeType(nodePtr->_nodeType)); 
	}	

	myFieldOffs = -999;

	/* is this a field of the actual base type? */
	/* did not find the field as an ISd field - is this a field of the actual X3D base node?? */
	/* mimic POSSIBLE_PROTO_EXPANSION(nodePtr,protoBaseNode); */

/* unused 
        if (nodePtr == NULL) protoBaseNode = NULL; 
        else {if (X3D_NODE(nodePtr)->_nodeType == NODE_Group) { 
                if (X3D_GROUP(nodePtr)->children.n>0) { 
                        protoBaseNode = X3D_GROUP(nodePtr)->children.p[0]; 
                } else protoBaseNode = NULL; 
        } else protoBaseNode = nodePtr; };
*/

	/* is this a proto expansion? */
	//if (X3D_NODE(nodePtr)->_nodeType == NODE_Group) {
	//	if (X3D_GROUP(nodePtr)->FreeWRL__protoDef != INT_ID_UNDEFINED) {
	if(isProto(nodePtr))
			isProtoExpansion = TRUE;
	//	}
	//}

	if (isProtoExpansion) {
		/* this possibly is an expanded PROTO?, change the nodePtr and fieldString around */
		if (!changeExpandedPROTOtoActualNode (cNode, &nodePtr, &fieldString,direction)) {
			ConsoleMessage ("Did NOT find field :%s: in PROTO expansion",fieldString);
		}
	} else {
		if (eaiverbose) printf ("EAI_GetType - no, this is NOT a proto node\n");
	}

	if (nodePtr == NULL) {
		if (isProtoExpansion)
		ConsoleMessage ("error looking up field :%s: a PROTO Definition\n", fieldString);
		else
		ConsoleMessage ("error looking up field :%s: in an unknown node\n", fieldString);
		return;			
	}

	if (eaiverbose) {
		printf ("node here is %lu\n",(unsigned long int) nodePtr);
		printf ("ok, going to try and find field :%s: in a node of type :%s:\n",fieldString,stringNodeType(nodePtr->_nodeType));
	}

	/* try finding it, maybe with a "set_" or "changed" removed */
	myField = findRoutedFieldInFIELDNAMES(nodePtr,fieldString,direction);

	if (eaiverbose) printf ("EAI_GetType, for field %s, myField is %d\n",fieldString,myField);

	/* find offsets, etc */
       	findFieldInOFFSETS(nodePtr->_nodeType, myField, &myFieldOffs, &ctype, accessType);

	if (eaiverbose) {
		printf ("EAI_GetType, after changeExpandedPROTOtoActualNode, C node %lu\n",(unsigned long int)nodePtr);
		printf ("	of string type %s\n",stringNodeType(nodePtr->_nodeType)); 
	}	


	if (eaiverbose) printf ("EAI_GetType, after findFieldInOFFSETS, have myFieldOffs %u, ctype %d, accessType %d \n",myFieldOffs, ctype, *accessType);

	/* is this a Script, or just an invalid field?? */ 
	if (myFieldOffs <= 0) {
        	int i;

		/* is this a Script node? */
		if (nodePtr->_nodeType == NODE_Script) {
			struct Shader_Script* myScript;
			struct CRjsnameStruct *JSparamnames = getJSparamnames();
			struct VRMLParser *globalParser = (struct VRMLParser *)gglobal()->CParse.globalParser;
			if (eaiverbose)
				printf ("EAI_GetType, node is a Script node...\n");
			myScript = X3D_SCRIPT(nodePtr)->__scriptObj;
			myScriptType = EAI_NODETYPE_SCRIPT;

        		for (i = 0; i !=  vectorSize(myScript->fields); ++i) {
        		        struct ScriptFieldDecl* sfield = vector_get(struct ScriptFieldDecl*, myScript->fields, i);
				
				if (eaiverbose)
				printf ("   field %d,  name %s type %s (type %s accessType %d (%s), indexName %d, stringType %s)\n",
						i,
						fieldDecl_getShaderScriptName(sfield->fieldDecl),
						stringFieldtypeType(fieldDecl_getType(sfield->fieldDecl)),
						stringFieldtypeType(fieldDecl_getType(sfield->fieldDecl)),
						fieldDecl_getAccessType(sfield->fieldDecl),
						stringPROTOKeywordType(fieldDecl_getAccessType(sfield->fieldDecl)),
						fieldDecl_getIndexName(sfield->fieldDecl),
						fieldDecl_getStringName(globalParser->lexer,sfield->fieldDecl)
				);
				
				
				if (strcmp(fieldString,fieldDecl_getShaderScriptName(sfield->fieldDecl)) == 0) {
					/* call JSparamIndex to get a unique index for this name - this is used for ALL
					   script access, whether from EAI or not */
					if(eaiverbose)
					printf ("found it at index, %d but returning JSparamIndex %d\n",i,
						fieldDecl_getShaderScriptIndex(sfield->fieldDecl));

					myFieldOffs = fieldDecl_getShaderScriptIndex(sfield->fieldDecl);
					/* switch from "PKW" to "KW" types */
					*accessType = mapToKEYWORDindex(fieldDecl_getAccessType(sfield->fieldDecl));
					ctype = fieldDecl_getType(sfield->fieldDecl);
					break;
				}
			}

		} else {
			if (nodePtr != NULL)
			printf ("EAI_GetType, warning: field :%s: not not found in node of type :%s:\n",fieldString,stringNodeType(nodePtr->_nodeType));
			else
			printf ("EAI_GetType, warning: field :%s: not not found in node with pointer of NULL\n",fieldString);
		}

	}

	/* save these indexes */

	me = vector_get(struct EAINodeIndexStruct *, p->EAINodeIndex, cNode);
	if (me->nodeParams == NULL) {
		struct EAINodeParams *np = MALLOC (struct EAINodeParams *, sizeof (struct EAINodeParams));
		if (eaiverbose) printf ("creating new field vector for this node\n");
		me->nodeParams = newVector(struct EAINodeParams*, 4);
		/* push a dummy one here, as we do not want to return an index of zero */
		vector_pushBack(struct EAINodeParams *, me->nodeParams, np);
	}

	newp = MALLOC (struct EAINodeParams *, sizeof (struct EAINodeParams));
	newp->fieldOffset = myFieldOffs;
	newp->datalen = ctype;
	newp->typeString = mapFieldTypeToEAItype(ctype);
	newp->scripttype = myScriptType;
	newp->invokedPROTOValue = invokedValPtr;

	/* has the node type changed, maybe because of a PROTO expansion? */
	if (me->actualNodePtr != nodePtr) {
		/* printf ("iEAI_GetType, node pointer changed, using new node pointer\n"); */
		newp->thisFieldNodePointer= nodePtr;
	} else {
		/* printf ("EAI_GetType, node is same as parent node\n"); */
		newp->thisFieldNodePointer= me->actualNodePtr;
	}

	vector_pushBack(struct EAINodeParams *, me->nodeParams, newp);

	/* 
	printf ("end of GetType, orig nodeptr %u, now %u\n",me->actualNodePtr, nodePtr);
	printf ("end of GetType, now, EAI node type %d\n",me->nodeType);
	*/

	*fieldOffset = vectorSize(me->nodeParams)-1; 	/* the entry into this field array for this node */
	*dataLen = (int) newp->datalen;	/* data len */
	*typeString = newp->typeString; /* data type in EAI type */
	*scripttype =newp->scripttype;
	*cNodePtr = cNode;	/* keep things with indexes */
}

	
char *EAI_GetTypeName (unsigned int uretval) {
	printf ("HELP::EAI_GetTypeName %d\n",uretval);
	return "unknownType";
}


int SAI_IntRetCommand (char cmnd, const char *fn) {
	printf ("HELP::SAI_IntRetCommand, %c, %s\n",cmnd,fn);
	return 0;
}

char * SAI_StrRetCommand (char cmnd, const char *fn) {
	printf ("HELP::SAI_StrRetCommand, %c, %s\n",cmnd,fn);
	return "iunknownreturn";
}

/* returns an viewpoint node  or NULL if not found */
struct X3D_Node *EAI_GetViewpoint(const char *str) {
       struct X3D_Node * myNode;

        /* Try to get X3D node name */

	#ifdef IPHONE
	myNode = NULL; 
	printf ("X3DParser_getNodeFromName not here yet\n");
	#else
	myNode = X3DParser_getNodeFromName(str);
	#endif
        if (myNode == NULL) {
                /* Try to get VRML node name */
                myNode = parser_getNodeFromName(str);
        }

	return myNode;
}



/* we have a GETVALUE command coming in */
void handleEAIGetValue (char command, char *bufptr, int repno) {
	struct X3D_Node *myNode;
	int nodeIndex, fieldIndex, length;
	char ctmp[4000];
	int retint;
	struct EAINodeIndexStruct *me;
	struct EAINodeParams *myParam;
	int eaiverbose;
	ppEAIHelpers p;
	ttglobal tg;
	UNUSED(retint); // compiler warning mitigation

	tg = gglobal();
	eaiverbose = gglobal()->EAI_C_CommonFunctions.eaiverbose;
	p = (ppEAIHelpers)gglobal()->EAIHelpers.prv;

	eaiverbose = gglobal()->EAI_C_CommonFunctions.eaiverbose;

	if (eaiverbose) printf ("GETVALUE %s \n",bufptr);

	/* format: ptr, offset, type, length (bytes)*/
	retint=sscanf (bufptr, "%d %d %c %d", &nodeIndex,&fieldIndex,ctmp,&length);
	myNode = getEAINodeFromTable(nodeIndex, fieldIndex);

	/* if myNode is NULL, we have an error, baby */
	if (myNode == NULL) {
		printf ("handleEAIGetValue - node does not exist!\n");
		return;
	}
	me = vector_get(struct EAINodeIndexStruct *, p->EAINodeIndex, nodeIndex);

	if (me==NULL) {
		printf ("handleEAIGetValue - node does not exist in vector!\n");
		return;
	}
	
	/* printf ("handleEAIGetValue, node %u, type %s\n",myNode, stringNodeType(myNode->_nodeType)); */

	/* is the pointer a pointer to a PROTO?? If so, then the getType did not find
	an actual field (an IS'd field??) in a proto expansion for us.  We have to 
	go through, as the offset will be the index in the PROTO field for us to get
	the value for */


	myParam = vector_get(struct EAINodeParams *, me->nodeParams, fieldIndex);

	if (myParam == NULL) {
		printf ("bad field in handleEAIGetValue\n");
		return;
	}

	if (myParam->invokedPROTOValue != NULL) {
		sprintf (tg->EAIHelpers.outBuffer,"RE\n%f\n%d\n%s",TickTime(),repno,getEAIInvokedValue(nodeIndex,fieldIndex));	
	} else {
		EAI_Convert_mem_to_ASCII (repno,"RE",mapEAItypeToFieldType(ctmp[0]),getEAIMemoryPointer(nodeIndex,fieldIndex), tg->EAIHelpers.outBuffer);
	}
}


/* this is a debugging function */
char *eaiPrintCommand (char command) {

	switch (command) {

		case GETNODE: return ("GETNODE");
		case GETEAINODETYPE: return ("GETEAINODETYPE");
		case SENDCHILD: return ("SENDCHILD");
		case SENDEVENT: return ("SENDEVENT");
		case GETVALUE: return ("GETVALUE");
		case GETFIELDTYPE: return ("GETFIELDTYPE");
		case REGLISTENER: return ("REGLISTENER");
		case ADDROUTE: return ("ADDROUTE");
		case REREADWRL: return ("REREADWRL");
		case DELETEROUTE: return ("DELETEROUTE");
		case GETNAME: return ("GETNAME");
		case GETVERSION: return ("GETVERSION");
		case GETCURSPEED: return ("GETCURSPEED");
		case GETFRAMERATE: return ("GETFRAMERATE");
		case GETURL: return ("GETURL");
		case REPLACEWORLD: return ("REPLACEWORLD");
		case LOADURL: return ("LOADURL");
		case VIEWPOINT: return ("VIEWPOINT");
		case CREATEVS: return ("CREATEVS");
		case CREATEVU: return ("CREATEVU");
		case STOPFREEWRL: return ("STOPFREEWRL");
		case UNREGLISTENER: return ("UNREGLISTENER");
		case GETRENDPROP: return ("GETRENDPROP");
		case GETENCODING: return ("GETENCODING");
		case CREATENODE: return ("CREATENODE");
		case CREATEPROTO: return ("CREATEPROTO");
		case UPDNAMEDNODE: return ("UPDNAMEDNODE");
		case REMNAMEDNODE: return ("REMNAMEDNODE");
		case GETPROTODECL: return ("GETPROTODECL");
		case UPDPROTODECL: return ("UPDPROTODECL");
		case REMPROTODECL: return ("REMPROTODECL");
		case GETFIELDDEFS: return ("GETFIELDDEFS");
		case GETNODEDEFNAME: return ("GETNODEDEFNAME");
		case GETROUTES: return ("GETROUTES");
		case GETNODETYPE: return ("GETNODETYPE");
		default:{} ;
	}
	return "unknown command...";
}


/* append str to the outbuffer, REALLOC if necessary */
void outBufferCat (char *str) {
	int a,b;
	struct tEAIHelpers* t = &gglobal()->EAIHelpers;
	a = (int) strlen (t->outBuffer);
	b = (int) strlen (str);

	/* should we increase the size here? */
	if ((a+b+2) >= t->outBufferLen) {
		t->outBufferLen = a+b+200; /* give it more space, and a bit more, so maybe
					   REALLOC does not need to be called all the time */
		t->outBuffer = REALLOC(t->outBuffer, t->outBufferLen);
	}
	strcat (t->outBuffer, str);
}


#ifdef SWAMPTEA

/* SWAMPTEA specific: get a switch node "whichChoice" pointer */
int* getSwitchNodeFromTable(int cNode) {
	ppEAIHelpers p;
	ttglobal tg = gglobal();
	struct EAINodeIndexStruct *me;

	p = (ppEAIHelpers)gglobal()->EAIHelpers.prv;
	me = vector_get(struct EAINodeIndexStruct *, p->EAINodeIndex,cNode);
	return offsetPointer_deref (int *, me->actualNodePtr, offsetof (struct X3D_Switch, whichChoice));
}

/* SWAMPTEA specific: move a little cone up/down depending on a fraction */
void confidenceConeSet(int coneNode, int shown,int found) {
	ppEAIHelpers p;
	ttglobal tg = gglobal();

	struct X3D_Transform *trans;
	struct EAINodeIndexStruct *me;
	struct SFVec3f *translation;

#define CONE_MOVE_HEIGHT 14.0

	p = (ppEAIHelpers)gglobal()->EAIHelpers.prv;
	// bounds check
	if (coneNode <=0) return;

    me = vector_get(struct EAINodeIndexStruct *, p->EAINodeIndex,coneNode);
	trans = X3D_TRANSFORM(me->actualNodePtr);

	if (trans->_nodeType != NODE_Transform) {
		ConsoleMessage ("confidence - not a Transform");
		return;
	}
	
	if (found==0) {
		ConsoleMessage ("confidence - nothing found, nothing to do");
		return;
	}

	// find the translation field of this Transform
	translation = offsetPointer_deref (struct SFVec3f*, trans, offsetof (struct X3D_Transform, translation));	
	// move it in the "Y" axis
	translation->c[1] = (float)shown/(float)found*CONE_MOVE_HEIGHT - (CONE_MOVE_HEIGHT/2);

	// tell the system to recalculate it all
	trans->_change ++;
}


/* SWAMPTEA specific: add/delete this texture to our multitexture */ 
void textureToMultiTexture(int texNode, int MultiTexture, int add) {
	ppEAIHelpers p;
	ttglobal tg = gglobal();

	char line[200];
	struct X3D_Node *MultiTexNode;
	struct X3D_Node *myTex;
	int addFlag;
	struct EAINodeIndexStruct *me;

	p = (ppEAIHelpers)gglobal()->EAIHelpers.prv;

	if (add) addFlag=1; else addFlag=2;
	


	sprintf (line,"textureToMultiTexture, texnode %d multitexture %d add %d",texNode,MultiTexture,add);
	ConsoleMessage(line);
	
	// bounds checking
	if ((texNode <=0) || (MultiTexture<=0)) {
		sprintf (line,"textureToMultiTexture, can not do: texnode %d multitexture %d add %d",texNode,MultiTexture,add);
		ConsoleMessage (line);
		return;
	}

        me = vector_get(struct EAINodeIndexStruct *, p->EAINodeIndex,MultiTexture);
	MultiTexNode = me->actualNodePtr;
        me = vector_get(struct EAINodeIndexStruct *, p->EAINodeIndex,texNode);
	myTex = me->actualNodePtr;

	sprintf (line,"textureToMultitexture, adding a :%s: to a :%s:",stringNodeType(myTex->_nodeType),
		stringNodeType(MultiTexNode->_nodeType));
	ConsoleMessage(line);

	if ((myTex->_nodeType!=NODE_PixelTexture) && (MultiTexNode->_nodeType != NODE_MultiTexture)) {
		ConsoleMessage ("textureToMultitexture, wrong type(s)");
		return;
	}

	/* is it there already? */
	if (add) {
		int count;
		struct Multi_Node *texture = offsetPointer_deref(struct Multi_Node *, MultiTexNode, offsetof (struct X3D_MultiTexture, texture));
		for (count=0; count<texture->n; count++) {
			if (texture->p[count] == myTex) return;
		}

	}

	AddRemoveChildren (X3D_NODE(MultiTexNode),
			offsetPointer_deref(void *, MultiTexNode, offsetof (struct X3D_MultiTexture, texture)),
			(struct X3D_Node * *)&myTex,1,addFlag,__FILE__,__LINE__);
}

#endif //SWAMPTEA
