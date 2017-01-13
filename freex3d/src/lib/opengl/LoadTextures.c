/*

  FreeWRL support library.
  New implementation of texture loading.

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

/*
Sept 6, 2016 note:
- looks like for years we have been loading image files into BGRA order in tti (texturetableindexstruct).
- then over in Textures.c for desktop we tell opengl our data is in GL_BGRA order
- and for mobile we tell it its in GL_RGBA order
- and we ask it to store internally in the same GL_RGBA format (so what's the performance gain)

from 2003:
"It was true that BGR most of the times resulted in faster performance.
Even if this would be true right now, there's nothing to worry. 
The video card will put the texture in its own internal format so besides downloading speed to it 
(which will be unnoticeably faster/slower thosedays), 
there should be no problem in using one format or the other."

Options:
1. leave as is -with different platforms loading in different order- and document better
	I'll use // BGRA below to identify code that's swapping to BGRA
2. fix by fixing all loading to do in one order (but GL_BGRA source format isn't available on mobile,
	so that means changing all the desktop loading code to RGBA)
3. fix with a BGRA to RGBA byte swapper, and apply to output of desktop loader code
4. add member to tti struct to say what order the loader used, so textures.c can apply right order if 
	available for the platform

Decision:
#3 - see texture_swap_B_R(tti) calls below, and changes to Textures.c to assume tti is RGBA on all platforms
Dec 6, 2016 tti->data now always in RGBA

*/

#include <config.h>
#include <system.h>
#include <system_threads.h>
#include <display.h>
#include <internal.h>

#include "vrml_parser/Structs.h"
#include "main/ProdCon.h"
#include "OpenGL_Utils.h"
#include "Textures.h"
#include "LoadTextures.h"
#include "../scenegraph/Component_CubeMapTexturing.h"

#include <list.h>
#include <io_files.h>
#include <io_http.h>

#include <threads.h>

#include <libFreeWRL.h>

/* We do not want to include Struct.h: enormous file :) */
typedef struct _Multi_String Multi_String;
void Multi_String_print(struct Multi_String *url);

#ifdef _MSC_VER
#include "ImageLoader.h"
#else
// OLD_IPHONE_AQUA #if !(defined(TARGET_AQUA) || defined(IPHONE) || defined(_ANDROID) || defined(ANDROIDNDK))
#if !(defined(_ANDROID) || defined(ANDROIDNDK))
		#include <Imlib2.h>
	#endif
#endif


// OLD_IPHONE_AQUA #if defined (TARGET_AQUA)
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA #ifdef IPHONE
// OLD_IPHONE_AQUA #include <CoreFoundation/CoreFoundation.h>
// OLD_IPHONE_AQUA #include <CoreGraphics/CoreGraphics.h>
// OLD_IPHONE_AQUA #include <ImageIO/ImageIO.h>
// OLD_IPHONE_AQUA #else
// OLD_IPHONE_AQUA #include <Carbon/Carbon.h>
// OLD_IPHONE_AQUA #include <QuickTime/QuickTime.h>
// OLD_IPHONE_AQUA #endif /* IPHONE */
// OLD_IPHONE_AQUA #endif /* TARGET_AQUA */

///* is the texture thread up and running yet? */
//int TextureThreadInitialized = FALSE;



//GLuint defaultBlankTexture;

typedef struct pLoadTextures{
	s_list_t* texture_request_list;// = NULL;
	bool loader_waiting;// = false;
	/* list of texture table entries to load */
	s_list_t *texture_list;// = NULL;
	/* are we currently active? */
	int TextureParsing; // = FALSE;
}* ppLoadTextures;
void *LoadTextures_constructor(){
	void *v = MALLOCV(sizeof(struct pLoadTextures));
	memset(v,0,sizeof(struct pLoadTextures));
	return v;
}
void LoadTextures_init(struct tLoadTextures *t)
{
	//public
	/* is the texture thread up and running yet? */
	//t->TextureThreadInitialized = FALSE;

	//private
	t->prv = LoadTextures_constructor();
	{
		ppLoadTextures p = (ppLoadTextures)t->prv;
		p->texture_request_list = NULL;
		p->loader_waiting = false;
		/* list of texture table entries to load */
		p->texture_list = NULL;
		/* are we currently active? */
		p->TextureParsing = FALSE;
	}
}
//s_list_t* texture_request_list = NULL;
//bool loader_waiting = false;

enum {
	IMAGETYPE_UNKNOWN = 0,
	IMAGETYPE_PNG = 1,
	IMAGETYPE_JPEG,
	IMAGETYPE_GIF,
	IMAGETYPE_DDS,
	IMAGETYPE_WEB3DIT,
	IMAGETYPE_NRRD,
	IMAGETYPE_VOL,
};
static int sniffImageFileHeader(char *filename) {
// return value:
// 0 unknown
// 1 png
// 2 jpeg
// 3 gif
//filenames coming in can be temp file names - scrambled
//there are 3 ways to tell in the backend what type of image file:
//a) .xxx original filename suffix
//b) MIME type 
//c) file signature https://en.wikipedia.org/wiki/List_of_file_signatures
// right now we aren't passing in the .xxx or mime or signature bytes
// except through the file conents we can get the signature
	char header[20];
	int iret;
	FILE* fp = fopen(filename,"rb");
	fread(header,20,1,fp);
	fclose(fp);

	iret = IMAGETYPE_UNKNOWN;
	if(!strncmp(&header[1],"PNG",3))
		iret = IMAGETYPE_PNG;

	if(!strncmp(header,"���",3)) //JPEG
		iret = IMAGETYPE_JPEG;

	if(!strncmp(header,"GIF",3))
		iret = IMAGETYPE_GIF;

	if(!strncmp(header,"DDS ",4)) // MS .dds cubemap and 3d textures
		iret = IMAGETYPE_DDS;

	if(!strncmp(header,"web3dit",7)) //.web3dit dug9/freewrl invention
		iret = IMAGETYPE_WEB3DIT;

	if(!strncmp(header,"NRRD",4))  //.nrrd 3D volume texture
		iret = IMAGETYPE_NRRD;

	if(!strncmp(header,"vol",3)) //.vol 3D volume
		iret = IMAGETYPE_VOL;

	return iret;
}

static int sniffImageChannels_bruteForce(unsigned char *imageblob, int width, int height){
	//iterates over entire 4byte-per-pixel RGBA image blob, or until it knows the answer,
	// and returns number of channels 1=Luminance, 2=Lum-alpha 3=rgb 4=rgba
	//detects by comparing alpha != 1 to detect alpha, and r != g != b to detect color
	int i,ii4,j,jj4, hasAlpha, hasColor, channels;
	hasAlpha = 0;
	hasColor = 0;
	channels = 4;
	for(i=0;i<height;i++){
		ii4 = i*width*4;
		if(!hasColor){
			//for gray-scale images, will need to scan the whole image looking for r != g != b
			//not tested with lossy compression ie jpeg, but jpeg is usually RGB -not gray, and no alpha- 
			// - so jpeg should exit color detection early anyway
			for(j=0;j<width;j++){
				jj4 = ii4 + j*4;
				hasAlpha = hasAlpha || imageblob[jj4+3] != 255;
				hasColor = hasColor || imageblob[jj4] != imageblob[jj4+1] || imageblob[jj4+1] != imageblob[jj4+2];
			}
		}else{
			//color found, can stop looking for color. now just look for alpha
			//- this is likely the most work, because if Alpha all 1s, it won't know until it scans whole image
			for(j=3;j<width*4;j+=4){
				hasAlpha = hasAlpha || imageblob[ii4 + j] != 255;
			}
		}
		if(hasAlpha && hasColor)break; //got the maximum possible answer, can exit early
	}
	channels = hasColor ? 3 : 1;
	channels = hasAlpha ? channels + 1 : channels;
	return channels;
}


/* All functions here works with the array of 'textureTableIndexStruct'.
 * In the future we may want to refactor this struct.
 * In the meantime lets make it work :).
 */

#ifdef TEXVERBOSE
static void texture_dump_entry(textureTableIndexStruct_s *entry)
{
	DEBUG_MSG("%s\t%p\t%s\n", texst(entry->status), entry, entry->filename);
}
#endif

void texture_dump_list()
{
#ifdef TEXVERBOSE
	DEBUG_MSG("TEXTURE: wait queue\n");
	ppLoadTextures p = (ppLoadTextures)gglobal()->LoadTextures.prv;
	ml_foreach(p->texture_list, texture_dump_entry(ml_elem(__l)));
	DEBUG_MSG("TEXTURE: end wait queue\n");
#endif
}

static void texture_swap_B_R(textureTableIndexStruct_s* this_tex)
{
	//swap red and blue // BGRA - converts back and forth from BGRA to RGBA 
	//search for GL_RGBA in textures.c
	int x,y,z,i,j,k,ipix,ibyte;
	unsigned char R,B,*data;
	x = this_tex->x;
	y = this_tex->y;
	z = this_tex->z;
	data = this_tex->texdata;
	for(i=0;i<z;i++){
		for(j=0;j<y;j++){
			for(k=0;k<x;k++)
			{
				ipix = (i*y + j)*x + k;
				ibyte = ipix * 4; //assumes tti->texdata is 4 bytes per pixel, in BGRA or RGBA order
				R = data[ibyte];
				B = data[ibyte+2];
				data[ibyte] = B;
				data[ibyte+2] = R;
			}
		}
	}
}

/**
 *   texture_load_from_pixelTexture: have a PixelTexture node,
 *                           load it now.
 */
static void texture_load_from_pixelTexture (textureTableIndexStruct_s* this_tex, struct X3D_PixelTexture *node)
{

/* load a PixelTexture that is stored in the PixelTexture as an MFInt32 */
	int hei,wid,depth;
	unsigned char *texture;
	int count;
	int ok;
	int *iptr;
	int tctr;

	iptr = node->image.p;

	ok = TRUE;

	DEBUG_TEX ("start of texture_load_from_pixelTexture...\n");

	/* are there enough numbers for the texture? */
	if (node->image.n < 3) {
		printf ("PixelTexture, need at least 3 elements, have %d\n",node->image.n);
		ok = FALSE;
	} else {
		//http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/fieldsDef.html#SFImageAndMFImage
		//SFImage fields contain three integers representing the width, height and number of components in the image
		//Pixels are specified from left to right, bottom to top (ie like a texture, not an image)
		wid = *iptr; iptr++;
		hei = *iptr; iptr++;
		depth = *iptr; iptr++;

		DEBUG_TEX ("wid %d hei %d depth %d\n",wid,hei,depth);

		if ((depth < 0) || (depth >4)) {
			printf ("PixelTexture, depth %d out of range, assuming 1\n",(int) depth);
			depth = 1;
		}
	
		if ((wid*hei-3) > node->image.n) {
			printf ("PixelTexture, not enough data for wid %d hei %d, have %d\n",
					wid, hei, (wid*hei)-2);
			ok = FALSE;
		}
	}

	/* did we have any errors? if so, create a grey pixeltexture and get out of here */
	if (!ok) {
		return;
	}

	/* ok, we are good to go here */
	this_tex->x = wid;
	this_tex->y = hei;
	this_tex->hasAlpha = ((depth == 2) || (depth == 4));
	this_tex->channels = depth;

	texture = MALLOC (unsigned char *, wid*hei*4);
	this_tex->texdata = texture; /* this will be freed when texture opengl-ized */
	this_tex->status = TEX_NEEDSBINDING;

	tctr = 0;
	if(texture != NULL){
		for (count = 0; count < (wid*hei); count++) {
			switch (depth) {
				case 1: {
					   texture[tctr++] = *iptr & 0xff;
					   texture[tctr++] = *iptr & 0xff;
					   texture[tctr++] = *iptr & 0xff;
					   texture[tctr++] = 0xff; /*alpha, but force it to be ff */
					   break;
				   }
				case 2: {
					   texture[tctr++] = (*iptr>>8) & 0xff;	 /*G*/
					   texture[tctr++] = (*iptr>>8) & 0xff;	 /*G*/
					   texture[tctr++] = (*iptr>>8) & 0xff;	 /*G*/
					   texture[tctr++] = (*iptr>>0) & 0xff; /*A*/
					   break;
				   }
				case 3: {
					   texture[tctr++] = (*iptr>>16) & 0xff; /*R*/
					   texture[tctr++] = (*iptr>>8) & 0xff;	 /*G*/
					   texture[tctr++] = (*iptr>>0) & 0xff; /*B*/ 
					   texture[tctr++] = 0xff; /*alpha, but force it to be ff */
					   break;
				   }
				case 4: {
					   texture[tctr++] = (*iptr>>24) & 0xff; /*R*/
					   texture[tctr++] = (*iptr>>16) & 0xff; /*G*/
					   texture[tctr++] = (*iptr>>8) & 0xff;	 /*B*/
					   texture[tctr++] = (*iptr>>0) & 0xff; /*A*/
					   break;
				   }
			}
			iptr++;
		}
	}
}


static void texture_load_from_pixelTexture3D (textureTableIndexStruct_s* this_tex, struct X3D_PixelTexture3D *node)
{

// load a PixelTexture that is stored in the PixelTexture as an MFInt32 
	int hei,wid,bpp,dep,nvox,nints;
	unsigned char *texture;
	int count;
	int ok;
	int *iptr;
	int tctr;

	iptr = node->image.p;

	ok = TRUE;

	DEBUG_TEX ("start of texture_load_from_pixelTexture...\n");

	// are there enough numbers for the texture? 
	if (node->image.n < 4) {
		printf ("PixelTexture, need at least 3 elements, have %d\n",node->image.n);
		ok = FALSE;
	} else {
		//http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/texture3D.html#PixelTexture3D
		//MFInt32 image field contain 4 integers representing channels(aka components), width, height,depth the image
		//it doesn't say the row-order (y-up like texture or y-down like image)
		//closest analogy: SFImage Field uses y-up (texture) convention
		//We will use y-up texture convention here. That means no row flipping as you read,
		//first row is bottom of texture
		bpp = *iptr; iptr++;
		wid = *iptr; iptr++;
		hei = *iptr; iptr++;
		dep = *iptr; iptr++;

		DEBUG_TEX ("bpp %d wid %d hei %d dep %d \n",bpp,wid,hei,dep);

		if ((bpp < 0) || (bpp >4)) {
			printf ("PixelTexture, bytes per pixel %d out of range, assuming 1\n",(int) bpp);
			bpp = 1;
		}
		nvox = wid*hei*dep;
		nints = (nvox * bpp) / 4; //4 bytes per int, how many ints
		if ((nints + 4) > node->image.n) {
			printf ("PixelTexture3D, not enough data for bpp %d wid %d hei %d, dep %d, need %d have %d\n",
					bpp, wid, hei, dep, nints + 4, node->image.n);
			ok = FALSE;
		}
	}

	// did we have any errors? if so, create a grey pixeltexture and get out of here 
	if (!ok) {
		return;
	}

	// ok, we are good to go here 
	this_tex->x = wid;
	this_tex->y = hei;
	this_tex->z = dep;
	this_tex->hasAlpha = ((bpp == 2) || (bpp == 4));
	this_tex->channels = bpp;

	texture = MALLOC (unsigned char *, wid*hei*4*dep);
	this_tex->texdata = texture; // this will be freed when texture opengl-ized 
	this_tex->status = TEX_NEEDSBINDING;

	tctr = 0;
	if(texture != NULL){
		for (count = 0; count < (wid*hei*dep); count++) {
			switch (bpp) {
				case 1: {
					   texture[tctr++] = *iptr & 0xff;
					   texture[tctr++] = *iptr & 0xff;
					   texture[tctr++] = *iptr & 0xff;
					   texture[tctr++] = 0xff; //alpha, but force it to be ff 
					   break;
				   }
				case 2: {
					   texture[tctr++] = (*iptr>>8) & 0xff;	 //G
					   texture[tctr++] = (*iptr>>8) & 0xff;	 //G
					   texture[tctr++] = (*iptr>>8) & 0xff;	 //G
					   texture[tctr++] = (*iptr>>0) & 0xff; //A
					   break;
				   }
				case 3: {
					   texture[tctr++] = (*iptr>>16) & 0xff; //R
					   texture[tctr++] = (*iptr>>8) & 0xff;	 //G
					   texture[tctr++] = (*iptr>>0) & 0xff; //B 
					   texture[tctr++] = 0xff; //alpha, but force it to be ff 
					   break;
				   }
				case 4: {
					   texture[tctr++] = (*iptr>>24) & 0xff; //R
					   texture[tctr++] = (*iptr>>16) & 0xff; //G
					   texture[tctr++] = (*iptr>>8) & 0xff;	 //B
					   texture[tctr++] = (*iptr>>0) & 0xff; //A
					   break;
				   }
			}
			iptr++;
		}
	}
}

