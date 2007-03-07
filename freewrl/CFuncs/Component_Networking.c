/*******************************************************************
  Copyright (C) 2005, 2006 John Stewart, Ayla Khan, Sarah Dumoulin, CRC Canada.
  DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
  See the GNU Library General Public License (file COPYING in the distribution)
  for conditions of use and redistribution.
 *********************************************************************/

/*******************************************************************

  X3D Networking Component

 *********************************************************************/

#include <math.h>
#include "headers.h"
#include "EAIheaders.h"
#include "installdir.h"

/* keep track of the Midi nodes. */
static uintptr_t *MidiNodes = NULL;
static int num_MidiNodes = 0;

/* ReWireName table */
struct ReWireNamenameStruct {
        char *name;
};

struct ReWireNamenameStruct *ReWireNamenames = 0;
int ReWireNametableSize = -1;
int MAXReWireNameNames = 0;

/* ReWire device/controller  table */
struct ReWireDeviceStruct {
	struct X3D_MidiControl* node;	/* pointer to the node that controls this */
	int encodedDeviceName;		/* index into ReWireNamenames */
	int bus;			/* which MIDI bus this is */
	int channel;			/* which MIDI channel on this bus it is */
	int encodedControllerName;	/* index into ReWireNamenames */
	int controller;			/* controller number */
	int cmin;			/* minimum value for this controller */
	int cmax;			/* maximum value for this controller */
	int ctype;			/* controller type */
};

struct ReWireDeviceStruct *ReWireDevices = 0;
int ReWireDevicetableSize = -1;
int MAXReWireDevices = 0;



/************ START OF MIDI NODE **************************/


/* go through the node, and create a new int value, and a new float value, from an int value */
void determineMIDIValFromFloat (struct X3D_MidiControl *node, int *value, float *fV) {
	int minV, maxV, possibleValueSpread;
	float tv;

	/* find the int value range possible */
	minV = 0;  maxV = 100000;
	if (minV < node->deviceMinVal) minV = node->deviceMinVal;
	if (minV < node->minVal) minV = node->minVal;
	if (maxV > node->deviceMaxVal) maxV = node->deviceMaxVal;
	if (maxV > node->maxVal) maxV = node->maxVal;
	possibleValueSpread = maxV-minV +1;

	/* bounds check the float value */
	if (*fV<0.0) *fV=0.0;
	if (*fV>1.0) *fV=1.0;
	tv = *fV * possibleValueSpread + minV;
	*value = (int) tv;

	#ifdef MIDIVERBOSE
	printf ("determinefromfloat, minV %d maxV %d, val %d fv %f tv %f\n",minV, maxV,*value, *fV,tv);
	#endif

}

/* go through the node, and create a new int value, and a new float value, from an int value */
void determineMIDIValFromInt (struct X3D_MidiControl *node, int *value, float *fV) {
	int minV, maxV, possibleValueSpread;

	minV = 0;  maxV = 100000;
	if (minV < node->deviceMinVal) minV = node->deviceMinVal;
	if (minV < node->minVal) minV = node->minVal;
	if (maxV > node->deviceMaxVal) maxV = node->deviceMaxVal;
	if (maxV > node->maxVal) maxV = node->maxVal;
		
	possibleValueSpread = maxV-minV +1;
	if (*value < minV) *value = minV;
	if (*value > maxV) *value = maxV;
	if (possibleValueSpread != 0) {
		*fV = ((float) *value + minV) / (possibleValueSpread);
	} else { 
		*fV = 0.0;
	}
	#ifdef MIDIVERBOSE
	printf ("determine, minV %d maxV %d, val %d fv %f\n",minV, maxV,*value, *fV);
	#endif
	
}


/* send this node out to the ReWire network */
sendNodeToReWire(struct X3D_MidiControl *node) {
	int buf[200];

	if (node->controllerPresent) {
		sprintf (buf,"RW %d %d %d %d\n",node->_bus, node->_channel, node->_controller, node->intValue);
		EAI_send_string(buf,EAIlistenfd);
	}
	
}



