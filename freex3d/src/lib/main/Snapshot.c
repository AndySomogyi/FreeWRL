/*
=INSERT_TEMPLATE_HERE=

$Id$

CProto ???

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
#include "headers.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../world_script/jsUtils.h"
#include "../world_script/CScripts.h"
#include "Snapshot.h"
#include "../scenegraph/Collision.h"
#include "../scenegraph/quaternion.h"
#include "../scenegraph/Viewer.h"
#include "../input/SensInterps.h"
#include "../x3d_parser/Bindable.h"

#if HAVE_DIRENT_H
# include <dirent.h>
#endif


/* snapshot stuff */
int snapRawCount=0;
int snapGoodCount=0;

#if defined(DOSNAPSEQUENCE)
/* need to re-implement this for OSX generating QTVR */
int snapsequence=FALSE;		/* --seq - snapshot sequence, not single click  */
int maxSnapImages=100; 		/* --maximg command line parameter 		*/
char *snapseqB = NULL;		/* --seqb - snap sequence base filename		*/
#endif

int snapGif = FALSE;		/* --gif save as an animated GIF, not mpg	*/
char *snapsnapB = NULL;		/* --snapb -single snapshot files		*/
const char default_seqtmp[] = "freewrl_tmp"; /* default value for seqtmp        */
char *seqtmp = NULL;		/* --seqtmp - directory for temp files		*/
int doSnapshot = FALSE;		/* are we doing a snapshot?			*/
int doPrintshot = FALSE; 	/* are we taking a snapshot in order to print? */
int savedSnapshot = FALSE;

#ifdef DOSNAPSEQUENCE
/* need to re-implement this for OSX generating QTVR */
void saveSnapSequence();
#endif

#if defined( WIN32) || defined (IPHONE)
/* stubbs for now */
void setSnapshot() {}
void Snapshot () {}
void setSnapGif(){}
void saveSnapSequence() {}


#else /*ifdef win32*/

void setSnapGif()
{
    snapGif = TRUE;
}

void setPrintShot() {
	doPrintshot = TRUE;
	savedSnapshot = doSnapshot;
	doSnapshot = TRUE;
	printf("setting printshot/ snapshot\n");
}

/* turn snapshotting on; if sequenced; possibly turn off an convert sequence */
void setSnapshot() {
#ifdef DOSNAPSEQUENCE
/* need to re-implement this for OSX generating QTVR */
	if (!doSnapshot) {
		doSnapshot = TRUE;
	} else {
		if (snapsequence) {
			doSnapshot = FALSE;
			saveSnapSequence();
		}
	}
#else
	doSnapshot = ! doSnapshot;
#endif
}

#ifdef DOSNAPSEQUENCE
/* need to re-implement this for OSX generating QTVR */

/* convert a sequence of snaps into a movie */
void saveSnapSequence() {
		char *mytmp, *myseqb;
		char sysline[2000];
		char thisRawFile[2000];
		char thisGoodFile[2000];
		int xx;
	
		/* make up base names - these may be command line parameters */
	        if (snapseqB == NULL)  myseqb  = "freewrl.seq";
	        else myseqb = snapseqB;
	        if (seqtmp == NULL)    mytmp   = "freewrl_tmp";
	        else mytmp = seqtmp;
	
		snapGoodCount++;
	
		if (snapGif) {
			sprintf (thisGoodFile,"%s/%s.%04d.gif",mytmp,myseqb,snapGoodCount);
		} else {
			sprintf (thisGoodFile,"%s/%s.%04d.mpg",mytmp,myseqb,snapGoodCount);
		}
		/* sprintf(sysline,"%s -size %dx%d -depth 8 -flip %s/%s*rgb %s", */
	
		/* Dani Rozenbaum - In order to generate 
		 movies (e.g. with mencoder) images have to be three-band RGB (in other 
		 words 24-bits) */
	sprintf(sysline, "%s -size %dx%d -depth 24 -colorspace RGB +matte -flip %s/%s*rgb %s",
			CONVERT, screenWidth, screenHeight,mytmp,myseqb,thisGoodFile);
	
		/* printf ("convert line %s\n",sysline); */
	
		if (system (sysline) != 0) {
			printf ("Freewrl: error running convert line %s\n",sysline);
		}
		printf ("snapshot is :%s\n",thisGoodFile);
		/* remove temporary files */
		for (xx=1; xx <= snapRawCount; xx++) {
			sprintf (thisRawFile, "%s/%s.%04d.rgb",mytmp,myseqb,xx);
			UNLINK (thisRawFile);
		}
		snapRawCount=0;
}
#endif