int loadImage3D_x3di3d(struct textureTableIndexStruct *tti, char *fname){
/*	SUPERCEEDED by web3dit
	reads 3D image in ascii format like you would put inline for PixelTexture3D
	except with sniffable header x3dimage3d ie:
	"""
	x3di3d
	3 4 6 2 0xFF00FF .... 
	"""
	3 channgels, nx=4, ny=6, nz=2 and one int string per pixel
	format 'invented' by dug9 for testing
*/
	int i,j,k,nx,ny,nz,ishex, iret, totalbytes, ipix, nchan;
	unsigned int pixint;
	FILE *fp;

	iret = FALSE;

	fp = fopen(fname,"r");
	if (fp != NULL) {
		char line [1000];
		fgets(line,1000,fp);
		if(strncmp(line,"x3di3d",6)){
			//not our type
			fclose(fp);
			return iret;
		}
		ishex = 0;
		if(!strncmp(line,"x3di3d x",8)) ishex = 1;
		fscanf(fp,"%d %d %d %d",&nchan, &nx,&ny,&nz);
		totalbytes = 4 * nx * ny * nz;
		if(totalbytes <= 128 * 128 * 128 * 4){
			unsigned char *rgbablob;
			rgbablob = malloc(nx * ny * nz * 4);
			memset(rgbablob,0,nx*ny*nz*4);

			//now convert to RGBA 4 bytes per pixel
			for(i=0;i<nz;i++){
				for(j=0;j<ny;j++){
					for(k=0;k<nx;k++){
						unsigned char pixel[4],*rgba;
						if(ishex)
							fscanf(fp,"%x",&pixint);
						else
							fscanf(fp,"%d",&pixint);
						//assume incoming red is high order, alpha is low order byte
						pixel[0] = (pixint >> 0) & 0xff; //low byte/little endian ie alpha, or B for RGB
						pixel[1] = (pixint >> 8) & 0xff;
						pixel[2] = (pixint >> 16) & 0xff;
						pixel[3] = (pixint >> 24) & 0xff;
						//printf("[%x %x %x %x]",(int)pixel[0],(int)pixel[1],(int)pixel[2],(int)pixel[3]);
						ipix = (i*nz +j)*ny +k;
						rgba = &rgbablob[ipix*4];
						//http://www.color-hex.com/ #aabbcc
						switch(nchan){
							case 1: rgba[0] = rgba[1] = rgba[2] = pixel[0]; rgba[3] = 255;break;
							case 2: rgba[0] = rgba[1] = rgba[2] = pixel[1]; rgba[3] = pixel[0];break;
							case 3: rgba[0] = pixel[2]; rgba[1] = pixel[1]; rgba[2] = pixel[2]; rgba[3] = 255;  // BGRA
							break;
							case 4: rgba[0] = pixel[3]; rgba[1] = pixel[2]; rgba[2] = pixel[1]; rgba[3] = pixel[0]; break; // BGRA
							default:
								break;
						}
						//memcpy(rgba,&pixint,4);
					}
				}
			}
			tti->channels = nchan;
			tti->x = nx;
			tti->y = ny;
			tti->z = nz;
			tti->texdata = rgbablob;
			iret = TRUE;
		}
		fclose(fp);
	}
	return iret;

}
void saveImage3D_x3di3d(struct textureTableIndexStruct *tti, char *fname){
/*	SUPERCEEDED by web3dit
	reads 3D image in ascii format like you would put inline for PixelTexture3D
	except with sniffable header x3dimage3d ie:
	"""
	x3di3d
	3 4 6 2 0xFF00FF .... 
	"""
	3 channgels, nx=4, ny=6, nz=2 and one int string per pixel
	format 'invented' by dug9 for testing
*/
	int i,j,k,nx,ny,nz, iret, ipix, nchan;
	unsigned int pixint;
	char *rgbablob;
	FILE *fp;

	iret = FALSE;

	fp = fopen(fname,"w+");
	nchan = tti->channels;
	nx = tti->x;
	ny = tti->y;
	nz = tti->z;
	rgbablob = tti->texdata;

	fprintf(fp,"x3di3d x\n"); //x for hex, i for int rgba, la order ie red is high
	fprintf(fp,"%d %d %d %d",nchan, nx,ny,nz);

	for(i=0;i<nz;i++){
		for(j=0;j<ny;j++){
			for(k=0;k<nx;k++){
				unsigned char *rgba;
				ipix = (i*nz +j)*ny +k;
				rgba = &rgbablob[ipix*4];
				pixint = 0;
				switch(nchan){
					case 1:	pixint = rgba[0];break;
					case 2:	pixint = (rgba[0] << 8) + rgba[3];break;
					case 3:	pixint = (rgba[0] << 16) + (rgba[1] << 8) + (rgba[2] << 0);break; 
					case 4:	pixint = (rgba[0] << 24) + (rgba[1] << 16) + (rgba[2] << 8) + rgba[3];break; // BGRA
					default:
						pixint = 0;
				}
				switch(nchan){
					case 1:	fprintf(fp," %#.2x",pixint);break;
					case 2:	fprintf(fp," %#.4x",pixint);break;
					case 3:	fprintf(fp," %#.6x",pixint);break;
					case 4:	fprintf(fp," %#.8x",pixint);break;
					default:
						fprintf(fp," 0x00");break;
				}
			}
		}
	}
	fclose(fp);

}

int loadImage_web3dit(struct textureTableIndexStruct *tti, char *fname){
/*	TESTED ONLY RGB Geometry 3 and C AS OF SEPT 9, 2016
	reads image in ascii format almost like you would put inline for PixelTexture
	Goal: easy to create image file format just sufficient for web3d types:
		2 2D texture
		3 3D texture
		C cubemap
		volume (float luminance)
		panorama
	with sniffable header web3dit:
	"""
web3ditG #H 7 byte magic header, means web3d compatible image in text form, 1byte for Geometry sniffing
1       #F file version
C       #G {C,P,3,2}: image geometry: C: cubemap RHS y-up z/depth/layer/order [+-x,+-y,+-z], top of top +z, bottom of bottom -z P: 360 panorama [L->R, 360/z ], 3: texture3D or Volume [z=depth], 2: texture2D
        #O optional description
x       #T {x,i,f} how to read space-delimited value: x as hex, i as int, f as float
0 255   #R range of channel, most useful for normalizing floats
4       #N channels/components per value ie RGBA as int: 4, RGBA as 4 ints: 1
1       #M values per pixel ie RGBA as int: 1, RGBA as 4 ints: 4
RGBA    #C[N*M] component names and order, choose from: {R,G,B,A,L} ie RGBA, LA, L, RGB
3       #D number of dimensions, 2 for normal 2D image, 3 for 3D image
3 3 3   #P[D] size in pixels in each dimension: x,y,z (use 1 for z if 2D)
D       #Y {U,D} image y-Down or texture y-Up row order
#I image values follow with x in inner loop, Y image direction, z in outer:
0xFF00FF .... 
	"""
	format 'invented' by dug9 for testing freewrl, License: MIT
*/
	int i,j,k,m,nx,ny,nz,nv,nc, iret, totalbytes, ipix, jpix, kpix, nchan;
	int version, Rmin, Rmax, Nchannelspervalue, Mvaluesperpixel, Dimensions;
	unsigned int pixint, Pixels[10], iydown;
	float pixfloat;
	char Geometry, ODescription[200], Type, Componentnames[10], YDirection;
	FILE *fp;

	iret = FALSE;

	fp = fopen(fname,"r");
	if (fp != NULL) {
		char line [1000];
		fgets(line,1000,fp);
		if(strncmp(line,"web3dit",7)){
			//not our type
			fclose(fp);
			return iret;
		}
		//could sniff Geometry here, if caller says what geometry type is OK for them, return if not OK
		fgets(line,1000,fp);
		sscanf(line,"%c",&Geometry);
		fgets(line,1000,fp);
		sscanf(line,"%d",&version);
		fgets(line,1000,fp);
		sscanf(line,"%s",ODescription);
		fgets(line,1000,fp);
		sscanf(line,"%c",&Type);
		fgets(line,1000,fp);
		sscanf(line,"%d %d",&Rmin,&Rmax);

		fgets(line,1000,fp);
		sscanf(line,"%d",&Nchannelspervalue);
		fgets(line,1000,fp);
		sscanf(line,"%d",&Mvaluesperpixel);
		fgets(line,1000,fp);
		sscanf(line,"%s",Componentnames);
		fgets(line,1000,fp);
		sscanf(line,"%d",&Dimensions);
		fgets(line,1000,fp);
		sscanf(line,"%d %d %d",&Pixels[0], &Pixels[1], &Pixels[2]);
		fgets(line,1000,fp);
		sscanf(line,"%c",&YDirection);
		fgets(line,1000,fp); //waste #I Image warning line


		nx = ny = nz = 1;
		nx = Pixels[0];
		ny = Pixels[1];
		if(Dimensions > 2) nz = Pixels[2];
		nv = Mvaluesperpixel;
		nc = Nchannelspervalue;
		nchan = nv * nc;
		iydown = 1;
		if(YDirection == 'U') iydown = 0;
			
		totalbytes = 4 * nx * ny * nz; //output 4 channel RGBA image size
		if(totalbytes <= 256 * 256 * 256 * 4){
			unsigned char *rgbablob;
			rgbablob = malloc(totalbytes);
			memset(rgbablob,0,totalbytes);

			//now convert to RGBA 4 bytes per pixel
			for(i=0;i<nz;i++){
				for(j=0;j<ny;j++){
					for(k=0;k<nx;k++){
						unsigned char pixel[4],*rgba, n;
						pixel[0] = pixel[1] = pixel[2] = pixel[3] = 0;
						for(m=0;m<nv;m++){
							switch(Type){
								case 'f':
								fscanf(fp,"%f",&pixfloat);
								break;
								case 'x':
								fscanf(fp,"%x",&pixint);
								break;
								case 'i':
								default:
								fscanf(fp,"%d",&pixint);
								break;
							}
							for(n=0;n<nc;n++){
								switch(Type){
									case 'f':
										pixel[n] = (unsigned char)(unsigned int)((pixfloat - Rmin) / (Rmax - Rmin) * 255.0);
									break;
									case 'x':
									case 'i':
									default:
										pixel[n] = (pixint >> n*8) & 0xff;
										break;
								}
							}

						}
						//for RGBA, pixel[0] is A, pixel[3] is B
						//printf("[%x %x %x %x]\n",(int)pixel[0],(int)pixel[1],(int)pixel[2],(int)pixel[3]);
						
						ipix = (i*ny +j)*nx +k;          //if file is like outgoing y-up texture order: first row is bottom of texture
						jpix = (i*ny +(ny-1-j))*nx + k;  //if file is in y-down image order: first row is top of image
						kpix = iydown ? jpix : ipix;
						rgba = &rgbablob[kpix*4];
						//http://www.color-hex.com/ #aabbcc
						switch(nchan){
							case 1: rgba[0] = rgba[1] = rgba[2] = pixel[0]; rgba[3] = 255;break;
							case 2: rgba[0] = rgba[1] = rgba[2] = pixel[1]; rgba[3] = pixel[0];break;
							case 3: rgba[0] = pixel[2]; rgba[1] = pixel[1]; rgba[2] = pixel[0]; rgba[3] = 255;  // BGRA
							break;
							case 4: rgba[0] = pixel[3]; rgba[1] = pixel[2]; rgba[2] = pixel[1]; rgba[3] = pixel[0]; break; // BGRA
							default:
								break;
						}
						//memcpy(rgba,&pixint,4);

					}
				}
			}
			tti->channels = nchan;
			tti->x = nx;
			tti->y = ny;
			tti->z = nz;
			tti->texdata = rgbablob;
			if(0){
				printf("\n");
				for(i=0;i<tti->z;i++){
					for(j=0;j<tti->y;j++){
						for(k=0;k<tti->x;k++){
							int ipix,jpix,kpix;
							unsigned int pixint;
							ipix = (i*tti->y + j)*tti->x + k;
							jpix = (i*tti->y + (tti->y -1 - j))*tti->x + k;
							kpix = ipix; //print it like we see it
							memcpy(&pixint,&tti->texdata[kpix*4],4);
							printf("%x ",pixint);
						}
						printf("\n");
					}

				}
			}
			iret = TRUE;
		}
		fclose(fp);
	}
	return iret;

}
void saveImage_web3dit(struct textureTableIndexStruct *tti, char *fname){
/*	TESTED ONLY RGB Geometry 3 and C AS OF SEPT 9, 2016
	writes image in ascii format almost like you would put inline for PixelTexture
	please put .web3dit suffix on fname (i stands for image, t stands for text format (future x xml, j json formats?)
	Goal: easy to create image file format just sufficient for web3d types:
		2D texture
		3D texture
		cubemap
		volume (float luminance)
	and write out the explanation of the header with the header so don't need this document
	with sniffable header web3dit:
	"""
web3ditG #H 7 byte magic header, means web3d compatible image in text form, 1byte for Geometry sniffing
C       #G {C,P,3,2} image geometry: C: cubemap RHS y-up z/depth/layer/order [+-x,+-y,+-z], top of top +z, bottom of bottom -z P: 360 panorama [L->R, 360/z ], 3: texture3D or Volume [z=depth], 2: texture2D
1       #F file version
        #O optional description
x       #T {x,i,f} how to read space-delimited value: x as hex, i as int, f as float
0 255   #R range of channel, most useful for normalizing floats
4       #N channels/components per value ie RGBA as int: 4, RGBA as 4 ints: 1
1       #M values per pixel ie RGBA as int: 1, RGBA as 4 ints: 4
RGBA    #C[N*M] component names and order, choose from: {R,G,B,A,L} ie RGBA, LA, L, RGB
3       #D number of dimensions, 2 for normal 2D image, 3 for 3D image
3 3 3   #P[D] size in pixels in each dimension: x,y,z(depth/layer) (use 1 for z if 2D)
D       #Y {U,D} image y-Down or texture y-Up row order
#I image values follow with x in inner loop, Y image direction, z in outer:
0xFF00FF .... 
	"""
	format 'invented' by dug9 for testing freewrl, License: MIT
*/
	int i,j,k,nx,ny,nz, iret, ipix, jpix, kpix, iydown, nchan;
	int version, Rmin, Rmax, Nchannelspervalue, Mvaluesperpixel, Dimensions;
	unsigned int pixint;
	char Geometry, *ODescription, Type, *Componentnames, Pixels[10], YDirection;
	static char *LRGBA [] = {"L","LA","RGB","RGBA"};
	FILE *fp;


	iret = FALSE;

	fp = fopen(fname,"w+");
	if (fp != NULL) {
		unsigned char *rgbablob;
		nchan = tti->channels;
		nx = tti->x;
		ny = tti->y;
		nz = tti->z;
		rgbablob = tti->texdata;
		Dimensions = nz == 1 ? 2 : 3;
		Pixels[0] = nx;
		Pixels[1] = ny;
		Pixels[2] = nz;
		Nchannelspervalue = nchan;
		Mvaluesperpixel = 1;
		Rmin = 0;
		Rmax = 255;
		Type = 'x';
		ODescription = "";
		Geometry = '2';
		if(nz > 1) Geometry = '3';
		Componentnames = LRGBA[nchan -1]; //"RGBA";
		version = 1;
		YDirection = 'D';
		iydown = (YDirection == 'D') ? 1 : 0;

		fprintf(fp,"web3dit%c #H 7 byte magic header, means web3d compatible image in text form, 1byte for Geometry sniffing\n",Geometry);
		fprintf(fp,"%c       #G {C,P,3,2}: image geometry: C: cubemap RHS y-up z/depth/layer/order [+-x,+-y,+-z], top of top +z, bottom of bottom -z P: 360 panorama [L->R, 360/z ], 3: texture3D or Volume [z=depth], 2: texture2D\n",Geometry);
		fprintf(fp,"%d       #F {1} file version\n",version);
		fprintf(fp,"%s       #O optional description\n",ODescription);
		fprintf(fp,"%c       #T {x,i,f} how to read space-delimited value: x as hex, i as int, f as float\n",Type);
		fprintf(fp,"%d %d   #R range of channel, most useful for normalizing floats\n",Rmin,Rmax);
		fprintf(fp,"%d       #N channels/components per value ie RGBA as int: 4, RGBA as 4 ints: 1\n",Nchannelspervalue);
		fprintf(fp,"%d       #M values per pixel ie RGBA as int: 1, RGBA as 4 ints: 4\n",Mvaluesperpixel);
		fprintf(fp,"%s       #C[N*M] component names and order, choose from: {R,G,B,A,L} ie RGBA, LA, L, RGB\n",Componentnames);
		fprintf(fp,"%d       #D number of dimensions, 2 for normal 2D image, 3 for 3D image\n",Dimensions);
		fprintf(fp,"%d %d %d  #P[D] size in pixels in each dimension: x,y,z (use 1 for z if 2D)\n",nx,ny,nz);
		fprintf(fp,"%c       #Y {U,D} image y-Down or texture y-Up row order\n",YDirection);
		fprintf(fp,"#I image values follow with x in inner loop, y-down image direction, z in outer:\n");

		//now convert to RGBA 4 bytes per pixel
		for(i=0;i<nz;i++){
			for(j=0;j<ny;j++){
				for(k=0;k<nx;k++){
					unsigned char *rgba;
					ipix = (i*ny +j)*nx +k; //incoming assumed in y-up texture order
					jpix = (i*ny +(ny-1-j))*nx + k; //outgoing in y-down image order
					kpix = iydown ? jpix : ipix;
					rgba = &rgbablob[kpix*4];
					pixint = 0;
					switch(nchan){
						case 1:	pixint = rgba[0];break;
						case 2:	pixint = (rgba[0] << 8) + rgba[3];break;
						case 3:	pixint = (rgba[0] << 16) + (rgba[1] << 8) + (rgba[2] << 0);break; 
						case 4:	pixint = (rgba[0] << 24) + (rgba[1] << 16) + (rgba[2] << 8) + rgba[3];break; // RGBA
						default:
							pixint = 0;
					}
					switch(nchan){
						case 1:	fprintf(fp," %#.2x",pixint);break;
						case 2:	fprintf(fp," %#.4x",pixint);break;
						case 3:	fprintf(fp," %#.6x",pixint);break;
						case 4:	fprintf(fp," %#.8x",pixint);break;
						default:
							fprintf(fp," 0x00");break;
					}
				}
			}
		}
		fclose(fp);
	}
}