/* return parameters associated with this name. returns TRUE if this device has been added by
the ReWire system */

int ReWireDeviceIndex (struct X3D_MidiControl *node, int *bus, int *channel, 
	int *controller, int *cmin, int *cmax, int *ctype) {
	int ctr;
	char *tmp;
	int dev = node->_deviceNameIndex;
	int cont = node->_channelIndex;

	
	/* is this a duplicate name and type? types have to be same,
	   name lengths have to be the same, and the strings have to be the same.
	*/
	for (ctr=0; ctr<=ReWireDevicetableSize; ctr++) {
		#ifdef VERBOSE
			printf ("comparing %d %d to %d %d\n",dev,ReWireDevices[ctr].encodedDeviceName,
			cont, ReWireDevices[ctr].encodedControllerName); 
		#endif

		if ((dev==ReWireDevices[ctr].encodedDeviceName) &&
			(cont == ReWireDevices[ctr].encodedControllerName)) {
			#ifdef MIDIVERBOSE
				printf ("ReWireDeviceIndex, FOUND IT at %d\n",ctr);
			#endif
			*bus = ReWireDevices[ctr].bus;
			*channel = ReWireDevices[ctr].channel;
			*controller = ReWireDevices[ctr].controller;
			*cmin = ReWireDevices[ctr].cmin;
			*cmax = ReWireDevices[ctr].cmax;
			*ctype = ReWireDevices[ctr].ctype;

			/* is this the first time this node is associated with this entry? */
			if (ReWireDevices[ctr].node == NULL) {
				ReWireDevices[ctr].node = node;
				#ifdef MIDIVERBOSE
				printf ("ReWireDeviceIndex, tying node %d to index %d\n",node,ctr);
				#endif
			}
			return TRUE; /* name found */
		}
	}

	/* nope, not duplicate */
	#ifdef MIDIVERVOSE
	printf ("ReWireDeviceIndex: name not added, name not found\n");
	#endif
	return FALSE; /* name not added, name not found */ 
}

/* returns TRUE if register goes ok, FALSE if already registered */
int ReWireDeviceRegister (int dev, int cont, int *bus, int *channel, 
	int *controller, int *cmin, int *cmax, int *ctype) {
	int ctr;
	char *tmp;
	
	/* is this a duplicate name and type? types have to be same,
	   name lengths have to be the same, and the strings have to be the same.
	*/
	for (ctr=0; ctr<=ReWireDevicetableSize; ctr++) {
		#ifdef VERBOSE
			printf ("comparing %d %d to %d %d\n",dev,ReWireDevices[ctr].encodedDeviceName,
			cont, ReWireDevices[ctr].encodedControllerName); 
		#endif

		if ((dev==ReWireDevices[ctr].encodedDeviceName) &&
			(cont == ReWireDevices[ctr].encodedControllerName)) {
			#ifdef MIDIVERBOSE
				printf ("ReWireDeviceRegister, FOUND IT at %d\n",ctr);
			#endif
			return FALSE; /* name found */
		}
	}

	/* nope, not duplicate */
	ReWireDevicetableSize ++;
	
	/* ok, we got a name and a type */
	if (ReWireDevicetableSize >= MAXReWireDevices) {
		/* oooh! not enough room at the table */
		MAXReWireDevices += 1024; /* arbitrary number */
		ReWireDevices = (struct ReWireDevicenameStruct*)realloc (ReWireDevices, sizeof(*ReWireDevices) * MAXReWireDevices);
	}
	
	ReWireDevices[ReWireDevicetableSize].bus = *bus;
	ReWireDevices[ReWireDevicetableSize].channel = *channel;
	ReWireDevices[ReWireDevicetableSize].controller = *controller;
	ReWireDevices[ReWireDevicetableSize].cmin = *cmin; 
	ReWireDevices[ReWireDevicetableSize].cmax = *cmax;
	ReWireDevices[ReWireDevicetableSize].ctype = *ctype;
	ReWireDevices[ReWireDevicetableSize].encodedDeviceName = dev;
	ReWireDevices[ReWireDevicetableSize].encodedControllerName = cont;
	ReWireDevices[ReWireDevicetableSize].node = NULL;
	#ifdef MIDIVERBOSE
		printf ("ReWireDeviceRegister, new entry at %d\n",ReWireDevicetableSize);
	#endif
	return TRUE; /* name not found, but, requested */
}

