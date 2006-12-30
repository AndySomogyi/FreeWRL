/* CProto.h - this is the object representing a PROTO definition and being
 * capable of instantiating it.
 * 
 * We keep a vector of pointers to all that pointers which point to "inner
 * memory" and need therefore be updated when copying.  Such pointers include
 * field-destinations and parts of ROUTEs.  Those pointers are then simply
 * copied, their new positions put in the new vector, and afterwards are all
 * pointers there updated.
 */

#ifndef CPROTO_H
#define CPROTO_H

#include "headers.h"

#include "CParseGeneral.h"
#include "Vector.h"

struct PointerHash;
struct VRMLParser;

/* ************************************************************************** */
/* ******************************** OffsetPointer *************************** */
/* ************************************************************************** */

/* A pointer which is made up of the offset/node pair */
struct OffsetPointer
{
 struct X3D_Node* node;
 unsigned ofs;
};

/* Constructor/destructor */
struct OffsetPointer* newOffsetPointer(struct X3D_Node*, unsigned);
#define offsetPointer_copy(me) \
 newOffsetPointer((me)->node, (me)->ofs)
#define deleteOffsetPointer(me) \
 free(me)

/* Dereference to simple pointer */
#define offsetPointer_deref(t, me) \
 ((t)(((char*)((me)->node))+(me)->ofs))

/* ************************************************************************** */
/* ********************************* ProtoFieldDecl ************************* */
/* ************************************************************************** */

/* The object */
struct ProtoFieldDecl
{
 indexT mode; /* field, exposedField, eventIn, eventOut */
 indexT type; /* field type */
 indexT name; /* field "name" (its lexer-index) */
 /* This is the list of desination pointers for this field */
 struct Vector* dests;
 /* Only for exposedField or field */
 BOOL alreadySet; /* Has the value already been set? */
 union anyVrml defaultVal; /* Default value */
};

/* Constructor and destructor */
struct ProtoFieldDecl* newProtoFieldDecl(indexT, indexT, indexT);
void deleteProtoFieldDecl(struct ProtoFieldDecl*);

/* Copies */
struct ProtoFieldDecl* protoFieldDecl_copy(struct ProtoFieldDecl*);

/* Accessors */
#define protoFieldDecl_getType(me) \
 ((me)->type)
#define protoFieldDecl_getAccessType(me) \
 ((me)->mode)
#define protoFieldDecl_getIndexName(me) \
 ((me)->name)
#define protoFieldDecl_getStringName(me) \
 lexer_stringUser_fieldName(protoFieldDecl_getIndexName(me), \
  protoFieldDecl_getAccessType(me))
#define protoFieldDecl_getDestinationCount(me) \
 vector_size((me)->dests)
#define protoFieldDecl_getDestination(me, i) \
 vector_get(struct OffsetPointer*, (me)->dests, i)
#define protoFieldDecl_getDefaultValue(me) \
 ((me)->defaultVal)

/* Add a destination this field's value must be assigned to */
#define protoFieldDecl_addDestinationOptr(me, optr) \
 vector_pushBack(struct OffsetPointer*, me->dests, optr)
#define protoFieldDecl_addDestination(me, n, o) \
 protoFieldDecl_addDestinationOptr(me, newOffsetPointer(n, o))

/* Sets this field's value (copy to destinations) */
void protoFieldDecl_setValue(struct ProtoFieldDecl*, union anyVrml*);

/* Build a ROUTE from/to this field */
void protoFieldDecl_routeTo(struct ProtoFieldDecl*,
 struct X3D_Node*, unsigned, int dir, struct VRMLParser*);
void protoFieldDecl_routeFrom(struct ProtoFieldDecl*,
 struct X3D_Node*, unsigned, int dir, struct VRMLParser*);

/* Finish this field - if value is not yet set, use default. */
#define protoFieldDecl_finish(me) \
 if(((me)->mode==PKW_field || (me)->mode==PKW_exposedField) && \
  !(me)->alreadySet) \
  protoFieldDecl_setValue(me, &(me)->defaultVal)

/* Add inner pointers' pointers to the vector */
void protoFieldDecl_addInnerPointersPointers(struct ProtoFieldDecl*,
 struct Vector*);

/* Return the length in bytes of this field's type */
size_t protoFieldDecl_getLength(struct ProtoFieldDecl*);