int loadImage3DVol(struct textureTableIndexStruct *tti, char *fname){
/* UNTESTED, UNUSED AS OF SEPT 6, 2016
	a simple 3D volume texture format 
	- primarily for int gray/luminance, useful for VolumeRendering
	- does have a 4 channel 
	x but only 2 bytes for 4 channels - 4 bits per channel
	x doesn't have an official sniff header
	- more appropriate for communicating between your own 2 programs, 
		where you know the meaning of the numbers
	x not generally appropriate for international standards support like web3d

http://paulbourke.net/dataformats/volumetric/
The data type is indicated as follows
1 - single bit per cell, two categories
2 - two byes per cell, 4 discrete levels or categories
4 - nibble per cell, 16 discrete levels
8 - one byte per cell (unsigned), 256 levels
16 - two bytes representing a signed "short" integer
32 - four bytes representing a signed integer
The endian is one of
0 for big endian (most significant byte first). For example Motorola processors, Sun, SGI, some HP.
1 for little endian (least significant byte first). For example Intel processors, Dec Alphas.
*/
	int i,j,k,nx,ny,nz, bitsperpixel, bpp, iendian, iret, totalbytes, ipix, jpix, nchan;
	float sx,sy,sz,tx,ty,tz;
	FILE *fp;

	iret = FALSE;

	fp = fopen(fname,"r+b");
	if (fp != NULL) {
		char line [1000];
		fgets(line,1000,fp);
		if(strncmp(line,"vol",3)){
			//for now we'll enforce 'vol' as first the chars of file for sniffing, but not enforcable
			fclose(fp);
			return iret;
		}
		fgets(line,1000,fp);
		sscanf(line,"%d %d %d",&nx,&ny,&nz);
		fgets(line,1000,fp);
		sscanf(line,"%f %f %f",&sx,&sy,&sz);
		fgets(line,1000,fp);
		sscanf(line,"%f %f %f",&tx,&ty,&tz);
		fgets(line,1000,fp);
		sscanf(line,"%d %d",&bitsperpixel,&iendian);
		bpp = bitsperpixel / 8;
		nchan = 1;
		switch(bitsperpixel){
			case 1: nchan = 1; break;
			//1 - single bit per cell, two categories
			case 2: nchan = 4; break;
			//2 - two byes per cell, 4 discrete levels or categories
			case 4: nchan = 1; break;
			//4 - nibble per cell, 16 discrete levels
			case 8: nchan = 1; break;
			//8 - one byte per cell (unsigned), 256 levels
			case 16: nchan = 1; break;
			//16 - two bytes representing a signed "short" integer
			case 32: nchan = 1; break;
			//32 - four bytes representing a signed integer
			default:
				break;
		}

		totalbytes = bpp * nx * ny * nz;
		if(totalbytes < 128 * 128 * 128 *4){
			unsigned char* blob, *rgbablob;
			blob = malloc(totalbytes + 4);
			rgbablob = malloc(nx * ny * nz * 4);
			memset(rgbablob,0,nx*ny*nz*4);

			fread(blob,totalbytes,1,fp);
			//now convert to RGBA 4 bytes per pixel
			for(i=0;i<nz;i++){
				for(j=0;j<ny;j++){
					for(k=0;k<nx;k++){
						unsigned char *pixel,*rgba;
						ipix = (i*ny +j)*nx +k; //incoming image 
						jpix = (i*ny +(ny-1-j))*nx +  k;
						pixel = &blob[ipix*bpp];
						rgba = &rgbablob[ipix*4];
						rgba[3] = 255;
						switch(bitsperpixel){
							case 1: break;
							//1 - single bit per cell, two categories
							//rgba[0] = rgba[1] = rgba[2] = 
							case 2:
							//2 - two byes per cell, 4 discrete levels or categories
							rgba[0] = pixel[0] >> 4;
							rgba[1] = pixel[0] & 0xF;
							rgba[2] = pixel[1] >> 4;
							rgba[3] = pixel[1] & 0xF;
							break;
							case 4:
							//4 - nibble per cell, 16 discrete levels
							break;
							case 8:
							//8 - one byte per cell (unsigned), 256 levels
							rgba[0] = rgba[1] = rgba[2] = (unsigned char)pixel[0];
							break;
							case 16:
							//16 - two bytes representing a signed "short" integer
							rgba[0] = rgba[1] = rgba[2] = (unsigned char) *(unsigned short*)pixel;
							break;
							case 32:
							//32 - four bytes representing a signed integer
							rgba[0] = pixel[0]; //too much range, we will split high part into rgb
							rgba[1] = pixel[1];
							rgba[2] = pixel[2];
							rgba[3] = pixel[3]; //and we'll even take a byte as alpha, for fun
							break;
							default:
								break;
						}
					}
				}
			}
			if(blob) free(blob);
			tti->channels = nchan;
			tti->x = nx;
			tti->y = ny;
			tti->z = nz;
			tti->texdata = rgbablob;
			iret = TRUE;
		}
		fclose(fp);
	}
	return iret;

}