void registerReWireNode(void *node) {
	struct X3D_Box * tmp;
	int count;
	uintptr_t *myptr;

	if (node == 0) {
		printf ("error in registerReWireNode; somehow the node datastructure is zero \n");
		return;
	}

	tmp = (struct X3D_Box*) node;

	if (tmp->_nodeType != NODE_MidiControl) return;

	MidiNodes = (uintptr_t *) realloc(MidiNodes,sizeof (uintptr_t *) * (num_MidiNodes+1));

	if (MidiNodes == 0) {
		printf ("can not allocate memory in registerReWireNode\n");
		num_MidiNodes = 0;
	}
	myptr = MidiNodes;

	/* does this event exist? */
	for (count=0; count <num_MidiNodes; count ++) {
		if (*myptr == (uintptr_t) node) {
			printf ("registerReWireNode, already have %d\n",node); 
			return;
		}	
		myptr++;
	}


	/* now, put the function pointer and data pointer into the structure entry */
	*myptr = (uintptr_t) node;

	num_MidiNodes++;
}

/* "forget" the ReWireNames. Keep the table around, though, as the entries will simply be used again. */
void kill_ReWireNameTable(void) {
	ReWireNametableSize = -1;
	ReWireDevicetableSize = -1;
}

/* return a node assoicated with this name. If the name exists, return the previous node. If not, return
the new node */
int ReWireNameIndex (char *name) {
	int ctr;
	char *tmp;
	
	/* printf ("ReWireNameIndex, looking for %s\n",name); */
	/* is this a duplicate name and type? types have to be same,
	   name lengths have to be the same, and the strings have to be the same.
	*/
	for (ctr=0; ctr<=ReWireNametableSize; ctr++) {
		/* printf ("ReWireNameIndex, comparing %s to %s\n",name,ReWireNamenames[ctr].name); */
		if (strcmp(name,ReWireNamenames[ctr].name)==0) {
			/* printf ("ReWireNameIndex, FOUND IT at %d\n",ctr); */
			return ctr;
		}
	}

	/* nope, not duplicate */

	ReWireNametableSize ++;

	/* ok, we got a name and a type */
	if (ReWireNametableSize >= MAXReWireNameNames) {
		/* oooh! not enough room at the table */
		MAXReWireNameNames += 1024; /* arbitrary number */
		ReWireNamenames = (struct ReWireNamenameStruct*)realloc (ReWireNamenames, sizeof(*ReWireNamenames) * MAXReWireNameNames);
	}

	ReWireNamenames[ReWireNametableSize].name = strdup(name);
	/* printf ("ReWireNameIndex, new entry at %d\n",ReWireNametableSize); */
	return ReWireNametableSize;
}

void sendCompiledNodeToReWire(struct X3D_MidiControl *node) {
#define MAXOUTLINE 3000
	char outline[MAXOUTLINE];
	printf ("sendCompiledNodeToReWire %d\n",node);

	sprintf (outline,"RWNODE\n%d %d %d %d %d %d %d %d %f %d %d %d %s\nRW_EOT",
			node,node->_bus, node->_channel, node->deviceMinVal, node->deviceMaxVal,
			node->minVal, node->maxVal,node->intValue, node->floatValue, node->useIntValue,
			node->continuousEvents, node->highResolution,node->controllerType->strptr);

	printf (outline); printf ("\n");
	EAI_send_string(outline,EAIlistenfd);
}

