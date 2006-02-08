/*******************************************************************
 *  Copyright (C) 2004 John Stewart, CRC Canada.
 *  DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 *  See the GNU Library General Public License (file COPYING in the distribution)
 *  for conditions of use and redistribution.
 **********************************************************************/

#include <stdlib.h>
#include "headers.h"
#include "Bindable.h"
#include "PluginSocket.h"
#include "Viewer.h"
extern unsigned _fw_instance;

/* get all system commands, and pass them through here. What we do
 * is take parameters and execl them, in specific formats, to stop
 * people (or, to try to stop) from typing malicious code. */

/* keep a list of children; if one hangs, doQuit will hang, also. */
#define MAXPROCESSLIST 128
pid_t childProcess[MAXPROCESSLIST];
int lastchildProcess = 0;
int childProcessListInit = FALSE;

void killErrantChildren(void) {
	int count;
	
	for (count = 0; count < MAXPROCESSLIST; count++) {
		if (childProcess[count] != 0) {
			/* printf ("trying to kill %d\n",childProcess[count]); */
			kill (childProcess[count],SIGINT);
		}
	}
}

int freewrlSystem (char *sysline) {

#define MAXEXECPARAMS 10
#define EXECBUFSIZE	2000
	char *paramline[MAXEXECPARAMS];
	char buf[EXECBUFSIZE];
	char *internbuf;
	int count;
	/* pid_t childProcess[lastchildProcess]; */
	int pidStatus;

	int waitForChild;
	int haveXmessage;


	/* make all entries in the child process list = 0 */
	if (childProcessListInit == FALSE) {
		for (count=0; count<MAXPROCESSLIST; count++) {
			childProcess[count] = 0;
		}
		childProcessListInit = TRUE;
	}
	
		
	waitForChild = TRUE;
	haveXmessage = !strncmp(sysline,XMESSAGE,strlen(XMESSAGE));

	internbuf = buf;

	/* bounds check */
	if (strlen(sysline)>=EXECBUFSIZE) return FALSE;
	strcpy (buf,sysline);

	/* printf ("freewrlSystem, have %s here\n",internbuf);  */
	for (count=0; count<MAXEXECPARAMS; count++) paramline[count] = NULL;
	count = 0;

	/* do we have a console message - (which is text with spaces) */
	if (haveXmessage) {
		paramline[0] = XMESSAGE;
		paramline[1] = strchr(internbuf,' ');
		count = 2;
	} else {
		/* split the command off of internbuf, for execing. */
		while (internbuf != NULL) {
			/* printf ("freewrlSystem: looping, count is %d\n",count); */
			paramline[count] = internbuf;
			internbuf = strchr(internbuf,' ');
			if (internbuf != NULL) {
				/* printf ("freewrlSystem: more strings here! :%s:\n",internbuf); */
				*internbuf = '\0';
				/* printf ("param %d is :%s:\n",count,paramline[count]);*/
				internbuf++;
				count ++;
				if (count >= MAXEXECPARAMS) return -1; /*  never...*/
			}
		}
	}
	
	/* printf ("freewrlSystem: finished while loop, count %d\n",count); */
	/* { int xx;
		for (xx=0; xx<MAXEXECPARAMS;xx++) {
			printf ("item %d is :%s:\n",xx,paramline[xx]);
	}} */
	


	if (haveXmessage) {
		waitForChild = FALSE;
	} else {
		/* is the last string "&"? if so, we don't need to wait around */
		if (strncmp(paramline[count],"&",strlen(paramline[count])) == 0) {
			waitForChild=FALSE;
			paramline[count] = '\0'; /*  remove the ampersand.*/
		}
	}

	if (count > 0) {
		switch (childProcess[lastchildProcess]=fork()) {
			case -1:
				perror ("fork"); exit(1);

			case 0: {
			int Xrv;

			/* child process */
			/* printf ("freewrlSystem: child execing, pid %d %d\n",childProcess[lastchildProcess], getpid()); */
		 	Xrv = execl(paramline[0],
				paramline[0],paramline[1], paramline[2],
				paramline[3],paramline[4],paramline[5],
				paramline[6],paramline[7]);
			printf ("FreeWRL: Fatal problem execing %s\n",paramline[0]); 
			exit (Xrv);
			}
			default: {
			/* parent process */
			/* printf ("freewrlSystem: parent waiting for child %d\n",childProcess[lastchildProcess]); */

			lastchildProcess++;
			if (lastchildProcess == MAXPROCESSLIST) lastchildProcess=0;


			/* do we have to wait around? */
			if (!waitForChild) {
				/* printf ("freewrlSystem - do not have to wait around\n"); */
				return 0;
			}
			waitpid (childProcess[lastchildProcess],&pidStatus,0);
			/* printf ("freewrlSystem: parent - child finished - pidStatus %d \n",
			 		pidStatus); */
			}
		}
		return pidStatus;
	} else {
		printf ("System call failed :%s:\n",sysline);
	}
	return -1;
}