// NRRD MIT ===========================>>>>>>>>>>>>>>
// a mini-nrrd reader, with MIT licence
// Oct 2, 2016: only luminance / scalar-value-per-voxel implemented, only unsigned char type tested
//#include <endian.h> //windows doesn't have
//#define IS_LITTLE_ENDIAN (1 == *(unsigned char *)&(const int)1)   //found on internet
int isMachineLittleEndian(){
	//dug9: this will/should detect at runtime between big and little endian host machines
	// which reverse byte order- but not mixed endian ie pdp endian
	unsigned short int one = 1;
	unsigned char *c;
	int iret; //, itest;
	c = (unsigned char *)&one;
	iret = (c[0] == 1) ? TRUE : FALSE;
	//itest = IS_LITTLE_ENDIAN ? TRUE : FALSE;
	//if(iret != itest) printf("endian confusion\n");
	return iret;
}
enum {
NRRDFIELD_type = 1,
NRRDFIELD_dimension,
NRRDFIELD_sizes,
NRRDFIELD_spacing,
NRRDFIELD_encoding,
NRRDFIELD_endian,
};
struct {
const char *fieldname;
int len;
const int fieldtype;
} nrrdfields [] = {
{"type:",5,NRRDFIELD_type},
{"dimension:",10,NRRDFIELD_dimension},
{"sizes:",6,NRRDFIELD_sizes},
{"spacings:",9,NRRDFIELD_spacing},
{"encoding:",9,NRRDFIELD_encoding},
{"endian:",7,NRRDFIELD_endian},
{NULL,0,0},
};
enum {
CDATATYPE_char = 1,
CDATATYPE_uchar,
CDATATYPE_short,
CDATATYPE_ushort,
CDATATYPE_int,
CDATATYPE_uint,
CDATATYPE_longlong,
CDATATYPE_ulonglong,
CDATATYPE_float,
CDATATYPE_double,
};
struct {
const char * stypes[7];
const int itype;
const int bsize;
const char * fmt;
} nrrddatatypes [] = {
{{"signed char", "int8_t", "int8",  NULL,NULL,NULL,NULL},	CDATATYPE_char, 1, "%hh"},
{{"uchar", "unsigned char", "uint8", "uint8_t",	NULL,NULL,NULL}, CDATATYPE_uchar, 1, "%hhu" },
{{"short", "short int", "signed short", "signed short int", "int16", "int16_t", NULL}, CDATATYPE_short, 2, "%hd" },
{{"ushort", "unsigned short", "unsigned short int", "uint16", "uint16_t", NULL, NULL}, CDATATYPE_ushort, 2, "%hu" },
{{"int", "signed int", "int32", "int32_t", NULL, NULL, NULL},	CDATATYPE_int, 4, "%d"},
{{"uint", "unsigned int", "uint32", "uint32_t", NULL, NULL, NULL}, CDATATYPE_uint, 4, "%u" },
{{"longlong", "long long", "long long int", "signed long long", "signed long long int", "int64", "int64_t"}, CDATATYPE_longlong, 8, "%lld"},
{{"ulonglong", "unsigned long long", "unsigned long long int", "uint64", "uint64_t", NULL, NULL}, CDATATYPE_ulonglong, 8, "%llu" },
{{"float", NULL,NULL,NULL,NULL, NULL,NULL},CDATATYPE_float,4, "%f" },
{{"double", NULL,NULL,NULL,NULL, NULL,NULL},CDATATYPE_double,8, "%lf"},
{{NULL,NULL,NULL,NULL, NULL,NULL,NULL},0},
};
enum {
NRRDENDIAN_LITTLE = 1,
NRRDENDIAN_BIG,
};
enum {
NRRDENCODING_RAW = 1,
NRRDENCODING_ASCII,
};
int loadImage_nrrd(struct textureTableIndexStruct *tti, char *fname){
/*
	license on this function: MIT or equivalent
	volume / 3D images, for VolumeRendering and Texturing3D components
	http://teem.sourceforge.net/nrrd/format.html
	subset implemented here: assumes luminance-only (scalar per voxel)
	(see the kinds[] field for more general interpretation of the rows)
	we will skip non-basic / optional fields and key/values since we aren't also writing back out in a full cycle
	The headers are ascii, and you can type the file to the console to see the header
C:>type brain.nrrd
NRRD0001
content: "MRI Brain for 3DVisualizer"
type: unsigned char
dimension: 3
sizes: 512 512 230
spacings: 1 1 0.4
encoding: raw

C:>type supine.nrrd
NRRD0001
content: "Torso Supine"
type: unsigned short
dimension: 3
sizes: 512 512 426
spacings: 1 1 1
endian: little
encoding: raw

a few sample images have degenerate first dimension
NRRD0001
type: unsigned char
dimension: 4
sizes: 1 256 256 124
spacings:  NaN 0.01 0.01 0.01
encoding: raw


*/
	int iret;
	FILE *fp;
	iret = FALSE;

	fp = fopen(fname,"r+b"); //need +b for binary mode, to read over nulls
	if (fp != NULL) {
		unsigned long long i,j,k;
		int ifieldtype, idatatype, kdatatype, idim, ilen, isize[4], iendian, iencoding, ifound,slen,klen, bsize;
		char line [2048];
		char cendian[256], cencoding[256];
		char *remainder;
		const char *fmt;
		unsigned long long nvoxel;
		unsigned long long totalbytes;
		unsigned char *data;
		unsigned char *voxel;
		double dhi, dlo; 
		double d255range;
		int counts[256]; //histogram

		fgets(line,2047,fp);
		if(strncmp(line,"NRRD",4)){
			//not our type
			fclose(fp);
			return iret;
		}

		iendian = 0;// NRRDENDIAN_LITTLE;
		idim = 0; //3;
		idatatype = 0; // CDATATYPE_int;
		isize[0] = isize[1] = isize[2] = isize[3] = 0;
		iencoding = 0; //NRRDENCODING_RAW;
		bsize = 1; //binary size of voxel, in bytes, for mallocing
		kdatatype = 0; //index into nrrddatatypes array
		//read header field, one per loop:
		for(;;){
			fgets(line,2047,fp);
			i = 0;
			ifieldtype = 0; //unknown
			ilen = 0; //length of field string
			if(strlen(line) < 3){
				// '...the data (following the blank line after the header) ..'
				break; //nrrd signals end of header with blank line ie \n' or \r\n
			}

			//see if we have a matching field from the sub-list that we care about 
			for(;;){
				if(!nrrdfields[i].fieldname)break;
				if(!strncmp(line,nrrdfields[i].fieldname,nrrdfields[i].len)){
					ifieldtype = nrrdfields[i].fieldtype;
					ilen = nrrdfields[i].len;
					break;
				}
				i++;
			}
			remainder = &line[ilen];
			switch(ifieldtype){
				case NRRDFIELD_type:
					//find first non-blank byte where it starts ie 'unsigned short int'
					for(i=0;i<10;i++){
						if(remainder[0] == ' ') remainder = &remainder[1];
						else break;
					}
					slen = strlen(remainder);
					//find last non-blank, non CRLF 
					klen = slen;
					for(i=0;i<slen;i++){
						char c = remainder[slen-1 -i];
						if(c == '\n' || c == '\r' || c == ' ') klen--;
						else break;
					}
					//compare known types to the full remainder string ie "unsigned short int"
					k = 0;
					ifound = FALSE;
					for(;;){
						if(nrrddatatypes[k].itype == 0) break;
						for(j=0;j<7;j++){
							if(nrrddatatypes[k].stypes[j]){  //some are null
								if(!strncmp(remainder,nrrddatatypes[k].stypes[j],klen)){
									ifound = TRUE;
									idatatype = nrrddatatypes[k].itype;
									kdatatype = (int)k;
									bsize = nrrddatatypes[k].bsize;
									fmt = nrrddatatypes[k].fmt;
									break; //break out of 0,7 loop
								}
							}
						}
						if(ifound) break;
						k++;
					}
					break;
				case NRRDFIELD_dimension:
					sscanf(remainder,"%d",&idim);
					idim = min(4,idim); //we can't use more yet ie a time-varying 3D image or separate R,G,B or X,Y,Z per voxel - just scalar per voxel
					break;
				case NRRDFIELD_sizes:
					switch(idim){
						case 1:
							sscanf(remainder,"%d",&isize[0]);break;
						case 2:
							sscanf(remainder,"%d%d",&isize[0],&isize[1]);break;
						case 3:
							sscanf(remainder,"%d%d%d",&isize[0],&isize[1],&isize[2]);break;
						case 4:
							sscanf(remainder,"%d%d%d%d",&isize[0],&isize[1],&isize[2],&isize[3]);break;
						default:
							break;
					}
					break;
				case NRRDFIELD_encoding:
					sscanf(remainder,"%s",cencoding);
					if(!strcmp(cencoding,"raw"))
						iencoding = NRRDENCODING_RAW;
					else if(!strcmp(cencoding,"ascii"))
						iencoding = NRRDENCODING_ASCII;
					break;

					break;
				case NRRDFIELD_endian:
					sscanf(remainder,"%s",cendian);
					if(!strcmp(cendian,"little"))
						iendian = NRRDENDIAN_LITTLE;
					else if(!strcmp(cendian,"big"))
						iendian = NRRDENDIAN_BIG;
					break;
				//we may need kinds[] which say how to interpret the scalars, otherwise limited to scalar-per-voxel
				//range field? would be helpful when compressing voxel significant bits into displayable unsigned char range
				default:
					//skip fields and key/value stuff we dont need or care about for our display app
					break;	
			}
		}
		if(1){
			printf("iendian %d idatatype %d iencoding %d idim %d isizes %d %d %d %d bsize %d\n",
					iendian,idatatype,iencoding,idim,isize[0],isize[1],isize[2],isize[3], bsize);
			printf("machine endian isLittle=%d\n",isMachineLittleEndian());
			printf("hows that?\n");

		}
		//clean up dimensions
		if(isize[0] == 1){
			//remove degenerate dimension, found in some images
			for(i=0;i<idim-1;i++){
				isize[i] = isize[i+1];
				//spacing[i] = spacing[i+1];
			}
			idim--;
		}
		if(idim <3) isize[2] = 1;
		if(idim <2) isize[1] = 1;
		if(idim >3) {
			idim = 3; //as of oct 3, 2016 we just do scalar / iso-value 3D images, not color, not time-series, not xyz
		}
		
		//malloc data buffer
		nvoxel = isize[0] * isize[1] * isize[2];
		totalbytes = nvoxel * bsize;
		data = MALLOC(unsigned char *,(size_t)totalbytes);
		memset(data,4,(size_t)totalbytes);
		voxel = MALLOC(unsigned char *, bsize);
		//read data
		if(iencoding == NRRDENCODING_RAW){
			int dataLittleEndian;
			size_t nelem_read, element_size = 0L;
			element_size = bsize;
			nelem_read = fread(data,element_size, (size_t)nvoxel,fp);
			printf("num elems read = %llu elemsize %ld bytes requeted = %llu %llu\n",(unsigned long long)nelem_read,(long)bsize,bsize*nvoxel,totalbytes);
			//endian conversion
			dataLittleEndian = iendian == NRRDENDIAN_LITTLE ? TRUE : FALSE;
			if(isMachineLittleEndian() != dataLittleEndian && bsize > 1){
				//data endian doesnt match machine endian - swap unconditionally
				printf("swapping endian\n");
				for(i=0;i<nvoxel;i++){
					char * voxel = &data[i*bsize];
					for(j=0;j<bsize/2;j++){
						char c;
						k = bsize -1 - j;
						c = voxel[j];
						voxel[j] = voxel[k];
						voxel[k] = c;
					}
				}
			}
		}else if(iencoding == NRRDENCODING_ASCII){
			int kvox = 0;
			//read all slices
			for(i=0;i<isize[2];i++){
				//read a slice
				for(j=0;j<isize[1];j++){
					//read a row
					for(k=0;k<isize[0];k++){
						//read a voxel - unfamiliar theory/method, dont trust
						fscanf(fp,fmt,voxel);
						//put voxel in data
						memcpy(&data[kvox*bsize],voxel,bsize);
					}
				}
			}
		}
		//we have binary data in voxel datatype described in file
		//currently (Oct 2, 2016) this function assumes scalar-per-voxel aka luminance or alpha

		//find range of data so we can compress range into unsigned char range 0-255 from much bigger ints and floats
		//initialize range - use maxint, minint or just init to first pixel which we do here
		voxel = &data[0];
		switch(idatatype){
			case CDATATYPE_char: 
				dlo = -127.0;
				dhi = 127.0; //or is it 128?
			break;
			case CDATATYPE_uchar: 
				dlo = 0.0;
				dhi = 255.0;
			break;
			case CDATATYPE_short: 
				dlo = dhi = (double) *(short*)(voxel);
			break;
			case CDATATYPE_ushort: 
				dlo = dhi = (double) *(unsigned short*)(voxel);
				printf("initial range for ushort hi %lf lo %lf\n",dhi,dlo);
			break;
			case CDATATYPE_int: 
				dlo = dhi = (double) *(long*)(voxel);
			break;
			case CDATATYPE_uint: 
				dlo = dhi = (double) *(unsigned long*)(voxel);
			break;
			case CDATATYPE_longlong: 
				dlo = dhi = (double) *(long long *)(voxel);
			break;
			case CDATATYPE_ulonglong: 
				dlo = dhi = (double) *(unsigned long long *)(voxel);
			break;
			case CDATATYPE_float: 
				dlo = dhi = (double) *(float*)(voxel);
			break;
			case CDATATYPE_double: 
				dlo = dhi = *(double*)(voxel);
			break;
			default:
				break;
		}
		//find lower and upper of range by looking at every value
		for(i=0;i<nvoxel;i++){
			unsigned char *voxel;
			//unsigned char A;
			unsigned char *rgba = &tti->texdata[i*4];
			//LUM-ALPHA with RGB=1, A= voxel scalar
			voxel = &data[i*bsize];
			switch(idatatype){
				case CDATATYPE_char: 
					dlo = min(dlo,(double)*(char*)(voxel));
					dhi = max(dhi,(double)*(char*)(voxel));
				break;
				case CDATATYPE_uchar: 
					dlo = min(dlo,(double)*(unsigned char*)(voxel));
					dhi = max(dhi,(double)*(unsigned char*)(voxel));
				break;
				case CDATATYPE_short: 
					dlo = min(dlo,(double)*(short*)(voxel));
					dhi = max(dhi,(double)*(short*)(voxel));
				break;
				case CDATATYPE_ushort: 
					dlo = min(dlo,(double)*(unsigned short*)(voxel));
					dhi = max(dhi,(double)*(unsigned short*)(voxel));
				break;
				case CDATATYPE_int: 
					dlo = min(dlo,(double)*(long*)(voxel));
					dhi = max(dhi,(double)*(long*)(voxel));
				break;
				case CDATATYPE_uint: 
					dlo = min(dlo,(double)*(unsigned long*)(voxel));
					dhi = max(dhi,(double)*(unsigned long*)(voxel));
				break;
				case CDATATYPE_longlong: 
					dlo = min(dlo,(double)*(unsigned long long*)(voxel));
					dhi = max(dhi,(double)*(unsigned long long*)(voxel));
				break;
				case CDATATYPE_ulonglong: 
					dlo = min(dlo,(double)*(unsigned long*)(voxel));
					dhi = max(dhi,(double)*(unsigned long*)(voxel));
				break;
				case CDATATYPE_float: 
					dlo = min(dlo,(double)*(float*)(voxel));
					dhi = max(dhi,(double)*(float*)(voxel));
				break;
				case CDATATYPE_double: 
					dlo = min(dlo,(double)*(double*)(voxel));
					dhi = max(dhi,(double)*(double*)(voxel));
				break;
				default:
					break;
			}
		}
		d255range = 255.0/(dhi - dlo); 
		if(1) printf("nrrd image voxel range hi %lf lo %lf 255range scale factor %lf\n",dhi,dlo,d255range);
		//now convert to display usable data type which currently is RGBA
		tti->texdata = MALLOC(unsigned char *,(size_t)nvoxel * 4); //4 for RGBA
		tti->channels = 1; //1=lum 2=lum-alpha 3=rgb 4=rgba //doing 2-channel allows modulation of material color
			//Oct 16, 2016: in textures.c we now compute gradient automatically and put in RGB, if channels == 1 and z > 1
		tti->hasAlpha = TRUE;
		tti->x = isize[0];
		tti->y = isize[1];
		tti->z = isize[2];
		memset(counts,0,256*sizeof(int));
		for(i=0;i<nvoxel;i++){
			unsigned char *voxel;
			unsigned char A;
			unsigned char *rgba = &tti->texdata[i*4];
			//LUM-ALPHA with RGB=1, A= voxel scalar
			voxel = &data[i*bsize];
			if(1){
				//no range-scale method - might be needed for experiments
				switch(idatatype){
					case CDATATYPE_char: 
						A = (char)(voxel[0]) + 127; //convert from signed char to unsigned
					break;
					case CDATATYPE_uchar: 
						A = voxel[0];
					break;
					case CDATATYPE_short: 
						A = (unsigned char) ((*(short *)voxel) / 255) + 127; //scale into uchar range, assumes short range is fully used
					break;
					case CDATATYPE_ushort: 
						{
						static unsigned short lastushort = 1;
						unsigned short thisushort;
						memcpy(&thisushort,voxel,bsize);
						//thisushort = *(unsigned short*)voxel;
						//A = (unsigned char) ((*(unsigned short *)voxel) / 255); //scale into uchar range, "
						//A = (*(unsigned short *)voxel) >> 8;
						//A = ((*(unsigned short *)voxel) << 8) >> 8;
						//A = (unsigned char) abs(*(unsigned short *)voxel)/256;
						thisushort /= 256;
						//if(thisushort != lastushort)
						//	printf("%d ", (int)thisushort);
						counts[thisushort]++;
						lastushort = thisushort;
						A = (unsigned char) thisushort;
						}
					break;
					case CDATATYPE_int: 
						A = (unsigned char)((*((long *)voxel))/65536/255 + 127);
					break;
					case CDATATYPE_uint: 
						A = (unsigned char) ((*((unsigned long *)voxel))/65536/255);
					break;
					case CDATATYPE_longlong: 
						A = (unsigned char) ((*((long long *)voxel))/65536/65536/255 + 127);
					break;
					case CDATATYPE_ulonglong: 
						A = (unsigned char) ((*((unsigned long long *)voxel))/65536/65536/255);
					break;
					//case CDATATYPE_float: 
					//	A = (unsigned char) ((int)((*((float *)voxel))/range + range/2.0f) + 127) ;
					//break;
					//case CDATATYPE_double: 
					//	A = (unsigned char) ((int)((*((double *)voxel))/range + range/2.0f) + 127) ;
					//break;
					default:
						break;
				}
			} else {
				//range scaling method
				double dtemp; //, dtemp2;
				//unsigned int lutemp;
				//unsigned short utemp;
				//unsigned char uctemp;

				switch(idatatype){
					case CDATATYPE_char: 
						A = (unsigned char)((int)(voxel[0])) + 127; //convert from signed char to unsigned
					break;
					case CDATATYPE_uchar: 
						A = voxel[0];
					break;
					case CDATATYPE_short: 
						dtemp = (double)(*(short *)voxel);
						A = (unsigned char)(unsigned short)(unsigned int)((dtemp - dlo)*d255range);
					break;
					case CDATATYPE_ushort: 
						dtemp = (double)(*(unsigned short *)voxel);
						//dtemp2 = (dtemp - dlo)*d255range;
						//lutemp = (unsigned int)dtemp2;
						//utemp = (unsigned short)lutemp;
						//uctemp = (unsigned char)utemp;
						//A = uctemp;
						//tip: get it into 0-255 range while still double, then cast to uchar
						A = (unsigned char)(unsigned short)(unsigned int)((dtemp - dlo)*d255range);
						//A = (unsigned char)(unsigned short)(unsigned int)dtemp2;
						//printf("[%lf %lu %u %d]  ",dtemp2,lutemp,utemp,(int)uctemp);
					break;
					case CDATATYPE_int: 
						dtemp = (double)(*(long *)voxel);
						A = (unsigned char)(unsigned short)(unsigned int)((dtemp - dlo)*d255range);
					break;
					case CDATATYPE_uint: 
						dtemp = (double)(*(unsigned long *)voxel);
						A = (unsigned char)(unsigned short)(unsigned int)((dtemp - dlo)*d255range);
					break;
					case CDATATYPE_longlong: 
						dtemp = (double)(*(long long *)voxel);
						A = (unsigned char)(unsigned short)(unsigned int)((dtemp - dlo)*d255range);
					break;
					case CDATATYPE_ulonglong: 
						dtemp = (double)(*(unsigned long long *)voxel);
						A = (unsigned char)(unsigned short)(unsigned int)((dtemp - dlo)*d255range);
					break;
					case CDATATYPE_float: 
						dtemp = (double)(*(float *)voxel);
						A = (unsigned char)(unsigned short)(unsigned int)((dtemp - dlo)*d255range);
					break;
					case CDATATYPE_double: 
						dtemp = (double)(*(double *)voxel);
						A = (unsigned char)(unsigned short)(unsigned int)((dtemp - dlo)*d255range);
					break;
					default:
						break;
				}
				counts[(int)A]++; //histogram accumulation


			}
			//this displays nice in texturing3D as 'white bones x-ray'
			rgba[0] = 255;
			rgba[1] = 255;
			rgba[2] = 255;
			rgba[3] = A;
		}
		//print histogram to console
		if(0) for(i=0;i<256;i++)
			if(counts[i] != 0) 
				printf("counts[%ld]=%ld\n",(long)i,(long)counts[i]);
		FREE_IF_NZ(data); //free the raw data we malloced, now that we have rgba, unless we plan to do more processing on scalar values later.
	}
	return TRUE;

}
//<<< NRRD MIT ================================


