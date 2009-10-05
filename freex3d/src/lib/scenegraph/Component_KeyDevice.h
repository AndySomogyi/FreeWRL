/*
=INSERT_TEMPLATE_HERE=

$Id$

Proximity sensor macro.

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


#ifndef __FREEWRL_SCENEGRAPH_KEYDEVICE_H__
#define __FREEWRL_SCENEGRAPH_KEYDEVICE_H__

/* required def's for this file */
#include "../vrml_parser/Structs.h"	/* for X3D_Node,X3D_KeySensor,X3D_StringSensor */


/* some function def's (are these global??) */
#define X3D_KEYSENSOR(node) ((struct X3D_KeySensor*)node)
#define X3D_STRINGSENSOR(node) ((struct X3D_StringSensor*)node)


/* function protos */
static void incrementKeySinkList();
int KeySensorNodePresent();
void addNodeToKeySensorList(struct X3D_Node* node);
void killKeySensorNodeList();
void sendKeyToKeySensor(const char key, int upDown);
static void sendToKS(struct X3D_Node* wsk, int key, int upDown);
static void sendToSS(struct X3D_Node *wsk, int key, int upDown);

#endif /* __FREEWRL_SCENEGRAPH_KEYDEVICE_H__ */