#ifdef AQUA

CGContextRef MyCreateBitmapContext(int pixelsWide, int pixelsHigh, unsigned char *buffer) { 
	CGContextRef context=NULL; 
	CGColorSpaceRef colorSpace; 
	unsigned char* bitmapData; 
	int bitmapByteCount; 
	int bitmapBytesPerRow; 
	int i;

	bitmapBytesPerRow =(pixelsWide*4); 
	bitmapByteCount =(bitmapBytesPerRow*pixelsHigh); 
	colorSpace=CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB); 
	bitmapData=(unsigned char*) malloc(bitmapByteCount); 

	if(bitmapData==NULL) 
	{ 
		fprintf(stderr,"Memorynotallocated!"); 
		return NULL; 
	} 

	/* copy the saved OpenGL data, but, invert it */
	for (i=0; i<pixelsHigh; i++) {
		memcpy (&bitmapData[i*bitmapBytesPerRow], 
			&buffer[(pixelsHigh-i-1)*bitmapBytesPerRow], 
			bitmapBytesPerRow);
	}

	context=CGBitmapContextCreate(bitmapData, 
		pixelsWide, 
		pixelsHigh, 
		8, // bits per component 
		bitmapBytesPerRow, 
		colorSpace, 
		kCGImageAlphaPremultipliedLast); 
	if (context== NULL) 
	{ 
		free (bitmapData); 
		fprintf (stderr, "Context not created!"); 
		return NULL; 
	} 
	CGColorSpaceRelease( colorSpace ); 
	return context; 
} 
#endif