#if defined(_ANDROID) || defined(ANDROIDNDK)
// sometimes (usually?) we have to flip an image vertically. 
static unsigned char *flipImageVerticallyB(unsigned char *input, int height, int width, int bpp) {
	int i,ii,rowcount;
	unsigned char *sourcerow, *destrow;
	unsigned char * blob;
    
	rowcount = width * bpp; //4; bytes per pixel
	blob = MALLOC(unsigned char*, height * rowcount);
	for(i=0;i<height;i++) {
		ii = height - 1 - i;
		sourcerow = &input[i*rowcount];
		destrow = &blob[ii*rowcount];
		memcpy(destrow,sourcerow,rowcount);
	}
	//FREE_IF_NZ(input);
	return blob;
}
static unsigned char *flipImageVertically(unsigned char *input, int height, int width) {
	return flipImageVerticallyB(input,height,width,4);
}
static unsigned char *expandto4bppfromGray(unsigned char *input, int height, int width, int bpp) {
	int i, j, rowcountin, rowcountout;
	unsigned char *sourcerow, *destrow;
	unsigned char * blob;

	rowcountin = width * bpp; //bytes per pixel
	rowcountout = width * 4;
	blob = MALLOCV(height * rowcountout);
	for (i = 0; i<height; i++) {
		sourcerow = &input[i*rowcountin];
		destrow = &blob[i*rowcountout];
		for (j = 0; j<width; j++) {
			unsigned char *op = &destrow[j * 4];
			op[0] = op[1] = op[2] = sourcerow[j*bpp];
			op[3] = bpp == 1 ? 255 : sourcerow[j*bpp + 1];
		}
	}
	//FREE_IF_NZ(input);
	return blob;
}
static unsigned char *expandto4bppfromRGB(unsigned char *input, int height, int width, int bpp) {
	int i, j, rowcountin, rowcountout;
	unsigned char *sourcerow, *destrow;
	unsigned char * blob;

	rowcountin = width * bpp; //bytes per pixel
	rowcountout = width * 4;
	blob = MALLOCV(height * rowcountout);
	for (i = 0; i<height; i++) {
		sourcerow = &input[i*rowcountin];
		destrow = &blob[i*rowcountout];
		for(j=0;j<width;j++){
			memcpy(&destrow[j*4], &sourcerow[j*bpp], bpp);
			destrow[j*4 + 3] = 255;
		}
	}
	//FREE_IF_NZ(input);
	return blob;
}
static unsigned char *expandto4bpp(unsigned char *input, int height, int width, int bpp) {
	unsigned char * retval = NULL;
	if(bpp == 1 || bpp == 2)
		retval = expandto4bppfromGray(input, height, width, bpp);
	else //if(bpp == 3)
		retval = expandto4bppfromRGB(input, height, width, bpp);
	return retval;
}
#endif //ANDROID - for flipImageVertically



// OLD_IPHONE_AQUA #if defined (TARGET_AQUA)
// OLD_IPHONE_AQUA /* render from aCGImageRef into a buffer, to get EXACT bits, as a CGImageRef contains only
// OLD_IPHONE_AQUA estimates. */
// OLD_IPHONE_AQUA /* from http://developer.apple.com/qa/qa2007/qa1509.html */
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA static inline double radians (double degrees) {return degrees * M_PI/180;} 
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA int XXX;
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA CGContextRef CreateARGBBitmapContext (CGImageRef inImage) {
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 	CGContextRef    context = NULL;
// OLD_IPHONE_AQUA 	CGColorSpaceRef colorSpace;
// OLD_IPHONE_AQUA 	int             bitmapByteCount;
// OLD_IPHONE_AQUA 	int             bitmapBytesPerRow;
// OLD_IPHONE_AQUA 	CGBitmapInfo	bitmapInfo;
// OLD_IPHONE_AQUA 	size_t		bitsPerComponent;
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 	 // Get image width, height. Well use the entire image.
// OLD_IPHONE_AQUA 	int pixelsWide = (int) CGImageGetWidth(inImage);
// OLD_IPHONE_AQUA 	int pixelsHigh = (int) CGImageGetHeight(inImage);
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 	// Declare the number of bytes per row. Each pixel in the bitmap in this
// OLD_IPHONE_AQUA 	// example is represented by 4 bytes; 8 bits each of red, green, blue, and
// OLD_IPHONE_AQUA 	// alpha.
// OLD_IPHONE_AQUA 	bitmapBytesPerRow   = (pixelsWide * 4);
// OLD_IPHONE_AQUA 	bitmapByteCount     = (bitmapBytesPerRow * pixelsHigh);
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 	// Use the generic RGB color space.
// OLD_IPHONE_AQUA     colorSpace = CGColorSpaceCreateDeviceRGB();
// OLD_IPHONE_AQUA 	if (colorSpace == NULL)
// OLD_IPHONE_AQUA 	{
// OLD_IPHONE_AQUA 	    fprintf(stderr, "Error allocating color space\n");
// OLD_IPHONE_AQUA 	    return NULL;
// OLD_IPHONE_AQUA 	}
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 	
// OLD_IPHONE_AQUA 	/* figure out the bitmap mapping */
// OLD_IPHONE_AQUA 	bitsPerComponent = CGImageGetBitsPerComponent(inImage);
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 	if (bitsPerComponent >= 8) {
// OLD_IPHONE_AQUA 		CGRect rect = CGRectMake(0., 0., pixelsWide, pixelsHigh);
// OLD_IPHONE_AQUA 		bitmapInfo = kCGImageAlphaNoneSkipLast;
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 		bitmapInfo = kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host;
// OLD_IPHONE_AQUA 	
// OLD_IPHONE_AQUA 		/* Create the bitmap context. We want pre-multiplied ARGB, 8-bits
// OLD_IPHONE_AQUA 		  per component. Regardless of what the source image format is
// OLD_IPHONE_AQUA 		  (CMYK, Grayscale, and so on) it will be converted over to the format
// OLD_IPHONE_AQUA 		  specified here by CGBitmapContextCreate. */
// OLD_IPHONE_AQUA 		context = CGBitmapContextCreate (NULL, pixelsWide, pixelsHigh,
// OLD_IPHONE_AQUA 			bitsPerComponent, bitmapBytesPerRow, colorSpace, bitmapInfo); 
// OLD_IPHONE_AQUA 	
// OLD_IPHONE_AQUA 		if (context == NULL) {
// OLD_IPHONE_AQUA 		    fprintf (stderr, "Context not created!");
// OLD_IPHONE_AQUA 		} else {
// OLD_IPHONE_AQUA 	
// OLD_IPHONE_AQUA 			/* try scaling and rotating this image to fit our ideas on life in general */
// OLD_IPHONE_AQUA 			CGContextTranslateCTM (context, 0, pixelsHigh);
// OLD_IPHONE_AQUA 			CGContextScaleCTM (context,1.0, -1.0);
// OLD_IPHONE_AQUA 		}
// OLD_IPHONE_AQUA 		CGContextDrawImage(context, rect,inImage);
// OLD_IPHONE_AQUA 	}
// OLD_IPHONE_AQUA     else
// OLD_IPHONE_AQUA     {
// OLD_IPHONE_AQUA         CGColorSpaceRelease(colorSpace);
// OLD_IPHONE_AQUA 		printf ("bits per component of %d not handled\n",(int) bitsPerComponent);
// OLD_IPHONE_AQUA 		return NULL;
// OLD_IPHONE_AQUA 	}
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 	/* Make sure and release colorspace before returning */
// OLD_IPHONE_AQUA 	CGColorSpaceRelease( colorSpace );
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 	return context;
// OLD_IPHONE_AQUA }
// OLD_IPHONE_AQUA #endif

#ifdef QNX
#include <img/img.h>
static img_lib_t  ilib = NULL;
int loadImage(textureTableIndexStruct_s* tti, char* fname)
{
	int ierr, iret;
	img_t img;
	if(!ilib) ierr = img_lib_attach( &ilib );
	img.format = IMG_FMT_PKLE_ARGB8888;  //GLES2 little endian 32bit - saw in sample code, no idea
	img.flags |= IMG_FORMAT;
	ierr= img_load_file(ilib, fname, NULL, &img);
	iret = 0;
	if(ierr == NULL)
	{

		//deep copy data so browser owns it (and does its FREE_IF_NZ) and we can delete our copy here and forget about it
		tti->x = img.w;
		tti->y = img.h;
		tti->frames = 1;
		tti->texdata = img.access.direct.data;
		if(!tti->texdata)
		   printf("ouch in gdiplus image loader L140 - no image data\n");
		else
		{
			int flipvertically = 1;
			if(flipvertically){
				int i,j,ii,rowcount;
				unsigned char *sourcerow, *destrow;
				unsigned char * blob;
				rowcount = tti->x * 4;
				blob = MALLOCV(img.h * rowcount);
				for(i=0;i<img.h;i++) {
					ii = tti->y - 1 - i;
					sourcerow = &tti->texdata[i*rowcount];
					destrow = &blob[ii*rowcount];
					memcpy(destrow,sourcerow,rowcount);
				}
				tti->texdata = blob;
				//try johns next time: tti->texdata = flipImageVertically(myFile->fileData, myFile->imageHeight, myFile->imageWidth); 

			}
		}
		tti->hasAlpha = 1; //img.transparency; //Gdiplus::IsAlphaPixelFormat(bitmap->GetPixelFormat())?1:0;
		tti->channels = 4; //don't know, don't have img_load_file() function
		//printf("fname=%s alpha=%ld\n",fname,tti->hasAlpha);
		iret = 1;
	}
	return iret;
}

#endif

char* download_file(char* filename);
void close_openned_file(openned_file_t *file);
int load_file_blob(const char *filename, char **blob, int *len);

#if defined(ANDROIDNDK)
#define HAVE_LIBJPEG_H 1
#ifdef HAVE_LIBJPEG_H
#include <jpeglib.h>
#include <setjmp.h>
struct my_error_mgr {
	struct jpeg_error_mgr pub;    /* "public" fields */
	jmp_buf setjmp_buffer; /* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

/*
* Here's the routine that will replace the standard error_exit method:
*/

METHODDEF(void)
my_error_exit(j_common_ptr cinfo)
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	my_error_ptr myerr = (my_error_ptr)cinfo->err;

	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	/* JAS (*cinfo->err->output_message) (cinfo); */

	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}
#define ERROR -1
#define NOT_JPEGT -2
#define JPEG_SUCCESS 0

static int loadImageTexture_jpeg(textureTableIndexStruct_s* this_tex, char *filename) {
	FILE *infile;
	//char *filename;
	GLuint texture_num;
	unsigned char *image_data = 0;


	/* jpeg variables */
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	JDIMENSION nrows;
	JSAMPROW row = 0;
	JSAMPROW rowptr[1];
	unsigned rowcount, columncount;
	int dp;

	int tempInt;


	if ((infile = fopen(filename, "rb")) == NULL) {
		fprintf(stderr, "can't open %s\n", filename);
		return ERROR;
	}

	/* is it a jpeg file */

	/* Select recommended processing options for quick-and-dirty output. */
	//cinfo.two_pass_quantize = FALSE;
	//cinfo.dither_mode = JDITHER_ORDERED;
	//cinfo.desired_number_of_colors = 216;
	//cinfo.dct_method = JDCT_FASTEST;
	//cinfo.do_fancy_upsampling = FALSE;

	/* call my error handler if there is an error */
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	if (setjmp(jerr.setjmp_buffer)) {
		/* if we are here, we have a JPEG error */
		ConsoleMessage("FreeWRL Image problem - could not read %s\n", filename);
		jpeg_destroy_compress((j_compress_ptr)&cinfo);
		fclose(infile);
		return ERROR;
	}


	jpeg_create_decompress(&cinfo);

	/* Specify data source for decompression */
	jpeg_stdio_src(&cinfo, infile);

	/* Read file header, set default decompression parameters */
	/* (void) jpeg_read_header(&cinfo, TRUE); */
	// https://www4.cs.fau.de/Services/Doc/graphics/doc/jpeg/libjpeg.html
	tempInt = jpeg_read_header(&cinfo, TRUE);


	/* Start decompressor */
	(void)jpeg_start_decompress(&cinfo);



	row = (JSAMPLE*)MALLOCV(cinfo.output_width * sizeof(JSAMPLE)*cinfo.output_components);
	rowptr[0] = row;
	image_data = (unsigned char *)MALLOCV(cinfo.output_width * sizeof(JSAMPLE) * cinfo.output_height * cinfo.output_components);
	/* Process data */
	for (rowcount = 0; rowcount < cinfo.output_height; rowcount++) {
		nrows = jpeg_read_scanlines(&cinfo, rowptr, 1);
		/* yield for a bit */
		sched_yield();


		for (columncount = 0; columncount < cinfo.output_width; columncount++) {
			for (dp = 0; dp<cinfo.output_components; dp++) {
				image_data[(cinfo.output_height - rowcount - 1)
					*cinfo.output_width*cinfo.output_components
					+ columncount* cinfo.output_components + dp]
					= row[columncount*cinfo.output_components + dp];
			}
		}
	}

	int iret = JPEG_SUCCESS;
	if (jpeg_finish_decompress(&cinfo) != TRUE) {
		printf("warning: jpeg_finish_decompress error\n");
		//releaseTexture(loadThisTexture->scenegraphNode);
		iret = ERROR;
	}


	//store_tex_info(loadThisTexture,
	//	cinfo.output_components,
	//	(int)cinfo.output_width,
	//	(int)cinfo.output_height, image_data, cinfo.output_components == 4);
	fclose(infile);

	this_tex->x = (int)cinfo.output_width;
	this_tex->y = (int)cinfo.output_height;
	this_tex->hasAlpha = 0; //jpeg doesn't have alpha?
	//int bpp = this_tex->hasAlpha ? 4 :  3; //bytes per pixel
	int bpp = cinfo.output_components; //4
	this_tex->channels = bpp; //3; //always RGB?

	//char *dataflipped = flipImageVerticallyB(image_data, this_tex->y, this_tex->x, bpp);
	char *data4bpp = expandto4bpp(image_data,this_tex->y,this_tex->x,bpp);
	//free(image_data);
	this_tex->frames = 1;
	this_tex->texdata = data4bpp;
	FREE_IF_NZ(image_data);
	this_tex->filename = filename;

	jpeg_destroy_decompress(&cinfo);
	FREE_IF_NZ(row);

	return JPEG_SUCCESS;
}



#endif //HAVE_LIBJPEG_H

#define HAVE_LIBPNG_H 1
#ifdef HAVE_LIBPNG_H
#include <png.h>

#define ERROR -1
#define NOT_PNG -2
#define PNG_SUCCESS 0
// http://www.learnopengles.com/loading-a-png-into-memory-and-displaying-it-as-a-texture-with-opengl-es-2-using-almost-the-same-code-on-ios-android-and-emscripten/
typedef struct {
	const png_byte* data;
	const png_size_t size;
} DataHandle;

typedef struct {
	const DataHandle data;
	png_size_t offset;
} ReadDataHandle;

typedef struct {
	const png_uint_32 width;
	const png_uint_32 height;
	const int color_type;
} PngInfo;
static GLenum get_gl_color_format(const int png_color_format) {
	//assert(png_color_format == PNG_COLOR_TYPE_GRAY
	//	|| png_color_format == PNG_COLOR_TYPE_RGB_ALPHA
	//	|| png_color_format == PNG_COLOR_TYPE_GRAY_ALPHA);

	switch (png_color_format) {
	case PNG_COLOR_TYPE_GRAY:
		return GL_LUMINANCE;
	case PNG_COLOR_TYPE_RGB_ALPHA:
		return GL_RGBA;
	case PNG_COLOR_TYPE_GRAY_ALPHA:
		return GL_LUMINANCE_ALPHA;
	case PNG_COLOR_TYPE_RGB:
		return GL_RGB;
	}

	return 0;
}

static PngInfo read_and_update_info(const png_structp png_ptr, const png_infop info_ptr)
{
	png_uint_32 width, height;
	int bit_depth, color_type;

	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(
		png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

	// Convert transparency to full alpha
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);

	// Convert grayscale, if needed.
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(png_ptr);

	// Convert paletted images, if needed.
	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);

	// Add alpha channel, if there is none.
	// Rationale: GL_RGBA is faster than GL_RGB on many GPUs)
	if (color_type == PNG_COLOR_TYPE_PALETTE || color_type == PNG_COLOR_TYPE_RGB)
		png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER);

	// Ensure 8-bit packing
	if (bit_depth < 8)
		png_set_packing(png_ptr);
	else if (bit_depth == 16)
		png_set_scale_16(png_ptr);

	png_read_update_info(png_ptr, info_ptr);

	// Read the new color type after updates have been made.
	color_type = png_get_color_type(png_ptr, info_ptr);

	return (PngInfo) { width, height, color_type };
}
static DataHandle read_entire_png_image(
	const png_structp png_ptr,
	const png_infop info_ptr,
	const png_uint_32 height)
{
	const png_size_t row_size = png_get_rowbytes(png_ptr, info_ptr);
	const int data_length = row_size * height;
	assert(row_size > 0);

	png_byte* raw_image = malloc(data_length);
	assert(raw_image != NULL);

	png_byte* row_ptrs[height];

	png_uint_32 i;
	for (i = 0; i < height; i++) {
		row_ptrs[i] = raw_image + i * row_size;
	}

	png_read_image(png_ptr, &row_ptrs[0]);

	return (DataHandle) { raw_image, data_length };
}
static void read_png_data_callback(
	png_structp png_ptr, png_byte* raw_data, png_size_t read_length) {
	ReadDataHandle* handle = png_get_io_ptr(png_ptr);
	const png_byte* png_src = handle->data.data + handle->offset;

	memcpy(raw_data, png_src, read_length);
	handle->offset += read_length;
}
enum {
TACTIC_FROM_FILE = 1,
TACTIC_FROM_BLOB = 2,
};
static int loadImageTexture_png(textureTableIndexStruct_s* this_tex, char *filename) {
	FILE *fp;
	//char *filename;
	GLuint texture_num;
	unsigned char *image_data = 0;
	int image_data_isMalloced;

	/* png reading variables */
	int rc;
	unsigned long image_width = 0;
	unsigned long image_height = 0;
	unsigned long image_rowbytes = 0;
	int image_channels = 0;
	int glcolortype = 0;
	double display_exponent = 0.0;
	char * png_data;
	int png_data_size;
	int is_png;
	int tactic;
	int tempInt;
	tactic= TACTIC_FROM_BLOB;


	png_structp png_ptr = png_create_read_struct(
		PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info_ptr = png_create_info_struct(png_ptr);
	
	//from memory (if from file there s png_set_io
	if(tactic == TACTIC_FROM_FILE){
		char header[8];
		fp = fopen(filename,"rb");
		fread(header, 1, 8, fp);
		is_png = !png_sig_cmp(png_data, 0, 8);
		fclose(fp);
		if (!is_png)
		{
			return (NOT_PNG);
		}
		fp = fopen(filename,"rb");
		png_init_io(png_ptr, fp);
	} else if(tactic == TACTIC_FROM_BLOB){
		if (!load_file_blob(filename, &png_data, &png_data_size)) {
			return ERROR;
		}
		is_png = !png_sig_cmp(png_data, 0, 8);
		if (!is_png)
		{
			return (NOT_PNG);
		}
		ReadDataHandle png_data_handle = (ReadDataHandle) { {png_data, png_data_size}, 0 };
		png_set_read_fn(png_ptr, &png_data_handle, read_png_data_callback);
	}

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr,
			(png_infopp)NULL);
		if (tactic == TACTIC_FROM_FILE) fclose(fp);
		return (ERROR);
	}

	//png_read_png(png_ptr, info_ptr, 0, NULL);

	//image_data = readpng_get_image(display_exponent, &image_channels,
	//	&image_rowbytes);
	const PngInfo png_info = read_and_update_info(png_ptr, info_ptr);
	const DataHandle raw_image = read_entire_png_image(
		png_ptr, info_ptr, png_info.height);

	png_read_end(png_ptr, info_ptr);
	this_tex->x = png_info.width;
	this_tex->y = png_info.height;
	//glcolortype = get_gl_color_format(png_info.color_type);
	//this_tex->hasAlpha = png_info.color_type == GL_RGBA || png_info.color_type == GL_LUMINANCE_ALPHA;
	//switch(glcolortype) { //png_info.color_type){
	//	case GL_LUMINANCE: this_tex->channels = 1; break;
	//	case GL_LUMINANCE_ALPHA: this_tex->channels = 2; break;
	//	case GL_RGB: this_tex->channels = 3; break;
	//	case GL_RGBA: this_tex->channels = 4; break;
	//	default:
	//		this_tex->channels = 4; break;
	//}
	image_channels = 4;
	switch (png_info.color_type) {
		case PNG_COLOR_TYPE_GRAY:		image_channels = 1; break;
		case PNG_COLOR_TYPE_GRAY_ALPHA:	image_channels = 2; break;
		case PNG_COLOR_TYPE_RGB:		image_channels = 3; break;
		case PNG_COLOR_TYPE_RGB_ALPHA:	image_channels = 4; break;
		default:
			image_channels = 4;
	}
	this_tex->channels = image_channels;
	this_tex->hasAlpha = this_tex->channels == 2 || this_tex->channels == 4;
	//int bpp = this_tex->hasAlpha ? 4 :  3; //bytes per pixel
	image_data = raw_image.data;

	image_data_isMalloced = 0;
	if(image_channels < 4){
		image_data = expandto4bpp(image_data, this_tex->y, this_tex->x, image_channels);
		image_data_isMalloced = 1;
	}
	int bpp = 4;
	unsigned char *dataflipped = flipImageVerticallyB(image_data, this_tex->y, this_tex->x, bpp);
	free(raw_image.data);
	if(image_data_isMalloced) free(image_data);
	this_tex->frames = 1;
	this_tex->texdata = dataflipped;
	this_tex->filename = filename;
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);


	//readpng_cleanup(FALSE);

	if (tactic == TACTIC_FROM_FILE) fclose(fp);
	return PNG_SUCCESS;
}