void prep_MidiControl (struct X3D_MidiControl *node) {
	/* get the name/device pairing */
	int tmp_bus;
	int tmp_channel;
	int tmp_controller;
	int tmpdeviceMinVal;
	int tmpdeviceMaxVal;
	int tmpintControllerType;
	int controllerPresent;
	int first_Time;

	/* is there anything to do? */
/*
                                                highResolution => [SFBool, TRUE, exposedField], # high resolution controller
*/


	/* first time through, the _ichange will be 0. Lets ensure that we start events properly. */
	if (node->_ichange == 0) {
		printf ("Midi Node being initialized\n");
		node->controllerPresent = 999; /* force event for correct controllerPresent */
	}

	if (node->deviceName->touched > 0) {
		printf ("NODE DEVICE NAME CHANGED\n");
		node->_deviceNameIndex = ReWireNameIndex(node->deviceName->strptr);
		node->deviceName->touched = 0;
	}
	if (node->channel->touched > 0) {
		printf ("NODE CHANNEL CHANGED\n");
		node->_channelIndex = ReWireNameIndex(node->channel->strptr);
		node->channel->touched = 0;
	}


	controllerPresent = ReWireDeviceIndex (node,
				&(tmp_bus),
				&(tmp_channel),
				&(tmp_controller),
				&(tmpdeviceMinVal),
				&(tmpdeviceMaxVal),
				&(tmpintControllerType));

	#ifdef MIDIVERBOSE
		printf ("compile_midiControl, for %d %d controllerPresent %d, node->controllerPresent %d\n",
				ReWireNameIndex(node->deviceName->strptr),
				ReWireNameIndex(node->channel->strptr),
				controllerPresent, node->controllerPresent);
	#endif

	if (controllerPresent != node->controllerPresent) {
		#ifdef MIDIVERBOSE
		printf ("controllerPresent changed\n");
		#endif
		node->controllerPresent = controllerPresent;
		mark_event (node, offsetof(struct X3D_MidiControl, controllerPresent));
	}
	if (node->deviceName->touched!= 0) {
		#ifdef MIDIVERBOSE
		printf ("device changed\n");
		#endif
		node->deviceName->touched= 0;
		mark_event (node, offsetof(struct X3D_MidiControl, deviceName));
	}
	if (node->channel->touched != 0) {
		#ifdef MIDIVERBOSE
		printf ("channel changed\n");
		#endif
		node->channel->touched = 0;  
		mark_event (node, offsetof(struct X3D_MidiControl, channel));
	}


	if (controllerPresent) {
		if (tmp_bus != node->_bus) {
			printf ("INTERNAL: bus changed from %d to %d\n",node->_bus, tmp_bus);
			node->_bus = tmp_bus;
		}
		if (tmp_channel != node->_channel) {
			printf ("INTERNAL: channel changed from %d to %d\n",node->_channel, tmp_channel);
			node->_channel = tmp_channel;
		}
		if (tmp_controller != node->_controller) {
			printf ("INTERNAL: controller changed from %d to %d\n",node->_channel, tmp_channel);
			node->_controller = tmp_controller;
		}
		if (tmpdeviceMinVal != node->deviceMinVal) {
			printf ("deviceMinVal changed from %d to %d\n",node->deviceMinVal, tmpdeviceMinVal);
			node->deviceMinVal = tmpdeviceMinVal;
			mark_event (node, offsetof(struct X3D_MidiControl, deviceMinVal));
		}
		if (tmpdeviceMaxVal != node->deviceMaxVal) {
			printf ("deviceMaxVal changed from %d to %d\n",node->deviceMaxVal, tmpdeviceMaxVal);
			node->deviceMaxVal = tmpdeviceMaxVal;
			mark_event (node, offsetof(struct X3D_MidiControl, deviceMaxVal));
		}
		if (tmpintControllerType != node->intControllerType) {
			printf ("intControllerType  changed from %d to %d\n",node->intControllerType, tmpintControllerType);
			node->intControllerType = tmpintControllerType;
			mark_event (node, offsetof(struct X3D_MidiControl, controllerType));
			switch (node->intControllerType) {
				case MIDI_CONTROLLER_CONTINUOUS:
					node->continuousEvents = TRUE;
					verify_Uni_String(node->controllerType,"Continuous");
					break;
				case MIDI_CONTROLLER_STEP:
					node->continuousEvents = FALSE;
					verify_Uni_String(node->controllerType,"Step");
					break;
				case MIDI_CONTROLLER_BIPOLAR:
					node->continuousEvents = FALSE;
					verify_Uni_String(node->controllerType,"BiPolar");
					break;
				case MIDI_CONTROLLER_UNKNOWN:
					node->continuousEvents = FALSE;
					verify_Uni_String(node->controllerType,"Unknown");
					break;
			}
		}
	} else {
		#ifdef MIDIVERBOSE
		printf ("compile_MidiControl - device not present yet\n");
		#endif
	}

	/*sendCompiledNodeToReWire(node); */

	MARK_NODE_COMPILED 
}

