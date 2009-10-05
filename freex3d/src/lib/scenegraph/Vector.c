/*
=INSERT_TEMPLATE_HERE=

$Id$

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



#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"

#include "Vector.h"


/* ************************************************************************** */
/* ******************************** Vector ********************************** */
/* ************************************************************************** */

/* Constructor/destructor */

struct Vector* newVector_(size_t elSize, size_t initSize) {
 	struct Vector* ret=MALLOC(sizeof(struct Vector));
 	ASSERT(ret);
 	ret->n=0;
 	ret->allocn=initSize;
 	ret->data=MALLOC(elSize*ret->allocn);
 	ASSERT(ret->data);
	#ifdef DEBUG_MALLOC
		printf ("vector, new  %x, data %x, size %d\n",ret, ret->data, initSize);
	#endif
	
	return ret;
}

#ifdef DEBUG_MALLOC
void deleteVector_(char *file, int line, size_t elSize, struct Vector* me) {
#else
void deleteVector_(size_t elSize, struct Vector* me) {
#endif
	ASSERT(me);
	#ifdef DEBUG_MALLOC
		printf ("vector, deleting me %x data %x at %s:%d\n",me,me->data,file,line);
	#endif
	if(me->data) FREE_IF_NZ(me->data);
	FREE_IF_NZ(me);
}

/* Ensures there's at least one space free. */
void vector_ensureSpace_(size_t elSize, struct Vector* me) {
	ASSERT(me);
	if(me->n==me->allocn) {
		if(me->allocn) me->allocn*=2;
		else me->allocn=1;

		me->data=REALLOC(me->data, elSize*me->allocn);
		#ifdef DEBUG_MALLOC
			printf ("vector, ensureSpace, me %x, data %x\n",me, me->data);
		#endif
		ASSERT(me->data);
	}
	ASSERT(me->n<me->allocn);
}

/* Shrinks the vector to allocn==n. */
void vector_shrink_(size_t elSize, struct Vector* me) {
	ASSERT(me);
	ASSERT(me->allocn>=me->n);
	if(me->n==me->allocn) return;

	me->allocn=me->n;
	me->data=REALLOC(me->data, elSize*me->allocn);
	ASSERT(!me->allocn || me->data);
}

void* vector_releaseData_(size_t elSize, struct Vector* me) {
	void* ret;

	vector_shrink_(elSize, me);
	ret=me->data;
	#ifdef DEBUG_MALLOC
		printf ("vector, me %x data %x\n",me, me->data);
	#endif
	me->data=NULL;

	return ret;
}