#endif //HAVE_LIBPNG_H

#define HAVE_LIBGIF_H 1
#ifdef HAVE_LIBGIF_H
#include <gif_lib.h>  //loads stdbool.h
// http://cd.textfiles.com/amigaplus/lesercd16/Tools/Development/ming0_2/util/gif2dbl.c

int getTransparentColor(GifFileType * file)
{
	//get the color index of transparent 
	int i;
	ExtensionBlock * ext = file->SavedImages[0].ExtensionBlocks;

	for (i = 0; i < file->SavedImages[0].ExtensionBlockCount; i++, ext++) {

		if (ext->Function == GRAPHICS_EXT_FUNC_CODE) {
			if (ext->Bytes[0] & 1)	// there is a transparent color 
				return ext->Bytes[3];	// here it is
		}
	}

	return -1;
}


//#define GIF_ERROR -1
#define NOT_GIF -2
#define GIF_SUCCESS 0

static int loadImageTexture_gif(textureTableIndexStruct_s* this_tex, char *filename) {
// http://giflib.sourceforge.net/gif_lib.html#idm46571690371280

	int ErrorCode, alpha, iret;
	int
	InterlacedOffset[] = { 0, 4, 2, 1 }, /* The way Interlaced image should. */
	InterlacedJumps[] = { 8, 8, 4, 2 };    /* be read - offsets and jumps... */
	ColorMapObject *ColorMap;
	GifRowType *ScreenBuffer;
	int Error;

	GifFileType *GifFile = DGifOpenFileName(filename, &ErrorCode);
	if(!GifFile){
		return GIF_ERROR;
	}
	if (GifFile->SHeight == 0 || GifFile->SWidth == 0) {
		return GIF_ERROR;
	}

	ErrorCode = DGifSlurp(GifFile);
	if(ErrorCode != GIF_OK)
		return GIF_ERROR;
	alpha = getTransparentColor(GifFile);
	iret = GIF_ERROR;
	ColorMap = (GifFile->Image.ColorMap
		? GifFile->Image.ColorMap
		: GifFile->SColorMap);
	if (ColorMap == NULL) {
		return GIF_ERROR;
	}

	if(GifFile->ImageCount){
		unsigned char *pixel;
		int i,j,ipix,icolor;
		
		unsigned char * raw = GifFile->SavedImages[0].RasterBits;
		int width = GifFile->SavedImages[0].ImageDesc.Width;
		int height = GifFile->SavedImages[0].ImageDesc.Height;
		GifColorType *Colors = ColorMap->Colors;
		unsigned char *rgba = MALLOCV(width * height * 4);
		GifColorType *color;

		for(i=0;i<height;i++){
			for(j=0;j<width;j++){
				ipix = i*width + j;
				pixel = &rgba[ipix*4];
				icolor = raw[ipix];
				color = &Colors[icolor];
				pixel[0] = color->Red;
				pixel[1] = color->Green;
				pixel[2] = color->Blue;
				pixel[3] = icolor == alpha ? 0 : 255;
			}
		}
		this_tex->x = width;
		this_tex->y = height;
		this_tex->hasAlpha = alpha > -1 ? 1 : 0; //jpeg doesn't have alpha?
		this_tex->channels = 3 + this_tex->hasAlpha;
		this_tex->frames = 1;
		int bpp = 4;
		char *dataflipped = flipImageVerticallyB(rgba, this_tex->y, this_tex->x, bpp);
		FREE_IF_NZ(rgba);
		this_tex->texdata = dataflipped;
		this_tex->filename = filename;
		iret = GIF_SUCCESS;
	}
	return iret;
	
}
#define bool int //set back to freewrl convention
#endif //HAVE_LIBGIF_H


static void __reallyloadImageTexture(textureTableIndexStruct_s* this_tex, char *filename) {
//filenames coming in can be temp file names - scrambled
//there are 3 ways to tell in the backend what type of image file:
//a) .xxx original filename suffix
//b) MIME type 
//c) file signature https://en.wikipedia.org/wiki/List_of_file_signatures
// right now we aren't passing in the .xxx or mime or signature bytes
// except through the file conents we can get the signature
	char header[20];
	FILE* fp = fopen(filename,"rb");
	fread(header,20,1,fp);
	fclose(fp);

#ifdef HAVE_LIBPNG_H
	if(!strncmp(&header[1],"PNG",3))
		loadImageTexture_png(this_tex, filename);
#endif
#ifdef HAVE_LIBJPEG_H
	if(!strncmp(header,"���",3))
		loadImageTexture_jpeg(this_tex, filename);
#endif
#ifdef HAVE_LIBGIF_H
	if(!strncmp(header,"GIF",3))
		loadImageTexture_gif(this_tex, filename);
#endif 
	return;
}


#endif // ANDROIDNDK



/**
 *   texture_load_from_file: a local filename has been found / downloaded,
 *                           load it now.
 */