void ReWireRegisterMIDI (char *str) {
	char *pt;
	int curBus;
	int curDevice;
	int curChannel;
	int curMin;
	int curMax;
	int curType;
	int curController;

	char *EOT;

	int encodedDeviceName;
	int encodedControllerName;



	/*
	   "Hardware Interface 1" 5 0 
	   "Melody Automation" 5 1 
	   1 "Mod Wheel" 1 127 0
	   5 "Portamento" 1 127 0
	   7 "Master Level" 1 127 0
	   9 "Oscillator B Decay" 1 127 0
	   12 "Oscillator B Sustain" 1 127 0
	   14 "Filter Env Attack" 1 127 0
	   15 "Filter Env Decay" 1 127 0
	   16 "Filter Env Sustain" 1 127 0
	   17 "Filter Env Release" 1 127 0
	   18 "Filter Env Amount" 1 127 0
	   19 "Filter Env Invert" 3 1 0
	   21 "Oscillator B Octave" 3 8 0
	   79 "Body Type" 3 4 0
	 */	

	/* set the device tables to the starting value. */
	ReWireDevicetableSize = -1;

	while (*str != '\0') {
		while (*str == '\n') str++;
		/* is this a new device? */

		if (*str == '"') {
			str++;
			EOT = strchr (str,'"');
			if (EOT != NULL) *EOT = '\0'; else {
				printf ("ReWireRegisterMidi, expected string here: %s\n",str);
			}
			#ifdef MIDIVERBOSE
			printf ("device name is :%s:\n",str);
			#endif
			encodedDeviceName = ReWireNameIndex(str);
			str = EOT+1;
			sscanf (str, "%d %d",&curBus, &curChannel);


		} else if (*str == '\t') {
			str++;
			sscanf (str, "%d", &curController);
			EOT = strchr (str,'"');
			if (EOT != NULL) str = EOT+1; else {
				printf ("ReWireRegisterMidi, expected string here: %s\n",str);
			}
			EOT = strchr (str,'"');
			if (EOT != NULL) *EOT = '\0'; else {
				printf ("ReWireRegisterMidi, expected string here: %s\n",str);
			}
			#ifdef MIDIVERBOSE
			printf ("	controllername is :%s:\n",str);
			#endif
			encodedControllerName = ReWireNameIndex(str);
			str = EOT+1;
			sscanf (str, "%d %d %d",&curMin, &curMax, &curType);

			#ifdef MIDIVERBOSE
			printf ("dev (%d %d)  %d %d chan %d %d %d %d\n",encodedDeviceName, encodedControllerName, curBus, curChannel,
				curController, curMin, curMax, curType);
			#endif
		

			/* register the info for this controller */
			if (!ReWireDeviceRegister(encodedDeviceName, encodedControllerName, &curBus, 
				&curChannel, &curController, &curMin, &curMax, &curType)) {
				#ifdef MIDIVERBOSE
				printf ("ReWireRegisterMIDI, duplicate device for %s %s\n",
					ReWireNamenames[encodedDeviceName].name, ReWireNamenames[encodedControllerName].name); 
				#endif
			}

		} else {
			if (*str != ' ') printf ("ReWireRegisterMidi - garbage (%c)  at:%s\n",*str,str);
		}
		
		/* skip to the end of the line */
		while (*str >= ' ') str++;
		while (*str == '\n') str++;
	}
}