/* implement Anchor/Browser actions */

int checkIfX3DVRMLFile(char *fn);
void Anchor_ReplaceWorld (char *fn);

void doBrowserAction () {
	int count;
	STRLEN xx;

	int localNode;
	int tableIndex;

	char *filename;
	char *mypath;
	char *thisurl;
	char *slashindex;
	int flen;
	char firstBytes[4];
	char sysline[1000];


	struct Multi_String Anchor_url;
	/* struct Multi_String { int n; SV * *p; };*/

	Anchor_url = AnchorsAnchor->url;

	if (!RUNNINGASPLUGIN)
		printf ("FreeWRL::Anchor: going to \"%s\"\n",
			SvPV(AnchorsAnchor->description,xx));

	filename = (char *)malloc(1000);

	/* lets make up the path and save it, and make it the global path */
	count = strlen(SvPV(AnchorsAnchor->__parenturl,xx));
	mypath = (char *)malloc ((sizeof(char)* count)+1);

	if ((!filename) || (!mypath)) {
		outOfMemory("Anchor can not malloc for filename\n");
	}

	/* copy the parent path over */
	strcpy (mypath,SvPV(AnchorsAnchor->__parenturl,xx));

	/* and strip off the file name, leaving any path */
	slashindex = (char *)rindex(mypath,'/');
	if (slashindex != NULL) {
		slashindex ++; /* leave the slash on */
		*slashindex = 0;
	 } else {mypath[0] = 0;}

	/* printf ("Anchor, url so far is %s\n",mypath);*/

	/* try the first url, up to the last */
	count = 0;
	while (count < Anchor_url.n) {
		thisurl = SvPV(Anchor_url.p[count],xx);

		/*  is this a local Viewpoint?*/
		if (thisurl[0] == '#') {
			localNode = EAI_GetViewpoint(&thisurl[1]);
			tableIndex = -1;

			for (flen=0; flen<totviewpointnodes;flen++) {
				if (localNode == viewpointnodes[flen]) {
					 tableIndex = flen;
					 break;
				}
			}
			/*  did we find a match with known Viewpoints?*/
			if (tableIndex>=0) {
				/* unbind current, and bind this one */
				send_bind_to(NODE_Viewpoint,
						(void *)viewpointnodes[currboundvpno],0);
				currboundvpno=tableIndex;
				send_bind_to(NODE_Viewpoint,
						(void *)viewpointnodes[currboundvpno],1);
			} else {
				printf ("failed to match local Viewpoint\n");
			}

			/*  lets get outa here - jobs done.*/
			free (filename);
			return;
		}

		/* check to make sure we don't overflow */
		if ((strlen(thisurl)+strlen(mypath)) > 900) break;

		/* put the path and the file name together */
		makeAbsoluteFileName(filename,mypath,thisurl);

		/* if this is a html page, just assume it's ok. If
		 * it is a VRML/X3D file, check to make sure it exists */

		if (!checkIfX3DVRMLFile(filename)) { break; }

		/* ok, it might be a file we load into our world. */
		if (fileExists(filename,firstBytes,FALSE)) { break; }
		count ++;
	}

	/*  did we locate that file?*/
	if (count == Anchor_url.n) {
		if (count > 0) {
			printf ("Could not locate url (last choice was %s)\n",filename);
		}
		free (filename);
		return;
	}
	/* printf ("we were successful at locating :%s:\n",filename);*/

	/* which browser are we running under? if we are running as a*/
	/* plugin, we'll have some of this information already.*/

	if (checkIfX3DVRMLFile(filename)) {
		Anchor_ReplaceWorld (filename);
	} else {
		/*  We should get the browser to get the new window,*/
		/*  but this code seems to fail in mozilla. So, lets*/
		/*  just open a new browser, and see what happens...*/
		/* if (RUNNINGASPLUGIN) {*/
		/* 	printf ("Anchor, running as a plugin - load non-vrml file\n");*/
		/* 	requestNewWindowfromPlugin(_fw_FD,_fw_instance,filename);*/
		/* } else {*/
			/* printf ("IS NOT a vrml/x3d file\n");*/
			/* printf ("Anchor: -DBROWSER is :%s:\n",BROWSER);*/


			char *browser = getenv("BROWSER");
			if (browser)
				strcpy (sysline, browser);
			else
				strcpy (sysline, BROWSER);
			strcat (sysline, " ");
			strcat (sysline, filename);
			strcat (sysline, " &");
			freewrlSystem (sysline);
		/* }*/
	}
	free (filename);
}