int textureIsDDS(textureTableIndexStruct_s* this_tex, char *filename); 
bool texture_load_from_file(textureTableIndexStruct_s* this_tex, char *filename)
{

/* Android, put it here... */
	
#if defined(ANDROIDNDK)
	int imtype, ret;
	char * fname = STRDUP(filename);
	imtype = sniffImageFileHeader(fname);

	ret = FALSE;
	switch(imtype){
		case IMAGETYPE_PNG:
			#ifdef HAVE_LIBPNG_H
			ret = loadImageTexture_png(this_tex, filename);
			#endif
			break;
		case IMAGETYPE_JPEG:
			#ifdef HAVE_LIBJPEG_H
			ret = loadImageTexture_jpeg(this_tex, filename);
			#endif
			break;
		case IMAGETYPE_GIF:
			#ifdef HAVE_LIBGIF_H
			ret = loadImageTexture_gif(this_tex, filename);
			#endif
			break;
		case IMAGETYPE_DDS:
			ret = textureIsDDS(this_tex, fname); break;
		case IMAGETYPE_WEB3DIT:
			ret = loadImage_web3dit(this_tex,fname); break;
		case IMAGETYPE_NRRD:
			ret = loadImage_nrrd(this_tex,fname); 
			break;
		case IMAGETYPE_VOL:
			ret = loadImage3DVol(this_tex, fname); break;
		case IMAGETYPE_UNKNOWN:
		default:
			ret = FALSE;
	}

	//if(loadImage_web3dit(this_tex,fname)){
	//	return TRUE;
	//}
	//if (loadImage3DVol(this_tex, fname))
	//	return TRUE;
	//if (textureIsDDS(this_tex, fname)) {
	//	//saveImage3D_x3di3d(this_tex,"temp2.x3di3d"); //good for testing round trip
	//	return TRUE;
	//}

	//__reallyloadImageTexture(this_tex, filename);

	/*
	// if we got null for data, lets assume that there was not a file there
	if (myFile->fileData == NULL) {
		result = FALSE;
	}
	else {
		this_tex->texdata = flipImageVertically(myFile->fileData, myFile->imageHeight, myFile->imageWidth);

		this_tex->filename = filename;
		this_tex->hasAlpha = myFile->imageAlpha;
		this_tex->frames = 1;
		this_tex->x = myFile->imageWidth;
		this_tex->y = myFile->imageHeight;
		result = TRUE;
	}
	//close_openned_file(myFile);
	FREE_IF_NZ(myFile);
	*/
	//return (ret != 0); // 
	return this_tex->frames;

#endif //ANDROIDNDK



#if defined(_ANDROID)
	unsigned char *image = NULL;
	unsigned char *imagePtr;
	int i;

	openned_file_t *myFile = load_file (filename);
    bool result = FALSE;
	/* if we got null for data, lets assume that there was not a file there */
	if (myFile->fileData == NULL) {
		result = FALSE;
	} else {
		//this_tex->texdata = MALLOC(unsigned char*,myFile->fileDataSize);
		//memcpy(this_tex->texdata,myFile->fileData,myFile->fileDataSize);
/*
{char me[200]; sprintf(me,"texture_load, %d * %d * 4 = %d, is it %d??",myFile->imageHeight, myFile->imageWidth,
			myFile->imageHeight*myFile->imageWidth*4, myFile->fileDataSize);
ConsoleMessage(me);}
*/

		this_tex->texdata = flipImageVertically(myFile->fileData, myFile->imageHeight, myFile->imageWidth); 

		this_tex->filename = filename;
		this_tex->hasAlpha = myFile->imageAlpha;
		this_tex->channels = 4; //don't know but but someone might. I added opened_files_t.imageChannels in case
		this_tex->frames = 1;
		this_tex->x = myFile->imageWidth;
		this_tex->y = myFile->imageHeight;
		result = TRUE;
	}
#ifdef FRONTEND_GETS_FILES
	close_openned_file(myFile);
	FREE_IF_NZ(myFile);
#endif
    return result;

#endif //ANDROID



/* WINDOWS */
#if defined (_MSC_VER) 
	char *fname;
	int ret, imtype;

	fname = STRDUP(filename);
	imtype = sniffImageFileHeader(fname);

	ret = FALSE;
	switch(imtype){
		case IMAGETYPE_PNG:
		case IMAGETYPE_JPEG:
		case IMAGETYPE_GIF:
			ret = loadImage(this_tex, fname); 
			#ifndef GL_ES_VERSION_2_0
			texture_swap_B_R(this_tex); //just for windows desktop gdiplusimage loading
			#endif
			{
				int nchan;
				if(imtype == IMAGETYPE_JPEG){
					nchan = 3; //jpeg always rgb, no alpha
				}else{
					nchan = sniffImageChannels_bruteForce(this_tex->texdata, this_tex->x, this_tex->y); 
				}
				if(nchan > -1) this_tex->channels = nchan;
			}
			break;
		case IMAGETYPE_DDS:
			ret = textureIsDDS(this_tex, fname); break;
		case IMAGETYPE_WEB3DIT:
			ret = loadImage_web3dit(this_tex,fname); break;
		case IMAGETYPE_NRRD:
			ret = loadImage_nrrd(this_tex,fname); break;
		case IMAGETYPE_VOL:
			ret = loadImage3DVol(this_tex, fname); break;
		case IMAGETYPE_UNKNOWN:
		default:
			ret = FALSE;
	}

	FREE(fname);
	return (ret != 0);

#endif


/* LINUX */
// OLD_IPHONE_AQUA #if !defined (_MSC_VER) && !defined (TARGET_AQUA) && !defined(_ANDROID) && !defined(ANDROIDNDK)

#if !defined (_MSC_VER) && !defined(_ANDROID) && !defined(ANDROIDNDK)
	Imlib_Image image;
	Imlib_Load_Error error_return;
	char *fname;
	int ret, imtype;

	fname = STRDUP(filename);
	imtype = sniffImageFileHeader(fname);
	ret = FALSE;

	switch(imtype){
		case IMAGETYPE_DDS:
			ret = textureIsDDS(this_tex, fname); break;
		case IMAGETYPE_WEB3DIT:
			ret = loadImage_web3dit(this_tex,fname); break;
		case IMAGETYPE_NRRD:
			ret = loadImage_nrrd(this_tex,fname); break;
		case IMAGETYPE_VOL:
			ret = loadImage3DVol(this_tex, fname); break;
		case IMAGETYPE_PNG:
		case IMAGETYPE_JPEG:
		case IMAGETYPE_GIF:
		case IMAGETYPE_UNKNOWN:
		default:
			ret = FALSE;
			//image = imlib_load_image_immediately(filename);
			//image = imlib_load_image(filename);
			image = imlib_load_image_with_error_return(filename,&error_return);

			if (!image) {
				char *es = NULL;
				switch(error_return){
					case IMLIB_LOAD_ERROR_NONE: es = "IMLIB_LOAD_ERROR_NONE";break;
					case IMLIB_LOAD_ERROR_FILE_DOES_NOT_EXIST: es = "IMLIB_LOAD_ERROR_FILE_DOES_NOT_EXIST";break;
					case IMLIB_LOAD_ERROR_FILE_IS_DIRECTORY: es = "IMLIB_LOAD_ERROR_FILE_IS_DIRECTORY";break;
					case IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_READ: es = "IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_READ";break;
					case IMLIB_LOAD_ERROR_NO_LOADER_FOR_FILE_FORMAT: es = "IMLIB_LOAD_ERROR_NO_LOADER_FOR_FILE_FORMAT";break;
					case IMLIB_LOAD_ERROR_PATH_TOO_LONG: es = "IMLIB_LOAD_ERROR_PATH_TOO_LONG";break;
					case IMLIB_LOAD_ERROR_PATH_COMPONENT_NON_EXISTANT: es = "IMLIB_LOAD_ERROR_PATH_COMPONENT_NON_EXISTANT";break;
					case IMLIB_LOAD_ERROR_PATH_COMPONENT_NOT_DIRECTORY: es = "IMLIB_LOAD_ERROR_PATH_COMPONENT_NOT_DIRECTORY";break;
					case IMLIB_LOAD_ERROR_PATH_POINTS_OUTSIDE_ADDRESS_SPACE: es = "IMLIB_LOAD_ERROR_PATH_POINTS_OUTSIDE_ADDRESS_SPACE";break;
					case IMLIB_LOAD_ERROR_TOO_MANY_SYMBOLIC_LINKS: es = "IMLIB_LOAD_ERROR_TOO_MANY_SYMBOLIC_LINKS";break;
					case IMLIB_LOAD_ERROR_OUT_OF_MEMORY: es = "IMLIB_LOAD_ERROR_OUT_OF_MEMORY";break;
					case IMLIB_LOAD_ERROR_OUT_OF_FILE_DESCRIPTORS: es = "IMLIB_LOAD_ERROR_OUT_OF_FILE_DESCRIPTORS";break;
					case IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_WRITE: es = "IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_WRITE";break;
					case IMLIB_LOAD_ERROR_OUT_OF_DISK_SPACE: es = "IMLIB_LOAD_ERROR_OUT_OF_DISK_SPACE";break;
					case IMLIB_LOAD_ERROR_UNKNOWN:
					default:
					es = "IMLIB_LOAD_ERROR_UNKNOWN";break;
				}
				ERROR_MSG("imlib load error = %d %s\n",error_return,es);
				ERROR_MSG("load_texture_from_file: failed to load image: %s\n", filename);
				return FALSE;
			}
			DEBUG_TEX("load_texture_from_file: Imlib2 succeeded to load image: %s\n", filename);

			imlib_context_set_image(image);
			imlib_image_flip_vertical(); /* FIXME: do we really need this ? */

			/* store actual filename, status, ... */
			this_tex->filename = filename;
			this_tex->hasAlpha = (imlib_image_has_alpha() == 1);
			this_tex->channels = this_tex->hasAlpha ? 4 : 3;
			this_tex->frames = 1;
			this_tex->x = imlib_image_get_width();
			this_tex->y = imlib_image_get_height();

			this_tex->texdata = (unsigned char *) imlib_image_get_data_for_reading_only(); 
			{
				int nchan, imtype;
				if(imtype == IMAGETYPE_JPEG)
					nchan = 3; //jpeg always rgb, no alpha
				else
					nchan = sniffImageChannels_bruteForce(this_tex->texdata, this_tex->x, this_tex->y); 
				//nchan = sniffImageChannels(fname);
				if(nchan > -1) this_tex->channels = nchan;
			}
			//(Sept 5, 2016 change) assuming imlib gives BGRA:
			texture_swap_B_R(this_tex); 
			//this_tex->data should now be RGBA. (if not comment above line)
			break;
	}

	FREE(fname);
	return (ret != 0);


#endif

// OLD_IPHONE_AQUA /* OSX */
// OLD_IPHONE_AQUA #if defined (TARGET_AQUA)
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 	CGImageRef 	image;
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 	int 		image_width;
// OLD_IPHONE_AQUA 	int 		image_height;
// OLD_IPHONE_AQUA 	int			channels;
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA #ifndef FRONTEND_GETS_FILES
// OLD_IPHONE_AQUA 	CFStringRef	path;
// OLD_IPHONE_AQUA     CFURLRef 	url;
// OLD_IPHONE_AQUA #endif
// OLD_IPHONE_AQUA     
// OLD_IPHONE_AQUA 	CGContextRef 	cgctx;
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 	unsigned char *	data;
// OLD_IPHONE_AQUA 	int		hasAlpha;
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 	CGImageSourceRef 	sourceRef;
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 	/* initialization */
// OLD_IPHONE_AQUA 	image = NULL;
// OLD_IPHONE_AQUA 	hasAlpha = FALSE;
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA #ifdef FRONTEND_GETS_FILES
// OLD_IPHONE_AQUA 	openned_file_t *myFile = load_file (filename);
// OLD_IPHONE_AQUA 	/* printf ("got file from load_file, openned_file_t is %p %d\n", myFile->fileData, myFile->fileDataSize); */
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 	/* if we got null for data, lets assume that there was not a file there */
// OLD_IPHONE_AQUA 	if (myFile->fileData == NULL) {
// OLD_IPHONE_AQUA 		sourceRef = NULL;
// OLD_IPHONE_AQUA 		image = NULL;
// OLD_IPHONE_AQUA 	} else {
// OLD_IPHONE_AQUA 		//CFDataRef localData = CFDataCreate(NULL,(const UInt8 *)myFile->fileData,myFile->fileDataSize);
// OLD_IPHONE_AQUA 		CFDataRef localData = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, (const UInt8 *)myFile->fileData,myFile->fileDataSize, kCFAllocatorNull);
// OLD_IPHONE_AQUA 		sourceRef = CGImageSourceCreateWithData(localData,NULL);
// OLD_IPHONE_AQUA 		CFRelease(localData);
// OLD_IPHONE_AQUA 	}
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 	/* step 2, if the data exists, was it a file for us? */
// OLD_IPHONE_AQUA 	if (sourceRef != NULL) {
// OLD_IPHONE_AQUA 		image = CGImageSourceCreateImageAtIndex(sourceRef, 0, NULL);
// OLD_IPHONE_AQUA 		CFRelease (sourceRef);
// OLD_IPHONE_AQUA 	}
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA #else /* FRONTEND_GETS_FILES */
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 	path = CFStringCreateWithCString(NULL, filename, kCFStringEncodingUTF8);
// OLD_IPHONE_AQUA 	url = CFURLCreateWithFileSystemPath (NULL, path, kCFURLPOSIXPathStyle, 0);
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 	/* ok, we can define USE_CG_DATA_PROVIDERS or TRY_QUICKTIME...*/
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 	/* I dont know whether to use quicktime or not... Probably not... as the other ways using core 
// OLD_IPHONE_AQUA 		graphics seems to be ok. Anyway, I left this code in here, as maybe it might be of use for mpegs
// OLD_IPHONE_AQUA 	*/
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 	sourceRef = CGImageSourceCreateWithURL(url,NULL);
// OLD_IPHONE_AQUA 	if (sourceRef != NULL) {
// OLD_IPHONE_AQUA 		image = CGImageSourceCreateImageAtIndex(sourceRef, 0, NULL);
// OLD_IPHONE_AQUA 		CFRelease (sourceRef);
// OLD_IPHONE_AQUA 	}
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 	CFRelease(url);
// OLD_IPHONE_AQUA 	CFRelease(path);
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA #endif /* FRONTEND_GETS_FILES */
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 	/* We were able to load in the image here... */
// OLD_IPHONE_AQUA 	if (image != NULL) {
// OLD_IPHONE_AQUA 		image_width = (int) CGImageGetWidth(image);
// OLD_IPHONE_AQUA 		image_height = (int) CGImageGetHeight(image);
// OLD_IPHONE_AQUA 		// https://developer.apple.com/reference/coregraphics/1408848-cgcolorspacegetnumberofcomponent?language=objc
// OLD_IPHONE_AQUA 		// https://developer.apple.com/reference/coregraphics/1454858-cgimagegetcolorspace?language=objc
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 		channels = CGColorSpaceGetNumberOfComponents(CGImageGetColorSpace(image));
// OLD_IPHONE_AQUA 		/* go through every possible return value and check alpha. 
// OLD_IPHONE_AQUA 			note, in testing, kCGImageAlphaLast and kCGImageAlphaNoneSkipLast
// OLD_IPHONE_AQUA 			are what got returned - which makes sense for BGRA textures */
// OLD_IPHONE_AQUA 		switch (CGImageGetAlphaInfo(image)) {
// OLD_IPHONE_AQUA 			case kCGImageAlphaNone: hasAlpha = FALSE; break;
// OLD_IPHONE_AQUA 			case kCGImageAlphaPremultipliedLast: hasAlpha = TRUE; break;
// OLD_IPHONE_AQUA 			case kCGImageAlphaPremultipliedFirst: hasAlpha = TRUE; break;
// OLD_IPHONE_AQUA 			case kCGImageAlphaLast: hasAlpha = TRUE; break;
// OLD_IPHONE_AQUA 			case kCGImageAlphaFirst: hasAlpha = TRUE; break;
// OLD_IPHONE_AQUA 			case kCGImageAlphaNoneSkipLast: hasAlpha = FALSE; break;
// OLD_IPHONE_AQUA 			case kCGImageAlphaNoneSkipFirst: hasAlpha = FALSE; break;
// OLD_IPHONE_AQUA 			default: hasAlpha = FALSE; /* should never get here */
// OLD_IPHONE_AQUA 		}
// OLD_IPHONE_AQUA 	
// OLD_IPHONE_AQUA 		#ifdef TEXVERBOSE
// OLD_IPHONE_AQUA 		printf ("\nLoadTexture %s\n",filename);
// OLD_IPHONE_AQUA 		printf ("CGImageGetAlphaInfo(image) returns %x\n",CGImageGetAlphaInfo(image));
// OLD_IPHONE_AQUA 		printf ("   kCGImageAlphaNone %x\n",   kCGImageAlphaNone);
// OLD_IPHONE_AQUA 		printf ("   kCGImageAlphaPremultipliedLast %x\n",   kCGImageAlphaPremultipliedLast);
// OLD_IPHONE_AQUA 		printf ("   kCGImageAlphaPremultipliedFirst %x\n",   kCGImageAlphaPremultipliedFirst);
// OLD_IPHONE_AQUA 		printf ("   kCGImageAlphaLast %x\n",   kCGImageAlphaLast);
// OLD_IPHONE_AQUA 		printf ("   kCGImageAlphaFirst %x\n",   kCGImageAlphaFirst);
// OLD_IPHONE_AQUA 		printf ("   kCGImageAlphaNoneSkipLast %x\n",   kCGImageAlphaNoneSkipLast);
// OLD_IPHONE_AQUA 		printf ("   kCGImageAlphaNoneSkipFirst %x\n",   kCGImageAlphaNoneSkipFirst);
// OLD_IPHONE_AQUA 	
// OLD_IPHONE_AQUA 		if (hasAlpha) printf ("Image has Alpha channel\n"); else printf ("image - no alpha channel \n");
// OLD_IPHONE_AQUA 	
// OLD_IPHONE_AQUA 		printf ("raw image, AlphaInfo %x\n",(int) CGImageGetAlphaInfo(image));
// OLD_IPHONE_AQUA 		printf ("raw image, BitmapInfo %x\n",(int) CGImageGetBitmapInfo(image));
// OLD_IPHONE_AQUA 		printf ("raw image, BitsPerComponent %d\n",(int) CGImageGetBitsPerComponent(image));
// OLD_IPHONE_AQUA 		printf ("raw image, BitsPerPixel %d\n",(int) CGImageGetBitsPerPixel(image));
// OLD_IPHONE_AQUA 		printf ("raw image, BytesPerRow %d\n",(int) CGImageGetBytesPerRow(image));
// OLD_IPHONE_AQUA 		printf ("raw image, ImageHeight %d\n",(int) CGImageGetHeight(image));
// OLD_IPHONE_AQUA 		printf ("raw image, ImageWidth %d\n",(int) CGImageGetWidth(image));
// OLD_IPHONE_AQUA 		#endif
// OLD_IPHONE_AQUA 		
// OLD_IPHONE_AQUA 	
// OLD_IPHONE_AQUA 	
// OLD_IPHONE_AQUA 		/* now, lets "draw" this so that we get the exact bit values */
// OLD_IPHONE_AQUA 		cgctx = CreateARGBBitmapContext(image);
// OLD_IPHONE_AQUA 	
// OLD_IPHONE_AQUA 		 
// OLD_IPHONE_AQUA 		#ifdef TEXVERBOSE
// OLD_IPHONE_AQUA 		printf ("GetAlphaInfo %x\n",(int) CGBitmapContextGetAlphaInfo(cgctx));
// OLD_IPHONE_AQUA 		printf ("GetBitmapInfo %x\n",(int) CGBitmapContextGetBitmapInfo(cgctx));
// OLD_IPHONE_AQUA 		printf ("GetBitsPerComponent %d\n",(int) CGBitmapContextGetBitsPerComponent(cgctx));
// OLD_IPHONE_AQUA 		printf ("GetBitsPerPixel %d\n",(int) CGBitmapContextGetBitsPerPixel(cgctx));
// OLD_IPHONE_AQUA 		printf ("GetBytesPerRow %d\n",(int) CGBitmapContextGetBytesPerRow(cgctx));
// OLD_IPHONE_AQUA 		printf ("GetHeight %d\n",(int) CGBitmapContextGetHeight(cgctx));
// OLD_IPHONE_AQUA 		printf ("GetWidth %d\n",(int) CGBitmapContextGetWidth(cgctx));
// OLD_IPHONE_AQUA 		#endif
// OLD_IPHONE_AQUA 		
// OLD_IPHONE_AQUA 		data = (unsigned char *)CGBitmapContextGetData(cgctx);
// OLD_IPHONE_AQUA 	
// OLD_IPHONE_AQUA /*
// OLD_IPHONE_AQUA 		#ifdef TEXVERBOSE
// OLD_IPHONE_AQUA 		if (CGBitmapContextGetWidth(cgctx) < 301) {
// OLD_IPHONE_AQUA 			int i;
// OLD_IPHONE_AQUA 	
// OLD_IPHONE_AQUA 			printf ("dumping image\n");
// OLD_IPHONE_AQUA 			for (i=0; i<CGBitmapContextGetBytesPerRow(cgctx)*CGBitmapContextGetHeight(cgctx); i++) {
// OLD_IPHONE_AQUA 				printf ("index:%d data:%2x\n ",i,data[i]);
// OLD_IPHONE_AQUA 			}
// OLD_IPHONE_AQUA 			printf ("\n");
// OLD_IPHONE_AQUA 		}
// OLD_IPHONE_AQUA 		#endif
// OLD_IPHONE_AQUA */
// OLD_IPHONE_AQUA 	
// OLD_IPHONE_AQUA 		/* is there possibly an error here, like a file that is not a texture? */
// OLD_IPHONE_AQUA 		if (CGImageGetBitsPerPixel(image) == 0) {
// OLD_IPHONE_AQUA 			ConsoleMessage ("texture file invalid: %s",filename);
// OLD_IPHONE_AQUA 		}
// OLD_IPHONE_AQUA 	
// OLD_IPHONE_AQUA 		if (data != NULL) {
// OLD_IPHONE_AQUA 			this_tex->filename = filename;
// OLD_IPHONE_AQUA 			this_tex->hasAlpha = hasAlpha;
// OLD_IPHONE_AQUA 			this_tex->channels = channels;
// OLD_IPHONE_AQUA 			this_tex->frames = 1;
// OLD_IPHONE_AQUA 			this_tex->x = image_width;
// OLD_IPHONE_AQUA 			this_tex->y = image_height;
// OLD_IPHONE_AQUA             
// OLD_IPHONE_AQUA             int bitmapBytesPerRow = (image_width * 4);
// OLD_IPHONE_AQUA             size_t bitmapByteCount = (bitmapBytesPerRow * image_height);
// OLD_IPHONE_AQUA             
// OLD_IPHONE_AQUA             unsigned char *	texdata = MALLOC(unsigned char*, bitmapByteCount);
// OLD_IPHONE_AQUA             memcpy(texdata, data, bitmapByteCount);
// OLD_IPHONE_AQUA             
// OLD_IPHONE_AQUA 			this_tex->texdata = texdata;
// OLD_IPHONE_AQUA 		}
// OLD_IPHONE_AQUA 	
// OLD_IPHONE_AQUA 		CGContextRelease(cgctx);
// OLD_IPHONE_AQUA 		CGImageRelease(image);
// OLD_IPHONE_AQUA #ifdef FRONTEND_GETS_FILES
// OLD_IPHONE_AQUA         close_openned_file(myFile);
// OLD_IPHONE_AQUA #endif
// OLD_IPHONE_AQUA 		return TRUE;
// OLD_IPHONE_AQUA 	} else {
// OLD_IPHONE_AQUA #ifdef FRONTEND_GETS_FILES
// OLD_IPHONE_AQUA         close_openned_file(myFile);
// OLD_IPHONE_AQUA         FREE_IF_NZ(myFile);
// OLD_IPHONE_AQUA #endif
// OLD_IPHONE_AQUA 		/* is this, possibly, a dds file for an ImageCubeMap? */
// OLD_IPHONE_AQUA 		return textureIsDDS(this_tex, filename);
// OLD_IPHONE_AQUA 	}
// OLD_IPHONE_AQUA #ifdef FRONTEND_GETS_FILES
// OLD_IPHONE_AQUA         close_openned_file(myFile);
// OLD_IPHONE_AQUA         FREE_IF_NZ(myFile);
// OLD_IPHONE_AQUA #endif
// OLD_IPHONE_AQUA     
// OLD_IPHONE_AQUA #endif

	return FALSE;
}