void do_MidiControl (void *this) {
	struct X3D_MidiControl* node;
	int value;
	float fV;
	int sendEvent;

	node = (struct X3D_MidiControl*) this;

	#ifdef MIDIVERBOSE
	printf ("do MidiControl for node %s\n",stringNodeType(node->_nodeType));
	#endif
	if (NODE_MidiControl == node->_nodeType) {
		#ifdef MIDIVERBOSE
		printf ("ReWire change %d %d ", node->_ichange, node->_change); 

		printf ("bus %d channel :%s: controllerType :%s: device :%s: ",
			node->_bus, node->channel->strptr, node->controllerType->strptr, node->deviceName->strptr);
		printf (" devp %d fv %f iv %d hr %d ct %d intVal %d max %d min %d\n",
			node->controllerPresent, node->floatValue, node->intValue, node->highResolution,
			node->intControllerType, node->intValue, node->maxVal, node->minVal);
		#endif

		value = node->intValue;
		fV = node->floatValue;
		if (node->useIntValue) {
			determineMIDIValFromInt (node, &value, &fV);
		} else {
			determineMIDIValFromFloat (node, &value, &fV); 
		}

		/* should we send this event? */
		sendEvent = node->continuousEvents;
		if (value != node->_oldintValue) sendEvent = TRUE;
					
		if (sendEvent) {
			node->intValue = value;
			node->_oldintValue = value;
			node->floatValue = fV;
			mark_event(node,offsetof(struct X3D_MidiControl, intValue));
			mark_event(node,offsetof(struct X3D_MidiControl, floatValue));
			sendNodeToReWire(node);
			update_node (node);

			#ifdef MIDIVERBOSE
			printf ("intValue changed - now is %d\n",node->intValue); 
			printf ("sending %d %f ",value, node->floatValue);
			printf ("mins %d %d maxs %d %d ",node->deviceMinVal, node->minVal, node->deviceMaxVal, node->maxVal);
			printf ("float %f node->floatVal\n",node->floatValue);
			#endif
		}
	}	
}


/* a MIDI event is coming in from the EAI port! */
ReWireMIDIEvent (char *line) {
	long int timeDiff;
	int bus, channel, controller, value;
	int rv;
	int ctr;
	struct X3D_MidiControl *node;
	float fV;
	int sendEvent;

	if ((rv=sscanf (line, "%ld %d %d %d %d",&timeDiff, &bus, &channel, &controller, &value)) != 5) {
		printf ("Error (%d)on reading MIDIEVIN, line %s\n",rv,line);
		return;
	}


	for (ctr=0; ctr<=ReWireDevicetableSize; ctr++) {
		if ((bus==ReWireDevices[ctr].bus) &&
			(channel == ReWireDevices[ctr].channel) &&
			(controller == ReWireDevices[ctr].controller)) {
			#ifdef MIDIVERBOSE
				printf ("ReWireDeviceIndex, FOUND IT at %d\n",ctr);
			#endif

			if (ReWireDevices[ctr].node == NULL) {
				printf ("ReWireMidiEvent, node for %d is still NULL\n",ctr);
			} else {
				node = ReWireDevices[ctr].node;
				#ifdef MIDIVERBOSE
				printf ("routing to device %s, channel %s ",node->deviceName->strptr, node->channel->strptr);
				printf (" nameIndex %d channelIndex %d deviceMinVal %d deviceMaxVal %d minVal %d maxVal%d\n",
					node->_deviceNameIndex, node->_channelIndex, node->deviceMinVal,
					node->deviceMaxVal, node->minVal, node->maxVal);
				#endif

				determineMIDIValFromInt (node, &value, &fV); 
				sendEvent = node->continuousEvents;
				if (value != node->_oldintValue) sendEvent = TRUE;
					
				if (sendEvent) {
					node->intValue = value;
					node->_oldintValue = value;
					node->floatValue = fV;
					mark_event(node,offsetof(struct X3D_MidiControl, intValue));
					mark_event(node,offsetof(struct X3D_MidiControl, floatValue));
				}

			}
		}
	}
}