/* get 1 frame; convert if we are doing 1 image at a time */
void Snapshot () {
	GLvoid *buffer;
	DIR *mydir;
	
	#ifndef AQUA
	char sysline[2000];
	FILE * tmpfile;
	char thisRawFile[2000];
	#endif

	char thisGoodFile[2000];
	char *mytmp, *mysnapb;

	#ifdef AQUA
        CFStringRef     path;
        CFURLRef        url;
	CGImageRef	image;
	CGImageDestinationRef imageDest;
	CGRect myBoundingBox; 
	CGContextRef myBitmapContext;
	#endif
	
	printf("do Snapshot ... \n");
	/* make up base names - these may be command line parameters */
	
#ifdef DOSNAPSEQUENCE
/* need to re-implement this for OSX generating QTVR */

	if (snapsequence) {
	        if (snapseqB == NULL)
	                mysnapb  = "freewrl.seq";
	        else
	                mysnapb = snapseqB;
	} else {
#endif
	        if (snapsnapB == NULL)
	                mysnapb = "freewrl.snap";
	        else
	                mysnapb = snapsnapB;
#ifdef DOSNAPSEQUENCE
/* need to re-implement this for OSX generating QTVR */

	}
#endif
	
	
	if (seqtmp == NULL)    mytmp   = "freewrl_tmp";
	else mytmp = seqtmp;
	
	/*does the directory exist? */
	if ((mydir = opendir(mytmp)) == NULL) {
		mkdir (mytmp,0755);
		if ((mydir = opendir(mytmp)) == NULL) {
			ConsoleMessage ("error opening Snapshot directory %s\n",mytmp);
			return;
		}
	}
	
#ifdef DOSNAPSEQUENCE
/* need to re-implement this for OSX generating QTVR */

	/* are we sequencing, or just single snapping? */
	if (!snapsequence) doSnapshot=FALSE;  	/* reset snapshot key */
#endif

	
	#ifdef AQUA	
		/* OSX needs 32 bits per byte. */
		/* MALLOC 4 bytes per pixel */
		buffer = MALLOC (4*screenWidth*screenHeight*sizeof(char));
	
		/* grab the data */
		FW_GL_PIXELSTOREI (GL_UNPACK_ALIGNMENT, 1);
		FW_GL_PIXELSTOREI (GL_PACK_ALIGNMENT, 1);
		FW_GL_READPIXELS (0,0,screenWidth,screenHeight,GL_RGBA,GL_UNSIGNED_BYTE, buffer);
	#else	
		/* Linux, etc, can get by with 3 bytes per pixel */
		/* MALLOC 3 bytes per pixel */
		buffer = MALLOC (3*screenWidth*screenHeight*sizeof(char));
	
		/* grab the data */
		FW_GL_PIXELSTOREI (GL_UNPACK_ALIGNMENT, 1);
		FW_GL_PIXELSTOREI (GL_PACK_ALIGNMENT, 1);
		FW_GL_READPIXELS (0,0,screenWidth,screenHeight,GL_RGB,GL_UNSIGNED_BYTE, buffer);
	#endif
	
	/* save this snapshot */
	snapRawCount ++;
#ifdef DOSNAPSEQUENCE
/* need to re-implement this for OSX generating QTVR */

	if (snapRawCount > maxSnapImages) {
		FREE_IF_NZ (buffer);
		return;
	}
#endif

	#ifdef AQUA

		myBoundingBox = CGRectMake (0, 0, screenWidth, screenHeight); 
		myBitmapContext = MyCreateBitmapContext (screenWidth, screenHeight,buffer); 

		image = CGBitmapContextCreateImage (myBitmapContext); 
		CGContextDrawImage(myBitmapContext, myBoundingBox, image); 
		char *bitmapData = CGBitmapContextGetData(myBitmapContext); 

		CGContextRelease (myBitmapContext); 
		if (bitmapData) free(bitmapData); 


		snapGoodCount++;
		if (doPrintshot) {
			sprintf (thisGoodFile, "/tmp/FW_print_snap_tmp.png");
			doPrintshot = FALSE;
			doSnapshot = savedSnapshot;
	        	path = CFStringCreateWithCString(NULL, thisGoodFile, kCFStringEncodingUTF8); 
			printf("thisGoodFile is %s\n", thisGoodFile);
	        	url = CFURLCreateWithFileSystemPath (NULL, path, kCFURLPOSIXPathStyle, FALSE);
		} else {
			sprintf (thisGoodFile,"%s/%s.%04d.png",mytmp,mysnapb,snapGoodCount);

	        	path = CFStringCreateWithCString(NULL, thisGoodFile, kCFStringEncodingUTF8); 
	        	url = CFURLCreateWithFileSystemPath (NULL, path, kCFURLPOSIXPathStyle, FALSE);
		}

		imageDest = CGImageDestinationCreateWithURL(url, CFSTR("public.png"), 1, NULL);
		CFRelease(url);
		CFRelease(path);

		if (!imageDest) {
			ConsoleMessage("Snapshot can not be written");
			return;
		}

		CGImageDestinationAddImage(imageDest, image, NULL);
		if (!CGImageDestinationFinalize(imageDest)) {
			ConsoleMessage ("Snapshot can not be written, part 2");
		}
		CFRelease(imageDest);
		CGImageRelease(image); 
	#else	
		/* save the file */
		sprintf (thisRawFile,"%s/%s.%04d.rgb",mytmp,mysnapb,snapRawCount);
		tmpfile = fopen(thisRawFile,"w");
		if (tmpfile == NULL) {
			printf ("can not open temp file (%s) for writing\n",thisRawFile);
			FREE_IF_NZ (buffer);
			return;
		}
	
		if (fwrite(buffer, 1, screenHeight*screenWidth*3, tmpfile) <= 0) {
			printf ("error writing snapshot to %s, aborting snapshot\n",thisRawFile);
			FREE_IF_NZ (buffer);
			return;
		}
		fclose (tmpfile);
	
		/* convert -size 450x300 -depth 8 -flip /tmp/snappedfile.rgb out.png works. */
	
		FREE_IF_NZ (buffer);
	
#ifdef DOSNAPSEQUENCE
/* need to re-implement this for OSX generating QTVR */

		/* now, if we are doing only 1, convert the raw into the good.... */
		if (!snapsequence) {
#endif
			snapGoodCount++;
			sprintf (thisGoodFile,"%s/%s.%04d.png",mytmp,mysnapb,snapGoodCount);
			sprintf(sysline,"%s -size %dx%d -depth 8 -flip %s %s",
			IMAGECONVERT,screenWidth, screenHeight,thisRawFile,thisGoodFile);
	
			if (system (sysline) != 0) {
				printf ("Freewrl: error running convert line %s\n",sysline);
			}
			printf ("snapshot is :%s\n",thisGoodFile);
			UNLINK (thisRawFile);
#ifdef DOSNAPSEQUENCE
/* need to re-implement this for OSX generating QTVR */

		}
#endif
	#endif
}
#endif /*ifdef win32*/
