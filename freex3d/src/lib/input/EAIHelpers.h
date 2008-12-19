/*
=INSERT_TEMPLATE_HERE=

$Id$

EAI Helpers functions

*/

#ifndef __FREEX3D_EAI_HELPERS_H__
#define __FREEX3D_EAI_HELPERS_H__


struct X3D_Node *getEAINodeFromTable(int index);
int returnElementRowSize (int type);
int returnElementLength(int type);
int getEAIActualOffset(int node, int field);
uintptr_t *getEAIMemoryPointer (int node, int field);
int registerEAINodeForAccess(struct X3D_Node* myn);

void EAI_GetType(int cNode, char *ctmp, char *dtmp,
		 uintptr_t *cNodePtr, uintptr_t *fieldOffset,
		 uintptr_t *dataLen, uintptr_t *typeString, unsigned int *scripttype, int *accessType);

#endif /* __FREEX3D_EAI_HELPERS_H__ */