/**
 *   texture_process_entry: process a texture table entry
 *
 * find the file, either locally or within the Browser. Note that
 * this is almost identical to the one for Inlines, but running
 * in different threads 
 */
static bool texture_process_entry(textureTableIndexStruct_s *entry)
{
	resource_item_t *res;
	resource_type_t restype;
	struct Multi_String *url;
	resource_item_t *parentPath = NULL;

	DEBUG_TEX("textureThread - working on %p (%s)\n"
		  "which is node %p, nodeType %d status %s, opengltex %u, and frames %d\n",
		  entry, entry->filename, entry->scenegraphNode, entry->nodeType, 
		  texst(entry->status), entry->OpenGLTexture, 
		  entry->frames);
	
	entry->status = TEX_LOADING;
	url = NULL;
	res = NULL;

    /* did this node just disappear? */
    if (!checkNode(entry->scenegraphNode,__FILE__,__LINE__)) {
        ConsoleMessage ("node for texture just deleted...\n");
        return FALSE;
    }
    
    
	switch (entry->nodeType) {

	case NODE_PixelTexture:
		texture_load_from_pixelTexture(entry,(struct X3D_PixelTexture *)entry->scenegraphNode);
		//sets TEX_NEEDSBINDING internally
		return TRUE;
		break;

	case NODE_PixelTexture3D:
		texture_load_from_pixelTexture3D(entry,(struct X3D_PixelTexture3D *)entry->scenegraphNode);
		//sets TEX_NEEDSBINDING internally
		return TRUE;
		break;

	case NODE_ImageTexture:
		url = & (((struct X3D_ImageTexture *)entry->scenegraphNode)->url);
		parentPath = (resource_item_t *)(((struct X3D_ImageTexture *)entry->scenegraphNode)->_parentResource);
		restype = resm_image;
		break;

	case NODE_ImageTexture3D:
		url = & (((struct X3D_ImageTexture3D *)entry->scenegraphNode)->url);
		parentPath = (resource_item_t *)(((struct X3D_ImageTexture3D *)entry->scenegraphNode)->_parentResource);
		restype = resm_image;
		break;

	case NODE_ComposedTexture3D:
		return TRUE;
		break;


	case NODE_MovieTexture:
		url = & (((struct X3D_MovieTexture *)entry->scenegraphNode)->url);
		parentPath = (resource_item_t *)(((struct X3D_MovieTexture *)entry->scenegraphNode)->_parentResource);
		entry->status = TEX_NEEDSBINDING; //as with pixeltexture, just do the move_to_opengl part, we load from file elsewhere
		restype = resm_movie;
		return TRUE;  //like pixeltexture - assume the pixels are delivered magically, not from file, so just return
		break;
	case NODE_ImageCubeMapTexture:
		url = & (((struct X3D_ImageCubeMapTexture *)entry->scenegraphNode)->url);
		parentPath = (resource_item_t *)(((struct X3D_ImageCubeMapTexture *)entry->scenegraphNode)->_parentResource);
		restype = resm_image;
		break;

	default: 
		printf ("invalid nodetype given to loadTexture, %s is not valid\n",stringNodeType(entry->nodeType));
	}
	if(!url){
		entry->status = TEX_NOTFOUND;
		return FALSE;
	}

	//TEX_LOADING
	res = resource_create_multi(url);
	res->type=rest_multi;
	res->media_type = restype; //resm_image; /* quick hack */
	resource_identify(parentPath, res);
	res->whereToPlaceData = entry;
	res->textureNumber = entry->textureNumber;
	resitem_enqueue(ml_new(res));
	return TRUE;

}
/*
parsing thread --> texture_loading_thread hand-off
GOAL: texture thread blocks when no textures requested. (rather than sleep(500) and for(;;) )
IT IS AN ERROR TO CALL (condition signal) before calling (condition wait). 
So you might have a global variable bool waiting = false.
1. The threads start, list=null, waiting=false
2. The texture thread loops to lock_mutex line, checks if list=null, 
   if so it sets waiting = true, and sets condition wait, and blocks, 
   waiting for the main thread to give it some texure names
3. The parsing/main thread goes to schedule a texture. It mutex locks, 
   list= add new item. it checks if textureloader is waiting, 
   if so signals condition (which locks momentarily blocks while 
   other thread does something to the list) then unlock mutex.
4. The texture thread gets a signal its waiting on. it copies the list and sets it null, 
   sets waiting =false, and unlocks and does its loading work 
   (on its copy of the list), and goes back around to 2.

*/

/**
 *   texture_process_list_item: process a texture_list item
 */
static void texture_process_list_item(s_list_t *item)
{
	bool remove_it = FALSE;
	textureTableIndexStruct_s *entry;
	// OLDCODE UNUSED ppLoadTextures p = (ppLoadTextures)gglobal()->LoadTextures.prv;

	if (!item || !item->elem)
		return;
	
	entry = ml_elem(item);
	
	DEBUG_TEX("texture_process_list: %s\n", entry->filename);
	
	/* FIXME: it seems there is no case in which we not want to remote it ... */

	switch (entry->status) {
	
	/* JAS - put in the TEX_LOADING flag here - it helps on OSX */
	case TEX_LOADING:
		if (texture_process_entry(entry)) {
			remove_it = TRUE;
		}else{
			remove_it = TRUE; //still remove it 
			// url doesn't exist (or none of multi-url exist)
			// no point in trying again, 
			// you'll just get the same result in a vicious cycle
		}
		break;
	case TEX_READ:
		entry->status = TEX_NEEDSBINDING;
		remove_it = TRUE;
		break;		
	default:
		//DEBUG_MSG("Could not process texture entry: %s\n", entry->filename);
		remove_it = TRUE;
		break;
	}
		
	if (remove_it) {
		/* free the parsed resource and list item */
		//OLDCODE UNUSED p->texture_list = ml_delete_self(p->texture_list, item);
		ml_free(item);
	}
}

void threadsafe_enqueue_item_signal(s_list_t *item, s_list_t** queue, pthread_mutex_t* queue_lock, pthread_cond_t *queue_nonzero);
s_list_t* threadsafe_dequeue_item_wait(s_list_t** queue, pthread_mutex_t *queue_lock, pthread_cond_t *queue_nonzero, int* wait);

void texitem_enqueue(s_list_t *item){
	ppLoadTextures p;
	ttglobal tg = gglobal();
	p = (ppLoadTextures)gglobal()->LoadTextures.prv;

	threadsafe_enqueue_item_signal(item, &p->texture_request_list, &tg->threads.mutex_texture_list, &tg->threads.texture_list_condition);
}
s_list_t *texitem_dequeue(){
	ppLoadTextures p;
	ttglobal tg = gglobal();
	p = (ppLoadTextures)gglobal()->LoadTextures.prv;

	return threadsafe_dequeue_item_wait(&p->texture_request_list, &tg->threads.mutex_texture_list, &tg->threads.texture_list_condition, &tg->threads.TextureThreadWaiting);
}
//we want the void* addresses of the following, so the int value doesn't matter
static const int tex_command_exit;

void texitem_queue_exit(){
	texitem_enqueue(ml_new(&tex_command_exit));
}

void send_texture_to_loader(textureTableIndexStruct_s *entry)
{
	texitem_enqueue(ml_new(entry));
}
textureTableIndexStruct_s *getTableIndex(int i);
void process_res_texitem(resource_item_t *res){
	//resitem after download+load -> texture thread
	textureTableIndexStruct_s *entry;
	int textureNumber;
	textureNumber = res->textureNumber;
	//check in case texture has been deleted due to inline unloading during image download
	//entry = res->whereToPlaceData;
	entry = getTableIndex(textureNumber);
	if(entry)
		texitem_enqueue(ml_new(entry));
}

/**
 *   _textureThread: work on textures, until the end of time.
 */


#if !defined(HAVE_PTHREAD_CANCEL)
void Texture_thread_exit_handler(int sig)
{ 
    ConsoleMessage("Texture_thread_exit_handler: No pTheadCancel - textureThread exiting - maybe should cleanup? Should be done but need to check some rainy day");
    pthread_exit(0);
}
#endif //HAVE_PTHREAD_CANCEL



void _textureThread(void *globalcontext)
{
	ttglobal tg = (ttglobal)globalcontext;
	tg->threads.loadThread = pthread_self();
	fwl_setCurrentHandle(tg, __FILE__, __LINE__);
	//ENTER_THREAD("texture loading");
	{
		ppLoadTextures p;
		//ttglobal tg = gglobal();
		p = (ppLoadTextures)tg->LoadTextures.prv;

		//tg->LoadTextures.TextureThreadInitialized = TRUE;
		tg->threads.TextureThreadRunning = TRUE;

		/* we wait forever for the data signal to be sent */
		for (;;) {
			void* elem;
			s_list_t *item = texitem_dequeue();
			elem = ml_elem(item);
			if (elem == &tex_command_exit){
				FREE_IF_NZ(item);
				break;
			}
			if (tg->threads.flushing){
				FREE_IF_NZ(item);
				continue;
			}
			p->TextureParsing = TRUE;
			texture_process_list_item(item);
			p->TextureParsing = FALSE;
		}
	}
	printf("Ending texture load thread gracefully\n");
	tg->threads.TextureThreadRunning = FALSE;

}