/* ************************************************************************** */
/* ******************************* ProtoRoute ******************************* */
/* ************************************************************************** */

/* A ROUTE defined inside a PROTO block. */
struct ProtoRoute
{
 struct X3D_Node* from;
 struct X3D_Node* to;
 int fromOfs;
 int toOfs;
 size_t len;
 int dir;
};

/* Constructor and destructor */
struct ProtoRoute* newProtoRoute(struct X3D_Node*, int, struct X3D_Node*, int,
 size_t, int);
#define protoRoute_copy(me) \
 newProtoRoute((me)->from, (me)->fromOfs, (me)->to, (me)->toOfs, \
 (me)->len, (me)->dir)
#define deleteProtoRoute(me) \
 free(me)

/* Register this route */
#define protoRoute_register(me) \
 CRoutes_RegisterSimple((me)->from, (me)->fromOfs, (me)->to, (me)->toOfs, \
 (me)->len, (me)->dir)

/* Add this one's inner pointers to the vector */
#define protoRoute_addInnerPointersPointers(me, vec) \
 { \
  vector_pushBack(void**, vec, &(me)->from); \
  vector_pushBack(void**, vec, &(me)->to); \
 }

/* ************************************************************************** */
/* ****************************** ProtoDefinition *************************** */
/* ************************************************************************** */

/* The object */
struct ProtoDefinition
{
 struct X3D_Group* tree; /* The scene graph of the PROTO definition */
 struct Vector* iface; /* The ProtoFieldDecls making up the interface */
 struct Vector* routes; /* Inner ROUTEs */
 struct Vector* innerPtrs; /* Pointers to pointers which need to be updated */
};

/* Constructor and destructor */
struct ProtoDefinition* newProtoDefinition();
void deleteProtoDefinition(struct ProtoDefinition*);

/* Add a node to the virtual group node */
void protoDefinition_addNode(struct ProtoDefinition*, struct X3D_Node*);

/* Adds a field declaration to the interface */
#define protoDefinition_addIfaceField(me, field) \
 vector_pushBack(struct ProtoFieldDecl*, (me)->iface, field)

/* Get fields by indices */
#define protoDefinition_getFieldCount(me) \
 vector_size((me)->iface)
#define protoDefinition_getFieldByNum(me, i) \
 vector_get(struct ProtoFieldDecl*, (me)->iface, i)

/* Retrieves a field declaration of this PROTO */
struct ProtoFieldDecl* protoDefinition_getField(struct ProtoDefinition*, 
 indexT, indexT);

/* Copies a ProtoDefinition, so that we can afterwards fill in field values */
struct ProtoDefinition* protoDefinition_copy(struct ProtoDefinition*);

/* Extracts the scene graph out of a ProtoDefinition */
struct X3D_Group* protoDefinition_extractScene(struct ProtoDefinition*);

/* Fills the innerPtrs field */
void protoDefinition_fillInnerPtrs(struct ProtoDefinition*);

/* Updates interface-pointers to a given memory block */
void protoDefinition_doPtrUpdate(struct ProtoDefinition*,
 uint8_t*, uint8_t*, uint8_t*);

/* Does a recursively deep copy of a node-tree */
struct X3D_Node* protoDefinition_deepCopy(struct X3D_Node*,
 struct ProtoDefinition*, struct PointerHash*);

/* Adds an inner route */
#define protoDefinition_addRoute(me, r) \
 vector_pushBack(struct ProtoRoute*, (me)->routes, r)

/* ************************************************************************** */
/* ******************************* PointerHash ****************************** */
/* ************************************************************************** */

/* A hash table used to check whether a specific pointer has already been
 * copied.  Otherwise we can't keep things like multiple references to the same
 * node when copying. */

/* An entry */
struct PointerHashEntry
{
 struct X3D_Node* original;
 struct X3D_Node* copy;
};

/* The object */
struct PointerHash
{
 #define POINTER_HASH_SIZE	4321
 struct Vector* data[POINTER_HASH_SIZE];
};

struct PointerHash* newPointerHash();
void deletePointerHash(struct PointerHash*);

/* Query the hash */
struct X3D_Node* pointerHash_get(struct PointerHash*, struct X3D_Node*);

/* Add to the hash */
void pointerHash_add(struct PointerHash*, struct X3D_Node*, struct X3D_Node*);

#endif /* Once-check */