/************ END OF MIDI NODE **************************/


void render_LoadSensor (struct X3D_LoadSensor *node) {
	int total;
	int count;
	int nowLoading;
	int nowFinished;
	struct X3D_ImageTexture *tnode;
	struct X3D_MovieTexture *mnode;
	struct X3D_AudioClip *anode;
	struct X3D_Inline *inode;
	
	/* if not enabled, do nothing */
	if (!node) return;
	if (!node->enabled) return;

	/* we only need to look at this during the rendering pass - once per event loop */
	if (!render_geom) return;

	/* do we need to re-generate our internal variables? */
	if (node->_change != node->_ichange) {
		node->_ichange = node->_change;
		node->__loading = 0;
		node->__finishedloading = 0;
		node->progress = 0.0;
		node->__StartLoadTime = 0.0;
	}

	/* do we actually have any nodes to watch? */
	if (node->watchList.n<=0) return;

	/* are all nodes loaded? */
	if (node->__finishedloading == node->watchList.n) return;

	/* our current status... */
	nowLoading = 0;
	nowFinished = 0;

	/* go through node list, and check to see what the status is */
	/* printf ("have %d nodes to watch\n",node->watchList.n); */
	for (count = 0; count < node->watchList.n; count ++) {

		tnode = (struct X3D_ImageTexture *) node->watchList.p[count];

		/* printf ("node type of node %d is %d\n",count,tnode->_nodeType); */
		switch (tnode->_nodeType) {
		case NODE_ImageTexture:
			/* printf ("opengl tex is %d\n",tnode->__texture); */
			/* is this texture thought of yet? */
			nowLoading++;
			if (isTextureLoaded(tnode->__textureTableIndex)) {
				/* is it finished loading? */
				nowFinished ++;
			}
				
			break;

		case NODE_MovieTexture:
			mnode = (struct X3D_MovieTexture *) tnode; /* change type to MovieTexture */
			/* printf ("opengl tex is %d\n",mnode->__texture0_); */
			/* is this texture thought of yet? */
			if (mnode->__texture0_ > 0) {
				nowLoading++;
				/* is it finished loading? */
				if (isTextureLoaded(mnode->__texture0_)) nowFinished ++;
			}
				
			break;

		case NODE_Inline:
			inode = (struct X3D_Inline *) tnode; /* change type to Inline */
			printf ("LoadSensor, Inline %d, type %d loadstatus %d at %d\n",inode,inode->_nodeType,inode->__loadstatus, &inode->__loadstatus);
			break;

		case NODE_Script:
			nowLoading ++; /* broken - assume that the url is ok for now */
			break;

		case NODE_AudioClip:
			anode = (struct X3D_AudioClip *) tnode; /* change type to AudioClip */
			/* AudioClip sourceNumber will be gt -1 if the clip is ok. see code for details */
			if (anode->__sourceNumber > -1) nowLoading ++;

			break;

		default :{} /* there should never be anything here, but... */
		}
	}
		

	/* ok, are we NOW finished loading? */
	if (nowFinished == node->watchList.n) {
		node->isActive = 0;
		mark_event (node, offsetof (struct X3D_LoadSensor, isActive));

		node->isLoaded = 1;
		mark_event (node, offsetof (struct X3D_LoadSensor, isLoaded));

		node->progress = 1.0;
		mark_event (node, offsetof (struct X3D_LoadSensor, progress));

		node->loadTime = TickTime;
		mark_event (node, offsetof (struct X3D_LoadSensor, loadTime));
	}	

	/* have we NOW started loading? */
	if ((nowLoading > 0) && (node->__loading == 0)) {
		/* mark event isActive TRUE */
		node->isActive = 1;
		mark_event (node, offsetof (struct X3D_LoadSensor, isActive));

	
		node->__StartLoadTime = TickTime;
	}
	
	/* what is our progress? */
	if (node->isActive == 1) {
		node->progress = (float)(nowFinished)/(float)(node->watchList.n);
		mark_event (node, offsetof (struct X3D_LoadSensor, progress));
	}

	/* remember our status for next time. */
	node->__loading = nowLoading;
	node->__finishedloading = nowFinished;

	/* did we run out of time? */
	if (node->timeOut > 0.0001) {			/* we have a timeOut specified */
		if (node->__StartLoadTime > 0.001) {	/* we have a start Time recorded from the isActive = TRUE */
		
			/* ok, we should look at time outs */
			if ((TickTime - node->__StartLoadTime) > node->timeOut) {
				node->isLoaded = 0;
				mark_event (node, offsetof (struct X3D_LoadSensor, isLoaded));

				node->isActive = 0;
				mark_event (node, offsetof (struct X3D_LoadSensor, isActive));

				/* and, we will just assume that we have loaded everything next iteration */
				node->__finishedloading = node->watchList.n;
			}
		}
	}
}