/*
 * Check to see if the file name is a geometry file.
 * return TRUE if it looks like it is, false otherwise
 *
 * We cant use the firstbytes of fileExists, because, we may be
 * trying to get a whole web-page for reload, and lets let the
 * browser adequately resolve that one.
 *
 * This should be kept in line with the plugin register code in
 * Plugin/netscape/source/npfreewrl.c
 */

int checkIfX3DVRMLFile(char *fn) {
	if ((strstr(fn,".wrl") > 0) ||
		(strstr(fn,".WRL") > 0) ||
		(strstr(fn,".x3d") > 0) ||
		(strstr(fn,".x3dv") > 0) ||
		(strstr(fn,".x3db") > 0) ||
		(strstr(fn,".X3DV") > 0) ||
		(strstr(fn,".X3DB") > 0) ||
		(strstr(fn,".X3D") > 0)) {
		return TRUE;
	}
	return FALSE;
}

/* we are an Anchor, and we are not running in a browser, and we are
 * trying to do an external VRML or X3D world.
 */

void Anchor_ReplaceWorld (char *filename) {
	int tmp;

	kill_oldWorld(TRUE,TRUE,TRUE);

	perlParse(FROMURL, filename,TRUE,FALSE,
		rootNode, offsetof (struct X3D_Group, children),&tmp,
		TRUE);
}



#include <ctype.h>

/* send in a 0 to 15, return a char representation */
char tohex (int mychar) {
	if (mychar <10) return (mychar + '0');
	else return (mychar - 10 + 'A');
}

/* should this character be encoded? */
int URLmustEncode(int ch) {
	if (
		   (ch == 0x20) 
		|| (ch == 0x22) 
		|| (ch == 0x3c) 
		|| (ch == 0x3e) 
		|| (ch == 0x23) 
		|| (ch == 0x25)
		|| (ch == 0x7b) 
		|| (ch == 0x7d) 
		|| (ch == 0x7c) 
		|| (ch == 0x5c) 
		|| (ch == 0x5e) 
		|| (ch == 0x7e) 
		|| (ch == 0x5b) 
		|| (ch == 0x5d) 
		|| (ch == 0x60)) return TRUE;
	return FALSE;
} 


/***************/
static FILE * tty = NULL;

/* prints to a log file if we are running as a plugin */
void URLprint (const char *m, const char *p) {
#ifdef URLPRINTDEBUG
	if (tty == NULL) {
		tty = fopen("/home/luigi/logURLencod", "w");
		if (tty == NULL)
			abort();
		fprintf (tty, "\nplugin restarted\n");
	}

	fprintf(tty, m,p);
	fflush(tty);
#endif
}

/* loop about waiting for the Browser to send us some stuff. */
/* Change a string to encode spaces for getting URLS. */
void URLencod (char *dest, char *src, int maxlen) {
	int mylen;
	int sctr;
	int destctr;
	int curchar;

	/* get the length of the source and bounds check */
	URLprint ("going to start URLencod %s\n","on a string");
	URLprint ("start, src is %s\n",src);
	URLprint ("maxlen is %d\n",maxlen);

	destctr = 0; /* ensure we dont go over dest length */
	mylen = strlen(src);
	if (mylen == 0) {
		dest[0]= '\0';
		return;
	}
	
	/* encode the string */
	for (sctr = 0; sctr < mylen; sctr ++) {
		curchar = (int) *src; src++;	
		/* should we encode this one? */

                if (URLmustEncode(curchar)) { 
			*dest = '%'; dest ++;
			*dest = tohex ((curchar >> 4) & 0x0f); dest++;
			*dest = tohex (curchar & 0x0f); dest++;
			destctr +=3;
		} else {
			*dest = (char) curchar; destctr++; dest++;
		}
	


		/* bounds check */
		if (destctr > (maxlen - 5))  {
			*dest = '\0';
			return;
		}
	}
	*dest = '\0'; /* null terminate this one */
}