void child_Anchor (struct X3D_Anchor *node) {
	int nc = (node->children).n;
	DIRECTIONAL_LIGHT_SAVE


	/* any children at all? */
	if (nc==0) return;

	#ifdef CHILDVERBOSE
	printf("RENDER ANCHOR START %d (%d)\n",node, nc);
	#endif

	/* do we have to sort this node? */
	if ((nc > 1 && !render_blend)) sortChildren(node->children);

	/* do we have a DirectionalLight for a child? */
	if(node->has_light) dirlightChildren(node->children);

	/* now, just render the non-directionalLight children */
	normalChildren(node->children);

	if (render_geom && (!render_blend)) {
		EXTENTTOBBOX
		BOUNDINGBOX
	}

	#ifdef CHILDVERBOSE
	printf("RENDER ANCHOR END %d\n",node);
	#endif

	DIRECTIONAL_LIGHT_OFF
}


void child_Inline (struct X3D_Inline *node) {
	int nc = (node->__children).n;
	DIRECTIONAL_LIGHT_SAVE

	#ifdef CHILDVERBOSE
	printf("RENDER INLINE START %d (%d)\n",node, nc);
	#endif

	/* lets see if we still have to load this one... */
	if ((node->__loadstatus)==0) loadInline(node);


	/* any children at all? */
	if (nc==0) return; 

	/* do we have to sort this node? */
	if ((nc > 1 && !render_blend)) sortChildren(node->__children);

	/* do we have a DirectionalLight for a child? */
	if(node->has_light) dirlightChildren(node->__children);

	/* now, just render the non-directionalLight children */
	normalChildren(node->__children);

	if (render_geom && (!render_blend)) {
		EXTENTTOBBOX
                /* pass the bounding box calculations on up the chain */
                propagateExtent((struct X3D_Box *)node);

		BOUNDINGBOX
	}

	#ifdef CHILDVERBOSE
	printf("RENDER INLINE END %d\n",node);
	#endif

	DIRECTIONAL_LIGHT_OFF
}

void changed_Anchor (struct X3D_Anchor *node) {
                int i;
                int nc = ((node->children).n);
                struct X3D_Box *p;
                struct X3D_Virt *v;

		INITIALIZE_EXTENT
		DIRECTIONAL_LIGHT_FIND
}
