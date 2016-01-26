/*


X3D Text Component

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

#include <system_fonts.h>

#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h"
#include "../input/InputFunctions.h"
#include "../main/headers.h"
#include "../scenegraph/Viewer.h"
#include "../opengl/OpenGL_Utils.h"
#include "../opengl/Textures.h"

#include "Collision.h"
#include "LinearAlgebra.h"
#include "Component_Shape.h"
#include "../scenegraph/Tess.h"
#include "../scenegraph/Polyrep.h"


#ifdef _ANDROID
#ifdef ANDROID_DEBUG
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif //ANDROID_DEBUG
#endif //ANDROID


#define XRES 96
#define YRES 96
#define PPI 72
#define POINTSIZE 20


#define TOPTOBOTTOM (fsparam & 0x04)
#define LEFTTORIGHT (fsparam & 0x02)
#define HORIZONTAL (fsparam & 0x01)

#define OUT2GL(a) (p->x_size * (0.0 +a) / ((1.0*(p->font_face[p->myff]->height)) / PPI*XRES))
#define OUT2GLB(a,s) (p->x_size * (0.0 +a) / ((1.0*(p->font_face[p->myff]->height)) / PPI*XRES)*s)

/* now defined in system_fonts.h
include <ft2build.h>
** include <ftoutln.h>
include FT_FREETYPE_H
include FT_GLYPH_H */


typedef struct pComponent_Text{

#if defined(_ANDROID) || defined(IPHONE)
	// Android UI sends in file descriptors and open file for fonts.
	// files are in the assets folder; we assume that the fd is open and fseek'd properly.

	FILE *androidFontFile;
	int fileLen;
#endif //ANDROID

	/* initialize the library with this variable */
	FT_Library library; /* handle to library */

	#define num_fonts 32
	FT_Face font_face[num_fonts];           /* handle to face object */
	int     font_opened[num_fonts];         /* is this font opened   */


	/* we load so many gliphs into an array for processing */
	#define         MAX_GLYPHS      2048
	FT_Glyph        glyphs[MAX_GLYPHS];
	int             cur_glyph;
	int             TextVerbose;// = FALSE;


	/* decompose interface func pointer */
	FT_Outline_Funcs FW_outline_interface;


	/* lets store the font paths here */
	#define fp_name_len 256
	char *font_directory;// = NULL;
	char thisfontname[fp_name_len];

	/* where are we? */
	double pen_x, pen_y;
	double shrink_x, shrink_y;
	/* if this is a status bar, put depth different than 0.0 */
	float TextZdist;

	double x_size;          /* size of chars from file */
	double y_size;          /* size of chars from file */
	int   myff;             /* which index into font_face are we using  */


	/* for keeping track of tesselated points */
	int FW_RIA[500];        /* pointer to which point is returned by tesselator  */
	int FW_RIA_indx;                        /* index into FW_RIA                         */
	struct X3D_PolyRep *FW_rep_;    /* this is the internal rep of the polyrep           */
	int FW_pointctr;                /* how many points used so far? maps into rep-_coord */
	int indx_count;                 /* maps intp FW_rep_->cindex                         */
	int coordmaxsize;               /* maximum coords before needing to REALLOC          */
	int cindexmaxsize;              /* maximum cindexes before needing to REALLOC        */


	/* Outline callbacks and global vars */
	int contour_started;
	FT_Vector last_point;
	int FW_Vertex;

	/* flag to determine if we need to call the open_font call */
	int started;// = FALSE;


}* ppComponent_Text;
void *Component_Text_constructor(){
	void *v = MALLOCV(sizeof(struct pComponent_Text));
	memset(v,0,sizeof(struct pComponent_Text));
	return v;
}
void Component_Text_init(struct tComponent_Text *t){
	//public
	//private
	t->prv = Component_Text_constructor();
	{
		ppComponent_Text p = (ppComponent_Text)t->prv;

		p->TextVerbose = FALSE;
		p->font_directory = NULL;
		/* flag to determine if we need to call the open_font call */
		p->started = FALSE;
	}
}
void Component_Text_clear(struct tComponent_Text *t){
	//public
	//private
	{
		ppComponent_Text p = (ppComponent_Text)t->prv;
		FREE_IF_NZ(p->font_directory);
	}
}
//	ppComponent_Text p = (ppComponent_Text)gglobal()->Component_Text.prv;

void fwl_fontFileLocation(char *fontFileLocation) {
	ppComponent_Text p;
	ttglobal tg = gglobal();
	p = (ppComponent_Text)tg->Component_Text.prv;
	/* Check if dir exists */
	if (fontFileLocation)
	if (do_dir_exists(fontFileLocation)) {
		FREE_IF_NZ(p->font_directory);
		p->font_directory = STRDUP(fontFileLocation);
	}
}

/* function prototypes */
static void FW_NewVertexPoint(double Vertex_x, double Vertex_y);
static int FW_moveto (FT_Vector* to, void* user);
static int FW_lineto(FT_Vector* to, void* user);
static int FW_conicto(FT_Vector* control, FT_Vector* to, void* user);
static int FW_cubicto(FT_Vector* control1, FT_Vector* control2, FT_Vector* to, void* user);
static void FW_make_fontname (int num);
static int FW_init_face(void);
static double FW_extent (int start, int length);
static FT_Error FW_Load_Char(unsigned int idx);
static void FW_draw_outline(FT_OutlineGlyph oglyph);
static void FW_draw_character(FT_Glyph glyph);
static int open_font(void);

#if defined(_ANDROID) || defined(IPHONE)
/* Android UI finds the font file(s) and sends them in here */
void fwg_AndroidFontFile(FILE *myFile,int len) {
	ppComponent_Text p = (ppComponent_Text)gglobal()->Component_Text.prv;
	p->androidFontFile = myFile;
	p->fileLen = len;
}

#endif //ANDROID



void render_Text (struct X3D_Text * node)
{
    COMPILE_POLY_IF_REQUIRED (NULL, NULL, NULL, NULL);
    DISABLE_CULL_FACE;
    render_polyrep(node);
}

void FW_NewVertexPoint (double Vertex_x, double Vertex_y)
{
    GLDOUBLE v2[3];
	ppComponent_Text p;
	ttglobal tg = gglobal();
	p = (ppComponent_Text)tg->Component_Text.prv;

    UNUSED(Vertex_x);
    UNUSED(Vertex_y);

    /* printf ("FW_NewVertexPoint setting coord index %d %d %d\n", */
    /*  p->FW_pointctr, p->FW_pointctr*3+2,p->FW_rep_->actualCoord[p->FW_pointctr*3+2]); */
    p->FW_rep_->actualCoord[p->FW_pointctr*3+0] = (float) OUT2GLB(p->last_point.x + p->pen_x,p->shrink_x);
    p->FW_rep_->actualCoord[p->FW_pointctr*3+1] = (float) (OUT2GLB(p->last_point.y,p->shrink_y) + p->pen_y);
    p->FW_rep_->actualCoord[p->FW_pointctr*3+2] = p->TextZdist;

    /* the following should NEVER happen.... */
    if (p->FW_RIA_indx >500) {
        ConsoleMessage ("Text, relative index too small\n");
        freewrlDie("FW_NewVertexPoint: this should never happen...");
    }

    p->FW_RIA[p->FW_RIA_indx]=p->FW_pointctr;
    v2[0]=p->FW_rep_->actualCoord[p->FW_pointctr*3+0];
    v2[1]=p->FW_rep_->actualCoord[p->FW_pointctr*3+1];
    v2[2]=p->FW_rep_->actualCoord[p->FW_pointctr*3+2];

	/* printf("glu s.b. rev 1.2 or newer, is: %s\n",gluGetString(GLU_VERSION)); */
    FW_GLU_TESS_VERTEX(tg->Tess.global_tessobj,v2,&p->FW_RIA[p->FW_RIA_indx]);

    if (p->TextVerbose) {
        printf ("FW_NewVertexPoint %f %f %f index %d\n",
                p->FW_rep_->actualCoord[p->FW_pointctr*3+0],
                p->FW_rep_->actualCoord[p->FW_pointctr*3+1],
                p->FW_rep_->actualCoord[p->FW_pointctr*3+2],
                p->FW_RIA_indx);
    }
    p->FW_pointctr++;
    p->FW_RIA_indx++;

    if (p->FW_pointctr >= p->coordmaxsize) {
        p->coordmaxsize+=800;
        p->FW_rep_->actualCoord = (float *)REALLOC(p->FW_rep_->actualCoord,
                                                sizeof(*(p->FW_rep_->actualCoord))*p->coordmaxsize*3);
		printf("realloc actualCoord=%p\n",p->FW_rep_->actualCoord);
    }
}
#define GLU_UNKNOWN     100124
int FW_moveto (FT_Vector* to, void* user)
{
	ppComponent_Text p;
	ttglobal tg = gglobal();
	p = (ppComponent_Text)tg->Component_Text.prv;
    UNUSED(user);

    /* Have we started a new line */
    if (p->contour_started) {
       FW_GLU_NEXT_CONTOUR(tg->Tess.global_tessobj,GLU_UNKNOWN);
    }

    /* well if not, tell us that we have started one */
    p->contour_started = TRUE;

    p->last_point.x = to->x; p->last_point.y = to->y;

    if (p->TextVerbose)
        printf ("FW_moveto tox %ld toy %ld\n",to->x, to->y);



    return 0;
}

int FW_lineto (FT_Vector* to, void* user)
{
	ppComponent_Text p = (ppComponent_Text)gglobal()->Component_Text.prv;
    UNUSED(user);


    if ((p->last_point.x == to->x) && (p->last_point.y == to->y)) {
        /* printf ("FW_lineto, early return\n"); */
        return 0;
    }

    p->last_point.x = to->x; p->last_point.y = to->y;
    if (p->TextVerbose) {
        printf ("FW_lineto, going to %ld %ld\n",to->x, to->y);
    }

    FW_NewVertexPoint(OUT2GLB(p->last_point.x+p->pen_x,p->shrink_x), OUT2GLB(p->last_point.y + p->pen_y,p->shrink_y));



    return 0;
}


int FW_conicto (FT_Vector* control, FT_Vector* to, void* user)
{
    FT_Vector ncontrol;
	ppComponent_Text p = (ppComponent_Text)gglobal()->Component_Text.prv;



    /* Bezier curve calcs; fairly rough, but makes ok characters */

    if (p->TextVerbose)
        printf ("FW_conicto\n");

    /* Possible fix here!!! */
    ncontrol.x = (int) ((double) 0.25*p->last_point.x + 0.5*control->x + 0.25*to->x);
    ncontrol.y =(int) ((double) 0.25*p->last_point.y + 0.5*control->y + 0.25*to->y);

    /* printf ("Cubic points (%d %d) (%d %d) (%d %d)\n", p->last_point.x,p->last_point.y, */
    /* ncontrol.x, ncontrol.y, to->x,to->y); */

    FW_lineto (&ncontrol,user);
    FW_lineto (to,user);



    return 0;
}

int FW_cubicto (FT_Vector* control1, FT_Vector* control2, FT_Vector* to, void* user)
{
	ppComponent_Text p = (ppComponent_Text)gglobal()->Component_Text.prv;
    /* really ignore control points */
    if (p->TextVerbose)
        printf ("FW_cubicto\n");

    FW_lineto (control1, user);
    FW_lineto (control2, user);
    FW_lineto (to, user);
    return 0;
}


/* make up the font name */
void FW_make_fontname(int num) {
/*
    bit:    0       BOLD        (boolean)
    bit:    1       ITALIC      (boolean)
    bit:    2       SERIF
    bit:    3       SANS
    bit:    4       TYPEWRITER

    JAS - May 2005 - The Vera freely distributable ttf files
    are:

    Vera.ttf
    VeraMono.ttf
    VeraSeBd.ttf
    VeraSe.ttf
    VeraMoBI.ttf
    VeraMoIt.ttf
    VeraIt.ttf
    VeraMoBd.ttf
    VeraBd.ttf
    VeraBI.ttf

    The files that were included were copyright Bitstream;
    the Vera files are also from Bitstream, but are
    freely distributable. See the copyright file in the
    fonts directory.
*/

    ppComponent_Text p = (ppComponent_Text)gglobal()->Component_Text.prv;
    #ifdef HAVE_FONTCONFIG
    FcPattern *FW_fp=NULL;
    FcChar8 *FW_file=NULL;
    FcResult result;

    // check whether we have a config file
    char* configfile = (char*)FcConfigFilename(0);
    FILE*fi = fopen(configfile, "rb");
    if(fi) {
        fclose(fi);
	//printf("<debug> Initializing FontConfig (configfile=%s)\n", configfile);
    } else {
	//printf("<debug> Initializing FontConfig (no configfile)\n");
    }

    if(!FcInit()) {
        printf("<debug> FontConfig Initialization failed.\n");
    }
    FcConfig * config = FcConfigGetCurrent();
    if(!config) {
        printf("<debug> FontConfig Config Initialization failed.\n");
    }

    FcFontSet *set =  FcConfigGetFonts(config, FcSetSystem);
    //printf("<verbose> FontConfig initialized. Found %d fonts\n", set?set->nfont:0);
    if(!set || !set->nfont) {
        printf("<debug> FontConfig has found zero fonts. This is probably a bad thing.\n");
    }
    #else

    if (!p->font_directory) {
        printf("Internal error: no font directory.\n");
        return;
    }
    strcpy (p->thisfontname, p->font_directory);
    #endif

    switch (num) {
    case 0x04:			/* Serif */
	#ifdef HAVE_FONTCONFIG
	FW_fp=FcPatternBuild(NULL,FC_FAMILY,FcTypeString,"serif",FC_OUTLINE,FcTypeBool,FcTrue,NULL);
	#else
	strcat (p->thisfontname,"/VeraSe.ttf");
	#endif
	break;
    case 0x05: 			/* Serif Bold */
	#ifdef HAVE_FONTCONFIG
	FW_fp=FcPatternBuild(NULL,FC_FAMILY,FcTypeString,"serif",FC_OUTLINE,FcTypeBool,FcTrue,NULL);
	FcPatternAddString(FW_fp,FC_STYLE,(const FcChar8*)"bold");
	#else
	strcat (p->thisfontname,"/VeraSeBd.ttf");
	#endif
	break;
    case 0x06:			/* Serif Ital */
	#ifdef HAVE_FONTCONFIG
	FW_fp=FcPatternBuild(NULL,FC_FAMILY,FcTypeString,"serif",FC_OUTLINE,FcTypeBool,FcTrue,NULL);
	FcPatternAddString(FW_fp,FC_STYLE,(const FcChar8*)"italic");
	FcPatternAddString(FW_fp,FC_STYLE,(const FcChar8*)"oblique");
	#else
	strcat (p->thisfontname,"/VeraSe.ttf");
	#endif
	break;
    case 0x07:			/* Serif Bold Ital */
	#ifdef HAVE_FONTCONFIG
	FW_fp=FcPatternBuild(NULL,FC_FAMILY,FcTypeString,"serif",FC_OUTLINE,FcTypeBool,FcTrue,NULL);
	FcPatternAddString(FW_fp,FC_STYLE,(const FcChar8*)"bold italic");
	FcPatternAddString(FW_fp,FC_STYLE,(const FcChar8*)"bold oblique");
	#else
	strcat (p->thisfontname,"/VeraSeBd.ttf");
	#endif
	break;
    case 0x08:			/* Sans */
	#ifdef HAVE_FONTCONFIG
	FW_fp=FcPatternBuild(NULL,FC_FAMILY,FcTypeString,"sans",FC_OUTLINE,FcTypeBool,FcTrue,NULL);
	#else
	strcat (p->thisfontname,"/Vera.ttf");
	#endif
	break;
    case 0x09: 			/* Sans Bold */
	#ifdef HAVE_FONTCONFIG
	FW_fp=FcPatternBuild(NULL,FC_FAMILY,FcTypeString,"sans",FC_OUTLINE,FcTypeBool,FcTrue,NULL);
	FcPatternAddString(FW_fp,FC_STYLE,(const FcChar8*)"bold");
	#else
	strcat (p->thisfontname,"/VeraBd.ttf");
	#endif
	break;
    case 0x0a: 			/* Sans Ital */
	#ifdef HAVE_FONTCONFIG
	FW_fp=FcPatternBuild(NULL,FC_FAMILY,FcTypeString,"sans",FC_OUTLINE,FcTypeBool,FcTrue,NULL);
	FcPatternAddString(FW_fp,FC_STYLE,(const FcChar8*)"italic");
	FcPatternAddString(FW_fp,FC_STYLE,(const FcChar8*)"oblique");
	#else
	strcat (p->thisfontname,"/VeraIt.ttf");
	#endif
	break;
    case 0x0b: 			/* Sans Bold Ital */
	#ifdef HAVE_FONTCONFIG
	FW_fp=FcPatternBuild(NULL,FC_FAMILY,FcTypeString,"sans",FC_OUTLINE,FcTypeBool,FcTrue,NULL);
	FcPatternAddString(FW_fp,FC_STYLE,(const FcChar8*)"bold italic");
	FcPatternAddString(FW_fp,FC_STYLE,(const FcChar8*)"bold oblique");
	#else
	strcat (p->thisfontname,"/VeraBI.ttf");
	#endif
	break;
    case 0x10:			/* Monospace */
	#ifdef HAVE_FONTCONFIG
	FW_fp=FcPatternBuild(NULL,FC_FAMILY,FcTypeString,"monospace",FC_OUTLINE,FcTypeBool,FcTrue,NULL);
	#else
	strcat (p->thisfontname,"/VeraMono.ttf");
	#endif
	break;
    case 0x11: 			/* Monospace Bold */
	#ifdef HAVE_FONTCONFIG
	FW_fp=FcPatternBuild(NULL,FC_FAMILY,FcTypeString,"monospace",FC_OUTLINE,FcTypeBool,FcTrue,NULL);
	FcPatternAddString(FW_fp,FC_STYLE,(const FcChar8*)"bold");
	#else
	strcat (p->thisfontname,"/VeraMoBd.ttf");
	#endif
	break;
    case 0x12: /* Monospace Ital */
	#ifdef HAVE_FONTCONFIG
	FW_fp=FcPatternBuild(NULL,FC_FAMILY,FcTypeString,"monospace",FC_OUTLINE,FcTypeBool,FcTrue,NULL);
	FcPatternAddString(FW_fp,FC_STYLE,(const FcChar8*)"italic");
	FcPatternAddString(FW_fp,FC_STYLE,(const FcChar8*)"oblique");
	#else
	strcat (p->thisfontname,"/VeraMoIt.ttf");
	#endif
	break;
    case 0x13: /* Monospace Bold Ital */
	#ifdef HAVE_FONTCONFIG
	FW_fp=FcPatternBuild(NULL,FC_FAMILY,FcTypeString,"monospace",FC_OUTLINE,FcTypeBool,FcTrue,NULL);
	FcPatternAddString(FW_fp,FC_STYLE,(const FcChar8*)"bold italic");
	FcPatternAddString(FW_fp,FC_STYLE,(const FcChar8*)"bold oblique");
	#else
	strcat (p->thisfontname,"/VeraMoBI.ttf");
	#endif
	break;
    default:
	printf ("dont know how to handle font id %x\n",num);
	return;
    }

    #ifdef HAVE_FONTCONFIG
    FcConfigSubstitute(0,FW_fp,FcMatchPattern);
    FcDefaultSubstitute(FW_fp);
    set = FcFontSort(0, FW_fp, 1, 0, &result);

    /* This isn't very smart, but we just go with the first match in the set that we can find a valid file for. */
    if(set) {
	// printf("<debug> okay, found a set with %d fonts\n", set?set->nfont:0);
	int t;
        for(t=0;t<set->nfont;t++) {
           FcPattern *match = set->fonts[t];
           if (FcPatternGetString(match,FC_FILE,0,&FW_file) != FcResultMatch) {
              printf("<debug> FontConfig: Couldn't get fontconfig's filename for font id %x\n", num);
              FW_file=0;
           } else {
              // printf("<debug> setting p->thisfontname to %s\n", FW_file);
	      /* strcpy didn't work, use strncpy and set the null character by hand */
              strncpy(p->thisfontname,(char *)FW_file,strlen((char *)FW_file));
              p->thisfontname[strlen((char *)FW_file)] = '\0';
              break;
           }
	}
    } else {
        printf("<debug> no set? wha?\n");
    }
    FcPatternDestroy(FW_fp);
    //FcPatternDestroy(set); bad - corrupts heap, set isn't a Pattern
	if (set) FcFontSetSortDestroy(set);
    #endif
}

/* initialize the freetype library */
static int FW_init_face()
{
    int err;
	ppComponent_Text p = (ppComponent_Text)gglobal()->Component_Text.prv;

#ifdef _ANDROID
        FT_Open_Args myArgs;

    if ((p->fileLen == 0) || (p->androidFontFile ==NULL)) {
	ConsoleMessage ("FW_init_face, fileLen and/or androidFontFile issue");
	return FALSE;

    }

#ifdef ANDROID_DEBUG
   {
	struct stat buf;
   	int fh,result;

ConsoleMessage ("TEXT INITIALIZATION - checking on the font file before doing anything");
   if (0 == fstat(fileno(p->androidFontFile), &buf)) {
      ConsoleMessage("TEXT INITIALIZATION file size is %ld\n", buf.st_size);
      ConsoleMessage("TEXT INITIALIZATION time modified is %s\n", ctime(&buf.st_atime));
   }

    }
#endif //ANDROID_DEBUG


    // ConsoleMessage("FT_Open_Face looks ok to go");

    unsigned char *myFileData = MALLOC(void *, p->fileLen+1);
    size_t frv;
    frv = fread (myFileData, (size_t)p->fileLen, (size_t)1, p->androidFontFile);
    myArgs.flags  = FT_OPEN_MEMORY;
    myArgs.memory_base = myFileData;
    myArgs.memory_size = p->fileLen;

    err = FT_Open_Face(p->library, &myArgs, 0, &p->font_face[p->myff]);
        if (err) {
            char line[2000];
            sprintf  (line,"FreeWRL - FreeType, can not set char size for font %s\n",p->thisfontname);
            ConsoleMessage(line);
            return FALSE;
        } else {
            p->font_opened[p->myff] = TRUE;
        }


#ifdef ANDROID_DEBUG
   {
        struct stat buf;
        int fh,result;

   if (0 == fstat(fileno(p->androidFontFile), &buf)) {
      ConsoleMessage("FIN TEXT INITIALIZATION file size is %ld\n", buf.st_size);
      ConsoleMessage("FIN TEXT INITIALIZATION time modified is %s\n", ctime(&buf.st_atime));
   }
}
#endif //ANDROID_DEBUG

	fclose(p->androidFontFile);
	p->androidFontFile = NULL;


#else //ANDROID
    /* load a font face */
    err = FT_New_Face(p->library, p->thisfontname, 0, &p->font_face[p->myff]);
#endif //ANDROID

    if (err) {
        printf ("FreeType - can not use font %s\n",p->thisfontname);
        return FALSE;
    } else {
        /* access face content */
        err = FT_Set_Char_Size(p->font_face[p->myff], /* handle to face object           */
                               POINTSIZE*64,    /* char width in 1/64th of points  */
                               POINTSIZE*64,    /* char height in 1/64th of points */
                               XRES,            /* horiz device resolution         */
                               YRES);           /* vert device resolution          */

        if (err) {
            printf ("FreeWRL - FreeType, can not set char size for font %s\n",p->thisfontname);
            return FALSE;
        } else {
            p->font_opened[p->myff] = TRUE;
        }
    }
    return TRUE;
}

/* calculate extent of a range of characters */
double FW_extent (int start, int length)
{
    int count;
    double ret = 0;
	ppComponent_Text p = (ppComponent_Text)gglobal()->Component_Text.prv;

    for (count = start; count <length+start; count++) {
        ret += p->glyphs[count]->advance.x >> 10;
    }
    return ret;
}

/* Load a character, a maximum of MAX_GLYPHS are here. Note that no
   line formatting is done here; this is just for pre-calculating
   extents, etc.

   NOTE: we store the handles to each glyph object for each
   character in the glyphs array
*/
FT_Error  FW_Load_Char(unsigned int idx)
{
    FT_Glyph  glyph = NULL;
    FT_UInt glyph_index;
    int error;
	ppComponent_Text p = (ppComponent_Text)gglobal()->Component_Text.prv;

    if (p->cur_glyph >= MAX_GLYPHS) {
        return 1;
    }

    /* retrieve glyph index from character code */
    glyph_index = FT_Get_Char_Index(p->font_face[p->myff],idx);

    /* loads the glyph in the glyph slot */

    error = FT_Load_Glyph(p->font_face[p->myff], glyph_index, FT_LOAD_DEFAULT) ||
        FT_Get_Glyph(p->font_face[p->myff]->glyph, &glyph);

    if (!error) { p->glyphs[p->cur_glyph++] = glyph; }
    return error;
}

static void FW_draw_outline (FT_OutlineGlyph oglyph)
{
    int thisptr = 0;
    int retval = 0;
	ppComponent_Text p;
	ttglobal tg = gglobal();
	p = (ppComponent_Text)tg->Component_Text.prv;

    /* gluTessBeginPolygon(global_tessobj,NULL); */

    FW_GLU_BEGIN_POLYGON(tg->Tess.global_tessobj);
    p->FW_Vertex = 0;

    /* thisptr may possibly be null; I dont think it is use in freetype */
    retval = FT_Outline_Decompose( &oglyph->outline, &p->FW_outline_interface, &thisptr);

    /* gluTessEndPolygon(global_tessobj); */
    FW_GLU_END_POLYGON(tg->Tess.global_tessobj);

    if (retval != FT_Err_Ok)
        printf("FT_Outline_Decompose, error %d\n",retval);
}

/* draw a glyph object */
static void FW_draw_character (FT_Glyph glyph)
{
	ppComponent_Text p = (ppComponent_Text)gglobal()->Component_Text.prv;
    if (glyph->format == ft_glyph_format_outline) {
        FW_draw_outline ((FT_OutlineGlyph) glyph);
        p->pen_x +=  (glyph->advance.x >> 10);
    } else {
        printf ("FW_draw_character; glyphformat  -- need outline for %s %s\n",
                p->font_face[p->myff]->family_name,p->font_face[p->myff]->style_name);
    }
    if (p->TextVerbose) printf ("done character\n");
}

/* UTF-8 to UTF-32 conversion -
// x3d and wrl strings are supposed to be in UTF-8
// when drawing strings, FreeType will take a UTF-32"
// http://en.wikipedia.org/wiki/UTF-32/UCS-4
// conversion method options:
// libicu
//    - not used - looks big
//    - http://site.icu-project.org/   LIBICU  - opensource, C/C++ and java.
// mbstocws
//    - win32 version didn't seem to do any converting
// iconv - gnu libiconv
//    - http://gnuwin32.sourceforge.net/packages/libiconv.htm
//    - the win32 version didn't run - bombs on open
// guru code:
//    - adopted - or at least the simplest parts
//    - not rigourous checking though - should draw bad characters if
//      if you have it wrong in the file
//    - http://floodyberry.wordpress.com/2007/04/14/utf-8-conversion-tricks/
//    - not declared opensource, so we are using the general idea
//      in our own code
*/
const unsigned int Replacement = ( 0xfffd );
const unsigned char UTF8TailLengths[256] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	3,3,3,3,3,3,3,3,4,4,4,4,5,5,0,0
};

unsigned int utf8_to_utf32_char(unsigned char *s, unsigned char *end, unsigned int *inc ) {
	unsigned int tail, i, c;
	c = (*s);
	s++;
	(*inc)++;
	if( c < 0x80 )
		return c; //regular ASCII
	tail = UTF8TailLengths[c];
	if( !tail  || (s + tail > end))
		return Replacement;

	//decoding loop
	c &= ( 0x3f >> tail );
	for(i=0;i<tail;++i) {
		if( (s[i] & 0xc0) != 0x80 )
			break;
		c = (c << 6) + (s[i] & 0x3f);
	}

	//s += i;
	(*inc) += i;
	if( i != tail )
		return Replacement;
	return c;
}

unsigned int *utf8_to_utf32(unsigned char *utf8string, unsigned int *len32)
{
	//does the UTF-8 to UTF-32 conversion
	//it allocates the unsigned int array it returns - please FREE() it
	//it adds an extra on the end and null-terminates
	//len32 is the length, not including the null/0 termination.
	unsigned int *to, *to0;
	unsigned char *start, *end;
	int lenchar, l32;
	lenchar = (int)strlen((const char *)utf8string);
	to0 = to = MALLOC(unsigned int*,(lenchar + 1)*sizeof(unsigned int));
	start = utf8string;
	end = (unsigned char *)&utf8string[lenchar];
	l32 = 0;
	while ( start < end ) {
		while ( ( *start < 0x80 ) && ( start < end ) ) {
			*to++ = *start++;
			l32++;
		}
		if ( start < end ) {
			unsigned int inc = 0;
			*to++ = utf8_to_utf32_char(start,end,&inc);
			start += inc;
			//start++; //done in above function
			l32++;
		}
	}
	to0[l32] = 0;
	*len32 = l32;
	return to0;
}
int utf8_to_utf32_bytes(unsigned char *s, unsigned char *end)
{
	unsigned int tail, c;
	int inc =0;
	c = (*s);
	s++;
	inc++;
	if( c < 0x80 )
		return 1; //regular ASCII 1 byte
	tail = UTF8TailLengths[c];
	tail = s + tail > end ? (unsigned int)end - (unsigned int)s : tail; //min(tail,end-s)
	return tail + 1;
}
int len_utf8(unsigned char *utf8string)
{
	unsigned char *start, *end;
	int lenchar, l32;
	lenchar = (int)strlen((const char *)utf8string);
	start = utf8string;
	end = (unsigned char *)&utf8string[lenchar];
	l32 = 0;
	while ( start < end ) {
		while ( ( *start < 0x80 ) && ( start < end ) ) {
			start++;
			l32++; //ASCII char
		}
		if ( start < end ) {
			start += utf8_to_utf32_bytes(start,end);
			l32++;
		}
	}
	return l32;
}

typedef struct chardata{
	unsigned int iglyph; //glyph index in p->gplyphs[iglyph]
	unsigned int advance; //char width or more precisely, advance of penx to next char start
	double x; //pen_x
	double y;
	double sx; //scale = 1-rshrink * 1-shrink applied appropriately ie the net scale needed for this char in x
	double sy;
} chardata;
typedef struct row32 {
	int len32;
	unsigned int *str32;
	int iglyphstartindex;
	double hrowsize; //all the char widths
	double vcolsize; //len32 x charheight
	unsigned int widestchar; //widest char in the row, in advance units
	chardata *chr;
}row32;

#ifndef DISABLER
#include <malloc.h>
#else
#include <malloc/malloc.h>
#endif

/* take a text string, font spec, etc, and make it into an OpenGL Polyrep.
   Note that the text comes EITHER from a SV (ie, from perl) or from a directstring,
   eg, for placing text on the screen from within FreeWRL itself */

void FW_rendertext(unsigned int numrows,struct Uni_String **ptr, char *directstring,
				unsigned int nl, float *length, double maxext,
				double spacing, double mysize, unsigned int fsparam,
				struct X3D_PolyRep *rp)
{
	unsigned char *str = NULL; /* string pointer- initialization gets around compiler warning */
	unsigned int i,row,ii,irow;
	row32 *rowvec;
	double shrink = 0;
	double rshrink = 0;
	int counter=0;
	int char_count=0;
	int est_tri=0;
	ppComponent_Text p;
	ttglobal tg = gglobal();
	p = (ppComponent_Text)tg->Component_Text.prv;

	p->shrink_x = 1.0;
	p->shrink_y = 1.0;
	/* fsparam has the following bitmaps:

	bit:    0       horizontal  (boolean)
	bit:    1       leftToRight (boolean)
	bit:    2       topToBottom (boolean)
	(style)
	bit:    3       BOLD        (boolean)
	bit:    4       ITALIC      (boolean)
	(family)
	bit:    5       SERIF
	bit:    6       SANS
	bit:    7       TYPEWRITER
	bit:    8       indicates exact font pointer (future use)
	(Justify - major)
	bit:    9       FIRST
	bit:    10      BEGIN
	bit:    11      MIDDLE
	bit:    12      END
	(Justify - minor)
	bit:    13      FIRST
	bit:    14      BEGIN
	bit:    15      MIDDLE
	bit:    16      END

	bit: 17-31      spare
	*/

	/* z distance for text - only the status bar has anything other than 0.0 */
	if (directstring) {
	#ifdef CALCAULATEANGLETAN
		float angletan;
		/* convert fieldofview into radians */
		angletan = fieldofview / 360.0f * PI * 2;

		/* take half of the angle; */
		angletan = angletan / 2.0f;

		/* find the tan of it; */
		angletan = tanf (angletan);

		/* and, divide the "general" text size by it */
		p->TextZdist = -0.010/angletan;
		//printf ("fov %f tzd %f \n",(float) fieldofview, (float) p->TextZdist);
	#else
		/* the equation should be simple, but it did not work. Lets try the following: */
		if (Viewer()->fieldofview < 12.0f) {
			p->TextZdist = -12.0f;
		} else if (Viewer()->fieldofview < 46.0f) {
			p->TextZdist = -0.2f;
		} else if (Viewer()->fieldofview  < 120.0f) {
			p->TextZdist = +2.0f;
		} else {
			p->TextZdist = + 2.88f;
		}
	#endif
	} else {
		p->TextZdist = 0.0f;
	}

	/* have we done any rendering yet */
	/* do we need to call open font? */
	if (!p->started) {
		if (open_font()) {
			p->started = TRUE;
		} else {
			printf ("Could not find System Fonts for Text nodes\n");
			return;
		}
	}

	if (p->TextVerbose)
		printf ("entering FW_Render_text \n");


	p->FW_rep_ = rp;

	p->FW_RIA_indx = 0;            /* index into FW_RIA                                  */
	p->FW_pointctr=0;              /* how many points used so far? maps into rep-_coord  */
	p->indx_count=0;               /* maps intp FW_rep_->cindex                          */
	p->contour_started = FALSE;

	p->pen_x = 0.0; p->pen_y = 0.0;
	p->cur_glyph = 0;
	p->x_size = mysize;            /* global variable for size */
	p->y_size = mysize;            /* global variable for size */

	/* is this font opened */
	p->myff = (fsparam >> 3) & 0x1F;

#if defined (ANDROID)
// Android - for now, all fonts are identical
p->myff = 4;
#endif

	if (p->myff <4) {
		/* we dont yet allow externally specified fonts, so one of
			the font style bits HAS to be set. If there was no FontStyle
			node, this will be blank, so... */
		p->myff = 4;
	}

	if (!p->font_opened[p->myff]) {
		FW_make_fontname(p->myff);
		if (!FW_init_face()) {
			/* tell this to render as fw internal font */
			FW_make_fontname (0);
			FW_init_face();
		}
	}
	if(!p->font_face[p->myff]) return; //couldn't load fonts
	/* type 1 fonts different than truetype fonts */
	if (p->font_face[p->myff]->units_per_EM != 1000)
		p->x_size = p->x_size * p->font_face[p->myff]->units_per_EM/1000.0;

	/* if we have a direct string, then we only have ONE, so initialize it here */
	if (directstring != 0) str = (unsigned char *)directstring;

	rowvec = (row32*)alloca(numrows * sizeof(row32));

	/* load all of the characters first... */
	for (row=0; row<numrows; row++) {
		unsigned int len32, total_row_advance, widest_char, *utf32;
		if (directstring == 0)
			str = (unsigned char *)ptr[row]->strptr;
		/* utf8_to_utf32 */
		utf32 = utf8_to_utf32(str,&len32);
		rowvec[row].iglyphstartindex = p->cur_glyph;
		rowvec[row].len32 = len32;
		rowvec[row].str32 = utf32;
		rowvec[row].chr = (chardata *) alloca(len32*sizeof(chardata));
		total_row_advance = 0;
		widest_char = 0;
		for(i=0;i<len32;i++){
			int icount;
			FW_Load_Char(utf32[i]);
			icount = p->cur_glyph -1;
			rowvec[row].chr[i].iglyph = icount;
			rowvec[row].chr[i].advance = p->glyphs[icount]->advance.x >> 10;
			total_row_advance += rowvec[row].chr[i].advance;
			widest_char = rowvec[row].chr[i].advance > widest_char ? rowvec[row].chr[i].advance : widest_char;
		}
		rowvec[row].hrowsize = total_row_advance;
		rowvec[row].vcolsize = len32 * spacing * p->y_size;
		rowvec[row].widestchar = widest_char;
		char_count += len32;
		//FREE_IF_NZ(utf32); //see bottom of this function
	}

	if (p->TextVerbose) {
		printf ("Text: rows %d char_count %d\n",numrows,char_count);
	}

	/* Jan 2016/dug9 - got all the permutations shown here -both horizontal and vertical- working:
		http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/text.html#t-horizontalTRUE
	*/

	if(HORIZONTAL){
		//find the longest row dimension
		shrink = 1.0;
		if(maxext > 0) {
			double maxlen = 0;
			for(row = 0; row < numrows; row++) {
				double hrowsize = OUT2GLB(rowvec[row].hrowsize,1.0);
				maxlen = hrowsize > maxlen ? hrowsize : maxlen;
			}
			if(maxlen > maxext) {shrink = maxext / maxlen;}
		}
		//shrink = 1.0;
		/* Justify MINOR (verticle), FIRST, BEGIN, MIDDLE and END */
		//bit:    13      FIRST
		//bit:    14      BEGIN
		//bit:    15      MIDDLE
		//bit:    16      END
		//http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/text.html#t-horizontalTRUE

		/* BEGIN */
		if(fsparam & (0x400<<(4))){
			p->pen_y = 0.0;
		}
		/* FIRST */
		if(fsparam & (0x200<<(4))){
			p->pen_y = 1.0; 
		}
		/* MIDDLE */
		if (fsparam & (0x800<<(4))) { 
			p->pen_y = (double)(numrows)/2.0; 
		}
		/* END */
		if (fsparam & (0x1000<<(4))) {
			/* printf ("rowlen is %f\n",rowlen); */
			p->pen_y = (double)numrows;
		}

	
		///* topToBottom */
		if (TOPTOBOTTOM) {
			p->pen_y -= 1.0;
		}else{
			if(fsparam & (0x200<<(4))) //if first, make like begin
				p->pen_y -= 1.0; 		
			p->pen_y = numrows - 1.0 - p->pen_y;
		}

		//screen/vector-agnostic loop to compute penx,y and shrinkage for each glyph
		for(irow = 0; irow < numrows; irow++) {
			unsigned int lenchars;
			double rowlen;

			row = irow;
			if(!TOPTOBOTTOM) row = numrows - irow -1;

			if (directstring == 0) str = (unsigned char *)ptr[row]->strptr;
			if (p->TextVerbose)
				printf ("text2 row %d :%s:\n",row, str);
			p->pen_x = 0.0;
			rshrink = 1.0;
			rowlen = rowvec[row].hrowsize;
			lenchars = rowvec[row].len32;

			if((row < nl) && !(APPROX(length[row],0.0))) {
				rshrink = length[row] / OUT2GLB(rowlen,1.0);
			}
			//if(shrink>0.0001) { FW_GL_SCALE_D(shrink,1.0,1.0); }
			//if(rshrink>0.0001) { FW_GL_SCALE_D(rshrink,1.0,1.0); }

			/* MAJOR Justify, FIRST, BEGIN, */
			if (((fsparam & 0x200) || (fsparam &  0x400)) && !LEFTTORIGHT ) {
				/* printf ("rowlen is %f\n",rowlen); */
				p->pen_x = -rowlen;
			}

			/* MAJOR MIDDLE */
			if (fsparam & 0x800) { p->pen_x = -rowlen/2.0; }

			/* MAJOR END */
			//if ((fsparam & 0x1000) && (fsparam & 0x01)) {
			if ((fsparam & 0x1000) && LEFTTORIGHT ) {
				/* printf ("rowlen is %f\n",rowlen); */
				p->pen_x = -rowlen;
			}

			for(ii=0; ii<lenchars; ii++) {
				/* FT_UInt glyph_index; */
				/* int error; */
				int kk;
				int x;
				i = ii;
				if(!LEFTTORIGHT)
					i = lenchars - ii -1;
				rowvec[row].chr[i].x = p->pen_x;
				rowvec[row].chr[i].y = p->pen_y;
				rowvec[row].chr[i].sx = shrink*rshrink;
				rowvec[row].chr[i].sy = 1.0;
				p->pen_x +=  rowvec[row].chr[i].advance;// * shrink * rshrink; // * directionx
			}
			//counter += lenchars;
			p->pen_y += -spacing * p->y_size;
		}
		//END HORIZONTAL
	}else{
		//IF VERTICAL
		//
		unsigned int widest_column;
		//find the longest row dimension
		double maxlen = 0.0;
		shrink = 1.0;
		for(row = 0; row < numrows; row++) {
			double vcolsize = rowvec[row].vcolsize;
			vcolsize = vcolsize*p->y_size;
			maxlen = vcolsize > maxlen ? vcolsize : maxlen;
		}
		if(maxext > 0) {
			if(maxlen > maxext) shrink = maxext / maxlen;
		}
		widest_column = 0;
		for(row=0;row<numrows;row++)
			widest_column = rowvec[row].widestchar > widest_column ? rowvec[row].widestchar : widest_column;

		/* Justify MINOR (verticle), FIRST, BEGIN, MIDDLE and END */
		//bit:    13      FIRST
		//bit:    14      BEGIN
		//bit:    15      MIDDLE
		//bit:    16      END
		//http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/text.html#t-horizontalTRUE

		/* BEGIN */
		/* FIRST */
		if(fsparam & (0x200<<(4)) || fsparam & (0x400<<(4))){
			//p->pen_x = -1.0 * widest_column; 
			if(LEFTTORIGHT)
				p->pen_x = 0.0;
			else
				p->pen_x = -(double)numrows * widest_column;
		}
		/* MIDDLE */
		if (fsparam & (0x800<<(4))) { 
			p->pen_x = -(double)(numrows)/2.0 *widest_column; 
		}
		/* END */
		if (fsparam & (0x1000<<(4))) {
			/* printf ("rowlen is %f\n",rowlen); */
			if(LEFTTORIGHT)
				p->pen_x = -(double)numrows * widest_column;
			else
				p->pen_x = 0.0;
		}

		if (!LEFTTORIGHT) {
			if(fsparam & (0x200<<(4))) //if first, make like begin
				p->pen_x -= 1.0; 		
		}

		//screen/vector-agnostic loop to compute penx,y and shrinkage for each glyph
		for(irow = 0; irow < numrows; irow++) {
			unsigned int lenchars;
			double rowlen;
			double starty;

			row = irow;
			if(!LEFTTORIGHT) row = numrows - irow -1;

			if (directstring == 0) str = (unsigned char *)ptr[row]->strptr;
			if (p->TextVerbose)
				printf ("text2 row %d :%s:\n",row, str);
			p->pen_y = 0.0;
			rshrink = 1.0;
			rowlen = rowvec[row].vcolsize;
			lenchars = rowvec[row].len32;

			if((row < nl) && !(APPROX(length[row],0.0))) {
				rshrink = length[row] / (rowlen*p->y_size);
			}
			starty = -1.0*shrink*rshrink*p->y_size;
			/* MAJOR Justify, FIRST, BEGIN, */
			if ((fsparam & 0x200) || (fsparam &  0x400)){
				if(TOPTOBOTTOM )
					p->pen_y = starty;
				else
					p->pen_y = rowlen + starty;
			}

			/* MAJOR MIDDLE */
			if (fsparam & 0x800) {
					p->pen_y = rowlen/2.0 + starty; 
			}

			/* MAJOR END */
			if (fsparam & 0x1000  ) {
				if(TOPTOBOTTOM)
					p->pen_y = rowlen + starty;
				else
					p->pen_y = starty;
			}

			for(ii=0; ii<lenchars; ii++) {
				/* FT_UInt glyph_index; */
				/* int error; */
				int kk;
				int x;
				double penx;
				i = ii;
				if(!TOPTOBOTTOM)
					i = lenchars - ii -1;
				penx = p->pen_x;
				if(!LEFTTORIGHT)
					penx = penx + widest_column * spacing - rowvec[row].chr[i].advance;
				rowvec[row].chr[i].x = penx;
				rowvec[row].chr[i].y = p->pen_y;
				rowvec[row].chr[i].sx = 1.0;
				rowvec[row].chr[i].sy = shrink*rshrink;
				p->pen_y += -p->y_size * shrink * rshrink;
			}
			//counter += lenchars;
			p->pen_x +=  widest_column * spacing; // * p->x_size; //rowvec[row].chr[i].advance; // * directionx

		}

	}

	//vector glyph construction
	/* what is the estimated number of triangles? assume a certain number of tris per char */
	est_tri = char_count*800; /* 800 was TESS_MAX_COORDS - REALLOC if needed */
	p->coordmaxsize=est_tri;
	p->cindexmaxsize=est_tri;
	p->FW_rep_->cindex=MALLOC(GLuint *, sizeof(*(p->FW_rep_->cindex))*est_tri);
	p->FW_rep_->actualCoord = MALLOC(float *, sizeof(*(p->FW_rep_->actualCoord))*est_tri*3);
	for(row = 0; row < numrows; row++) {
		unsigned int lenchars = rowvec[row].len32;
		for(i=0; i<lenchars; i++) {
			int kk,x;
			chardata chr;

			chr = rowvec[row].chr[i];
			p->pen_x = chr.x;
			p->pen_y = chr.y;
			p->shrink_x = chr.sx;
			p->shrink_y = chr.sy;
			//p->y_size = xys.sy;
			//p->x_size = xys.sx;
			tg->Tess.global_IFS_Coord_count = 0;
			p->FW_RIA_indx = 0;
			//kk = rowvec[row].iglyphstartindex + i;
			kk = rowvec[row].chr[i].iglyph;
			p->shrink_x = rowvec[row].chr[i].sx;
			p->shrink_y = rowvec[row].chr[i].sy;
			FW_draw_character (p->glyphs[kk]);
			FT_Done_Glyph (p->glyphs[kk]);
			/* copy over the tesselated coords for the character to
				* the rep structure */

			for (x=0; x<tg->Tess.global_IFS_Coord_count; x++) {
					/*printf ("copying %d\n",global_IFS_Coords[x]); */

				/* did the tesselator give us back garbage? */

				if ((tg->Tess.global_IFS_Coords[x] >= p->cindexmaxsize) ||
					(p->indx_count >= p->cindexmaxsize) ||
					(tg->Tess.global_IFS_Coords[x] < 0)) {
						if (p->TextVerbose)
						printf ("Tesselated index %d out of range; skipping indx_count, %d cindexmaxsize %d global_IFS_Coord_count %d\n",
						tg->Tess.global_IFS_Coords[x],p->indx_count,p->cindexmaxsize,tg->Tess.global_IFS_Coord_count);
					/* just use last point - this sometimes happens when */
					/* we have intersecting lines. Lets hope first point is */
					/* not invalid... JAS */
					p->FW_rep_->cindex[p->indx_count] = p->FW_rep_->cindex[p->indx_count-1];
					if (p->indx_count < (p->cindexmaxsize-1)) p->indx_count ++;
				} else {
					/*
					printf("global_ifs_coords is %d indx_count is %d \n",global_IFS_Coords[x],p->indx_count);
					printf("filling up cindex; index %d now points to %d\n",p->indx_count,global_IFS_Coords[x]);
					*/
					p->FW_rep_->cindex[p->indx_count++] = tg->Tess.global_IFS_Coords[x];
				}
			}

			if (p->indx_count > (p->cindexmaxsize-400)) {
				p->cindexmaxsize += 800; /* 800 was TESS_MAX_COORDS; */
				p->FW_rep_->cindex=(GLuint *)REALLOC(p->FW_rep_->cindex,sizeof(*(p->FW_rep_->cindex))*p->cindexmaxsize);
			}
		}
	}
	/* save the triangle count (note, we have a "vertex count", not a "triangle count" */
	p->FW_rep_->ntri=p->indx_count/3;
	/* set these variables so they are not uninitialized */
	p->FW_rep_->ccw=FALSE;

	/* if indx count is zero, DO NOT get rid of MALLOCd memory - creates a bug as pointers cant be null */
	if (p->indx_count !=0) {
		/* REALLOC bug in linux - this causes the pointers to be eventually lost... */
		/* REALLOC (p->FW_rep_->cindex,sizeof(*(p->FW_rep_->cindex))*p->indx_count); */
		/* REALLOC (p->FW_rep_->actualCoord,sizeof(*(p->FW_rep_->actualCoord))*p->FW_pointctr*3); */
	}

	/* now, generate normals */
	p->FW_rep_->normal = MALLOC(float *, sizeof(*(p->FW_rep_->normal))*p->indx_count*3);
	for (i = 0; i<(unsigned int)p->indx_count; i++) {
		p->FW_rep_->normal[i*3+0] = 0.0f;
		p->FW_rep_->normal[i*3+1] = 0.0f;
		p->FW_rep_->normal[i*3+2] = 1.0f;
	}

	/* do we have texture mapping to do? */
	if (HAVETODOTEXTURES) {
		p->FW_rep_->GeneratedTexCoords = MALLOC(float *, sizeof(*(p->FW_rep_->GeneratedTexCoords))*(p->FW_pointctr+1)*3);
		/* an attempt to try to make this look like the NIST example */
		/* I can't find a standard as to how to map textures to text JAS */
		for (i=0; i<(unsigned int)p->FW_pointctr; i++) {
			p->FW_rep_->GeneratedTexCoords[i*3+0] = p->FW_rep_->actualCoord[i*3+0]*1.66f;
			p->FW_rep_->GeneratedTexCoords[i*3+1] = 0.0f;
			p->FW_rep_->GeneratedTexCoords[i*3+2] = p->FW_rep_->actualCoord[i*3+1]*1.66f;
		}
	}

	//free malloced temps (allocas are stack, will self-delete)
	for (row=0; row<numrows; row++) {
		FREE_IF_NZ(rowvec[row].str32);
	}

	if (p->TextVerbose) printf ("exiting FW_Render_text\n");
}

int open_font()
{
    int len;
    int err;
	ppComponent_Text p = (ppComponent_Text)gglobal()->Component_Text.prv;

    if (p->TextVerbose)
        printf ("open_font called\n");

    p->FW_outline_interface.move_to = (FT_Outline_MoveTo_Func)FW_moveto;
    p->FW_outline_interface.line_to = (FT_Outline_LineTo_Func)FW_lineto;
    p->FW_outline_interface.conic_to = (FT_Outline_ConicTo_Func)FW_conicto;
    p->FW_outline_interface.cubic_to = (FT_Outline_CubicTo_Func)FW_cubicto;
    p->FW_outline_interface.shift = 0;
    p->FW_outline_interface.delta = 0;

#ifndef _ANDROID

#ifndef HAVE_FONTCONFIG
    /* where are the fonts stored? */
	if(!p->font_directory)
		p->font_directory = makeFontDirectory();
	//ConsoleMessage("font directory=%s\n",p->font_directory);
    /* were fonts not found? */
    if (p->font_directory == NULL) {
#ifdef AQUA
        ConsoleMessage ("No Fonts; this should not happen on OSX computers; contact FreeWRL team\n");
#else
        ConsoleMessage ("No Fonts; check the build parameter --with-fontsdir, or set FREEWRL_FONTS_DIR environment variable\n");
#endif
        return FALSE;
    }
#endif //HAVE_FONTCONFIG
#endif //ANDROID

    /* lets initialize some things */
    for (len = 0; len < num_fonts; len++) {
        p->font_opened[len] = FALSE;
    }

    if ((err = FT_Init_FreeType(&p->library))) {
        fprintf(stderr, "FreeWRL FreeType Initialize error %d\n",err);
        return FALSE;
    }

    return TRUE;
}

int avatarCollisionVolumeIntersectMBBf(double *modelMatrix, float *minVals, float *maxVals);

void collide_Text (struct X3D_Text *node)
{
	struct sNaviInfo *naviinfo;
	GLDOUBLE awidth,atop,abottom,astep,modelMatrix[16];
    struct point_XYZ delta = {0,0,-1};
    struct X3D_PolyRep pr;
    int change = 0;

	ttglobal tg = gglobal();
	naviinfo = (struct sNaviInfo*)tg->Bindable.naviinfo;

    awidth = naviinfo->width; /*avatar width*/
    atop = naviinfo->width; /*top of avatar (relative to eyepoint)*/
    abottom = -naviinfo->height; /*bottom of avatar (relative to eyepoint)*/
    astep = -naviinfo->height+naviinfo->step;


    /*JAS - normals are always this way - helps because some
      normal calculations failed because of very small triangles
      which made collision calcs fail, which moved the Viewpoint...
      so, if there is no need to calculate normals..., why do it? */

    /* JAS - first pass, intern is probably zero */
    if (node->_intern == NULL) return;

    /* JAS - no triangles in this text structure */
    if (node->_intern->ntri == 0) return;

    /*save changed state.*/
    if (node->_intern)
        change = node->_intern->irep_change;

    COMPILE_POLY_IF_REQUIRED(NULL, NULL, NULL, NULL);

    if (node->_intern)
        node->_intern->irep_change = change;

    /* restore changes state, invalidates compile_polyrep work done, so it can be done
       correclty in the RENDER pass */

    pr = *(node->_intern);

    /* do the triangle test again, now that we may have compiled the node. */
    if (pr.ntri == 0) {
        /* printf ("TRIANGLE NOW HAS ZERO NODES...\n"); */
        return;
    }

    FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelMatrix);

	matmultiplyAFFINE(modelMatrix,modelMatrix,FallInfo()->avatar2collision);
	//dug9july2011 matmultiply(modelMatrix,FallInfo()->avatar2collision,modelMatrix);

	if(!avatarCollisionVolumeIntersectMBBf(modelMatrix,pr.minVals,pr.maxVals) )return;
    delta = planar_polyrep_disp(abottom,atop,astep,awidth,pr,modelMatrix,PR_DOUBLESIDED,delta);
    /* delta used as zero */

    vecscale(&delta,&delta,-1);

    accumulate_disp(CollisionInfo(),delta);

#ifdef COLLISIONVERBOSE
    if((fabs(delta.x) != 0. || fabs(delta.y) != 0. || fabs(delta.z) != 0.)) {
        fprintf(stderr,"COLLISION_TXT: (%f %f %f) (%f %f %f)\n",
                t_orig.x, t_orig.y, t_orig.z,
                delta.x, delta.y, delta.z);
    }
#endif
}

void make_Text (struct X3D_Text *node)
{
    struct X3D_PolyRep *rep_ = node->_intern;
    double spacing = 1.0;
    double size = 1.0;
	int isScreenFontStyle;
    unsigned int fsparams = 0;

	isScreenFontStyle = FALSE;
    /* We need both sides */
    DISABLE_CULL_FACE;

    if (node->fontStyle) {
        /* We have a FontStyle. Parse params (except size and spacing) and
           make up an unsigned int with bits indicating params, to be
           passed to the Text Renderer

           bit:    0       horizontal  (boolean)
           bit:    1       leftToRight (boolean)
           bit:    2       topToBottom (boolean)
           (style)
           bit:    3       BOLD        (boolean)
           bit:    4       ITALIC      (boolean)
           (family)
           bit:    5       SERIF
           bit:    6       SANS
           bit:    7       TYPEWRITER
           bit:    8       indicates exact font pointer (future use)
           (Justify - major)
           bit:    9       FIRST
           bit:    10      BEGIN
           bit:    11      MIDDLE
           bit:    12      END
           (Justify - minor)
           bit:    13      FIRST
           bit:    14      BEGIN
           bit:    15      MIDDLE
           bit:    16      END

           bit: 17-31      spare
        */

        struct X3D_FontStyle *fsp;
        unsigned char *lang;
        unsigned char *style;
        struct Multi_String family;
        struct Multi_String justify;
        int tmp; int tx;
        struct Uni_String **svptr;
        unsigned char *stmp;

        /* step 0 - is the FontStyle a proto? */
        POSSIBLE_PROTO_EXPANSION(struct X3D_FontStyle *, node->fontStyle,fsp);

        /* fsp = (struct X3D_FontStyle *)node->fontStyle; */
        if (fsp->_nodeType != NODE_FontStyle && fsp->_nodeType != NODE_ScreenFontStyle) {
            ConsoleMessage ("Text node has FontStyle of %s\n",stringNodeType(fsp->_nodeType));
            node->fontStyle = NULL; /* stop dumping these messages */
        }

        /* step 0.5 - now that we know FontStyle points ok, go for
         * the other pointers */
        lang = (unsigned char *)fsp->language->strptr;
        style = (unsigned char *)fsp->style->strptr;

        family = fsp->family;
        justify = fsp->justify;

        /* Step 1 - record the spacing and size, for direct use */
        spacing = fsp->spacing;
        size = fsp->size;
		if(fsp->_nodeType == NODE_ScreenFontStyle){
			static float pixels_per_point = 4.0f/3.0f; //about 16 pixels for 12 point font, assume we are in ScreenGroup?
			size = fsp->size * pixels_per_point;
			isScreenFontStyle = TRUE;
		}

        /* Step 2 - do the SFBools */
        fsparams = (fsp->horizontal)|(fsp->leftToRight<<1)|(fsp->topToBottom<<2);

        /* Step 3 - the SFStrings - style and language */
        /* actually, language is not parsed yet */

        if (strlen((const char *)style)) {
            if (!strcmp((const char *)style,"ITALIC")) {fsparams |= 0x10;}
            else if(!strcmp((const char *)style,"BOLD")) {fsparams |= 0x08;}
            else if (!strcmp((const char *)style,"BOLDITALIC")) {fsparams |= 0x18;}
            else if (strcmp((const char *)style,"PLAIN")) {
                printf ("Warning - FontStyle style %s  assuming PLAIN\n",style);}
        }
        if (strlen((const char *)lang)) {
            printf ("Warning - FontStyle - language param unparsed\n");
        }


        /* Step 4 - the MFStrings now. Family, Justify. */
        /* family can be blank, or one of the pre-defined ones. Any number of elements */

        svptr = family.p;
        for (tmp = 0; tmp < family.n; tmp++) {
            stmp = (unsigned char *)svptr[tmp]->strptr;
            if (strlen((const char *)stmp) == 0) {fsparams |=0x20; }
            else if (!strcmp((const char *)stmp,"SERIF")) { fsparams |= 0x20;}
            else if(!strcmp((const char *)stmp,"SANS")) { fsparams |= 0x40;}
            else if (!strcmp((const char *)stmp,"TYPEWRITER")) { fsparams |= 0x80;}
            /* else { printf ("Warning - FontStyle family %s unknown\n",stmp);}*/
        }

        svptr = justify.p;
        tx = justify.n;
        /* default is "BEGIN" "FIRST" */
        if (tx == 0) { fsparams |= 0x2400; }
        else if (tx == 1) { fsparams |= 0x2000; }
        else if (tx > 2) {
            printf ("Warning - FontStyle, max 2 elements in Justify\n");
            tx = 2;
        }

        for (tmp = 0; tmp < tx; tmp++) {
            stmp = (unsigned char *)svptr[tmp]->strptr;
            if (strlen((const char *)stmp) == 0) {
                if (tmp == 0) {
                    fsparams |= 0x400;
                } else {
                    fsparams |= 0x2000;
                }
            }
            else if (!strcmp((const char *)stmp,"FIRST")) { fsparams |= (0x200<<(tmp*4));}
            else if(!strcmp((const char *)stmp,"BEGIN")) { fsparams |= (0x400<<(tmp*4));}
            else if (!strcmp((const char *)stmp,"MIDDLE")) { fsparams |= (0x800<<(tmp*4));}
            else if (!strcmp((const char *)stmp,"END")) { fsparams |= (0x1000<<(tmp*4));}
            /* else { printf ("Warning - FontStyle family %s unknown\n",stmp);}*/
        }
    } else {
        /* send in defaults */
        fsparams = 0x2427;
    }

    /*  do the Text parameters, guess at the number of triangles required*/
    rep_->ntri = 0;

    /*
       printf ("Text, calling FW_rendertext\n");
       call render text - NULL means get the text from the string
    */
	//normal scene 3D vectorized text
	node->__rendersub = 0;
	FW_rendertext(((node->string).n),((node->string).p),NULL,
				((node->length).n),((node->length).p),
					(node->maxExtent),spacing,size,fsparams,rep_);


}







//==========================SCREENFONT===============================================
/* thanks go to dug9 for contributing atlasfont code to freewrl from his dug9gui project 
	
	Notes on freewrl use of atlas/screen fonts, new Jan 2016:
	CaptionText -a kind of direct-to-screen text - see mainloop.c contenttype_captiontext-
	and Text + Component_Layout > ScreenFontStyle 
	both need speedy rendering of what could be rapidly changing text strings, such 
	as Time or FPS that might change on every frame. For that we don't want to recompile
	a new polyrep on each frame. Just run down the string printing characters as textured rectangles.
	And we want to share fonts and anyone can load a font Text -vector or screen- or CaptionText, 
	and once loaded the others recognize and don't need to reload.
	The texture used -called a font atlas- holds all the ascii chars by default -with a fast lookup-, 
	plus it can add as-needed extended utf8 characters with a slower lookup method. 

	AtlasFont - the facename, font path, one per fontface
	Atlas - the bitmap, used as texture, one per AtlasFont
	AtlasEntrySet - the lookup table for ascii and extended characters, 
		one per fontsize for a given AtlasFont
*/

/*	UTF8 String literal support
	 if you want to put extended (non ASCII, > 127) chars in CaptionText:
		a) keep/make the string literals utf8 somehow, and 
		b) we call a utf8 to utf32 function below
	a) How to keep/make string literals utf8:
	DO NOT (non-portable):
	X use u8"" or utf8"" string literals, which gcc supports I think, MS does not (by default uses locale codepage which crashes freetype lib), so not portable
	X use wchar_t and L"", which both msvc and gcc support, but gcc is 32bit unicode and MS is 16bit unicode,
		 so not quite portable, although you could convert either to utf8 from literals early,
		 using platform specific code
	DO (portable):
	a) embed escape sequences. Capital Omega is hex CE A9 "\xCE\xA9" or octal 316 251 "\316\251"   http://calc.50x.eu/
	b) convert from codepage to utf8 externally, and paste sequence into string:  codepage windows-1250 � = utf8 "è"  � = "é" http://www.motobit.com/util/charset-codepage-conversion.asp
	   or use linux iconv
	c) read strings from a utf8 encoded file (utf16 and utf32 files requires BOM byte order mark 
		to determine endieness of file, utf8 does not need this mark, except your reading software needs to know
		whether to convert from UTF8 or trust it's ASCII, or convert from a specific codepage. Determining heuristically
		is difficult. So just put an ascii string UTF8 or ASCII or CODEPAGE-12500 on the first line to tell your own code)
*/

static int iyup = 0;  //iyup = 1 means y is up on texture (like freewrl) (doesn't work right), iyup=0 means y-down texture coords (works)

typedef struct AtlasFont AtlasFont;
typedef struct Atlas Atlas;
typedef struct AtlasEntry AtlasEntry;
typedef struct GUIElement GUIElement;
void *GUImalloc(struct Vector **guitable, int type);
typedef struct ivec2 {int X; int Y;} ivec2;
ivec2 ivec2_init(int x, int y);
typedef enum GUIElementType 
{ 
	GUI_FONT = 9,
	GUI_ATLAS = 10,
	GUI_ATLASENTRY = 11,
	GUI_ATLASENTRYSET = 12,
} GUIElementType;

//STATICS
static struct Vector *font_table; //AtlasFontSize*
static struct Vector *entry_table; //AtlasEntry* - just for GC
static struct Vector *entryset_table; //AtlasEntrySet*
static struct Vector *atlas_table; //Atlas *



//atlas entry has the box for one glyph, or one widget icon
typedef struct AtlasEntry {
	char *name;
	int type;
	ivec2 apos; //position in atlas texture, pixels from UL of texture image
	ivec2 size; //size in atlas texture, pixels
	int ichar;  //int pseudoname instead of char * name, used for unicode char
	ivec2 pos;  //shift/offset from target placement ie glyph image shift from lower left corner of character
	ivec2 advance; //used for glyphs, advance to the next char which may be different -wider- than pixel row width
} AtlasEntry;
void AtlasEntry_init1(AtlasEntry *me,  char *name, int index, int x, int y, int width, int height){
	//use this for .bmp atlases that are already tiled
	me->name = name;
	me->type = GUI_ATLASENTRY;
	me->apos.X = x;
	me->apos.Y = y;
	me->size.X = width;
	me->size.Y = height;
	me->ichar = index;
}



//atlas is an image buffer. It doesn't care what's stored in the image, although it
//does help when adding things to the atlas, by storing the last location as penx,y
//The reason for using an atlas versus individual little images: fewer texture changes sent to the GPU
// which dramatically speeds rendering of the gui.
// For example drawing a textpanel full of text glyph by glyph slows rendering to 8 FPS on intel i5, 
// and using an atlas it's 60FPS - hardly notice the gui rendering.
// The reason we don't do all font as atlasses by default: some font ie textCaption could be dynamically
//  resizable by design, and if it's just a few chars, its more efficent to do glyph by glyph than
//  render several atlases. But in theory if its just a few chars, you could render just those chars
//  to an atlas at different sizes.
typedef struct Atlas {
	char *name;
	int type;
	char *texture;  //the GLubyte* buffer
	//int textureID; //gl texture buffer
	int bytesperpixel;  //1 for alpha, 2 lumalpha 4 rgba. font should be 1 alpha
	//FT_Face fontFace;
	ivec2 size;  //pixels, of texture: X=width, Y=height
	int rowheight;  //if items are in regular rows, this is a hint, during making of the atlas
	ivec2 pen;  //have a cursor, so it's easy to position an additional entry in unoccupied place in atlas
} Atlas;
void Atlas_init(Atlas *me, int size, int rowheight){
	//use this for generating font atlas from .ttf
	me->pen.X = me->pen.Y = 0;
	me->size.X = me->size.Y = size;
	me->rowheight = rowheight; //spacing between baselines for textpanel, in pixels (can get this from fontFace once loaded and sized to EM)
	//me->EMpixels = EMpixels;  //desired size of "EM" square (either x or y dimension, same) in pixels
	me->bytesperpixel = 1; //TT fonts are rendered to an antialiased 8-bit alpha texture
	me->texture = (char*)malloc(me->size.X *me->size.Y*me->bytesperpixel);
	memset(me->texture,127,me->size.X *me->size.Y*me->bytesperpixel); //make it black by default
	/*
	//here are some fun stripes to initialize the texture data, for debugging:
	int kk;
	for(int i=0;i<size;i++){
		for(int j=0;j<size;j++){
			kk = (i*size + j)*me->bytesperpixel;
			me->texture[kk+me->bytesperpixel-1] = i % 2 == 0 ? 255 : 0;
		}
	}
	*/
}

void subimage_paste(char *image, ivec2 size, char* subimage, int bpp, ivec2 ulpos, ivec2 subsize ){
	int i;
	int imrow, imcol, impos;
	int isrow, iscol, ispos;
	for(i=0;i<subsize.Y;i++ ){
		imrow = ulpos.Y + i;
		imcol = ulpos.X;
		impos = (imrow * size.X + imcol)*bpp;
		isrow = i;
		iscol = 0;
		ispos = (i*subsize.X + iscol)*bpp;
		if(impos >= 0 && (impos+subsize.X*bpp <= size.X*size.Y*bpp))
			memcpy(&image[impos],&subimage[ispos],subsize.X*bpp);
		else
			printf("!");
	}
}

// named type is for upcasting any GUI* to a simple name
// so a generic table search can be done by name for any type
typedef struct GUINamedType {
	char *name;
	int type;
} GUINamedType;
GUINamedType *searchGUItable(struct Vector* guitable, char *name){
	int i;
	GUINamedType *retval = NULL;
	if(guitable)
	for(i=0;i<vectorSize(guitable);i++){
		GUINamedType *el = vector_get(GUINamedType*,guitable,i);
		///printf("[SGT %s %s] ",name,el->name);
		if(!strcmp(name,el->name)){
			retval = el;
			break;
		}
	}
	return retval;
}


void Atlas_addEntry(Atlas *me, AtlasEntry *entry, char *gray){
	ivec2 pos;
	//use this with atlasEntry_init for .ttf font atlas
	//paste in somewhere, and update cRow,cCol by width,height of gray
	//layout in rows
	//if((me->pen.X + me->rowheight) > me->size.X){
	//	me->pen.Y += me->rowheight;
	//	me->pen.X = 0;
	//}
	if((me->pen.X + entry->size.X) > me->size.X){
		me->pen.Y += me->rowheight;
		me->pen.X = 0;
	}

	//paste glyph image into atlas image
	pos.X = me->pen.X + entry->pos.X;
	pos.Y = me->pen.Y + entry->pos.Y;
	
	if(1) subimage_paste(me->texture,me->size,gray,1,me->pen,entry->size);
	if(0) subimage_paste(me->texture,me->size,gray,1,pos,entry->size);

	if(1) {
		entry->apos.X = me->pen.X;
		entry->apos.Y = me->pen.Y;
	}else{
		entry->apos.X = pos.X;
		entry->apos.Y = pos.Y;
	}
	me->pen.X += entry->size.X; //entry->advance.X;
	//me->pen.Y += entry->advance.Y;
}


//a fontsize with the alphabet, or a whole set of named widgets, comprise an AtlasEntrySet
// in theory more than one AtlasEntrySet could use the same atlas, allowing fewer textures, and 
// better texture surface utilization %
// You need to do a separate 'Set for each fontsize, because there's only one ascii lookup table per Set
typedef struct AtlasEntrySet {
	char *name;
	int type;
	int EMpixels;
	int maxadvancepx; //max_advance for x for a fontface
	int rowheight;  //if items are in regular rows, this is a hint, during making of the atlas
	char *atlasName;
	Atlas *atlas;
	AtlasEntry *ascii[128]; //fast lookup table, especially for ascii 32 - 126 to get entry *. NULL if no entry.
	struct Vector *entries; //atlasEntry *  -all entries -including ascii as first 128- sorted for binary searching
} AtlasEntrySet;
void AtlasEntrySet_init(AtlasEntrySet *me, char *name){
	me->name = name;
	me->type = GUI_ATLASENTRYSET;
	me->entries = newVector(AtlasEntry *,256);
	memset(me->ascii,0,128*sizeof(int)); //initialize ascii fast lookup table to NULL, which means no char glyph stored
}

void AtlasEntrySet_addEntry1(AtlasEntrySet *me, AtlasEntry *entry){
	//use this with atlasEntry_init1 for .bmp widget texture atlas
	vector_pushBack(AtlasEntry*,me->entries,entry);
	if(entry->ichar > 0 && entry->ichar < 128){
		//if its an ascii char, add to fast lookup table
		me->ascii[entry->ichar] = entry;
	}
}
void AtlasEntrySet_addEntry(AtlasEntrySet *me, AtlasEntry *entry, char *gray){
	
	vector_pushBack(AtlasEntry*,me->entries,entry);
	if(entry->ichar > 0 && entry->ichar < 128){
		//if its an ascii char, add to fast lookup table
		me->ascii[entry->ichar] = entry;
	}
	if(!me->atlas)
		me->atlas = (Atlas*)searchGUItable(atlas_table,me->atlasName);
	if(me->atlas)
		Atlas_addEntry(me->atlas, entry, gray);
}
AtlasEntry *AtlasEntrySet_getEntry1(AtlasEntrySet *me, char *name){
	//use this to get an atlas entry by char* name, slow
	int i;
	for(i=0;i<vectorSize(me->entries);i++){
		AtlasEntry *entry = vector_get(AtlasEntry*,me->entries,i);
		if(!strcmp(entry->name,name))
			return entry;
	}
	return NULL;
}
AtlasEntry *AtlasEntrySet_getEntry(AtlasEntrySet *me, int ichar){
	//use this to get an atlas entry for a font glyph
	// uses fast lookup for ASCII chars first, then slow lookup since its a 16 char x 16 char atlas, max 256 chars stored
	// ichar is unicode
	AtlasEntry *ae = NULL;
	if(ichar > 0 && ichar < 128){  // < 0x80
		ae = me->ascii[ichar];
	}else{
		//could be a binary search here
		int i;
		for(i=128;i<vectorSize(me->entries);i++){
			AtlasEntry *entry = vector_get(AtlasEntry*,me->entries,i);
			if(entry->ichar == ichar){
				ae = entry;
				break;
			}
		}
	}
	return ae;
}


//GUIFont instances go in a public lookup table, so a fontface is loaded only once
//and if an atlas has been generated for that fontface by the programmer, it's added
//to the font
typedef struct AtlasFont {
	char *name;
	int type;
	char *path;
	FT_Face fontFace;
	struct Vector atlasSizes; //GUIAtlasEntrySet*
} AtlasFont;
void AtlasFont_init(AtlasFont *me,char *facename, char* path){

	me->name = facename;
	me->type = GUI_FONT;
	me->path = path;
	me->fontFace = NULL;
	me->atlasSizes.n = 0; //no atlas renderings to begin with
	me->atlasSizes.allocn = 2;
	me->atlasSizes.data = malloc(2*sizeof(AtlasEntrySet*));
}


AtlasEntrySet* searchAtlasFontForSizeOrMake(AtlasFont *font,int EMpixels){
	AtlasEntrySet *set = NULL;
	if(font){
		if(font->atlasSizes.n){
			for(int i=0;i<font->atlasSizes.n;i++){
				AtlasEntrySet *aes = vector_get(AtlasEntrySet*,&font->atlasSizes,i);
				if(aes){
					if(aes->EMpixels == EMpixels){
						set = aes;
						break;
					}
				}
			}
		}
		if(!set){
			//make set
		}
	}
	return set;
}



char *newstringfromchar(char c){
	char *ret = malloc(2);
	ret[0] = c;
	ret[1] = '\0';
	return ret;
}
static FT_Library fontlibrary; /* handle to library */

int RenderFontAtlasCombo(AtlasFont *font, AtlasEntrySet *entryset,  char * cText){
	//pass in a string with your alphabet, numbers, symbols or whatever, 
	// and we use freetype2 to render to bitmap, and then tile those little
	// bitmaps into an atlas texture
	//wText is UTF-8 since FreeType expect this	 
	char *fontname;
	int EMpixels, i;
	Atlas *atlas;
	FT_Face fontFace;
	int err;	

	fontname = font->path;
	EMpixels = entryset->EMpixels;
	atlas = entryset->atlas;


	err = FT_Init_FreeType(&fontlibrary);
    if (err) {
        fprintf(stderr, "FreeWRL FreeType Initialize error %d\n",err);
        return FALSE;
    }
    err = FT_New_Face(fontlibrary, fontname, 0, &fontFace);
    if (err) {
        printf ("FreeType - can not use font %s\n",fontname);
        return FALSE;
    } 
	if(1){
		int nsizes;
		printf("fontface flags & Scalable? = %d \n",fontFace->face_flags & FT_FACE_FLAG_SCALABLE );
		nsizes = fontFace->num_fixed_sizes;
		printf("num_fixed_sizes = %d\n",nsizes);
	}
	#define POINTSIZE 20
	#define XRES 96
	#define YRES 96
	if(0)
    err = FT_Set_Char_Size(fontFace, /* handle to face object           */
                            POINTSIZE*64,    /* char width in 1/64th of points  */
                            POINTSIZE*64,    /* char height in 1/64th of points */
                            XRES,            /* horiz device resolution         */
                            YRES);           /* vert device resolution          */
	if(1)
	err = FT_Set_Pixel_Sizes(
		fontFace,   /* handle to face object */
		0,      /* pixel_width           */
		EMpixels );   /* pixel_height          */
    
	if(1){
		int h;
		printf("spacing between rows = %f\n",fontFace->height);
		h = fontFace->size->metrics.height;
		printf("height(px)= %d.%d x_ppem=%d\n",h >>6,(h<<26)>>26,(unsigned int)fontFace->size->metrics.x_ppem);
		atlas->rowheight = fontFace->size->metrics.height >> 6;

	}
    if (err) {
        printf ("FreeWRL - FreeType, can not set char size for font %s\n",fontname);
        return FALSE;
	}
	for (i = 0; i < strlen(cText); i++) 	
	{ 		
		FT_GlyphSlot glyph;
		FT_Error error;
		unsigned long c;
		AtlasEntry *entry;
		
		c = FT_Get_Char_Index(fontFace, (int) cText[i]); 		
		error = FT_Load_Glyph(fontFace, c, FT_LOAD_RENDER); 	
		if(error) 		
		{ 			
			//Logger::LogWarning("Character %c not found.", wText.GetCharAt(i)); 
			printf("ouch87");
			continue; 		
		}
		glyph = fontFace->glyph;

		entry = malloc(sizeof(AtlasEntry));
		//atlasEntry_init1(entry,names[i*2],(int)cText[i],0,0,16,16);
		entry->ichar = 0;
		if( cText[i] > 31 && cText[i] < 128 ) entry->ichar = cText[i]; //add to fast lookup table if ascii
		entry->pos.X = glyph->bitmap_left;
		entry->pos.Y = glyph->bitmap_top;
		entry->advance.X = glyph->advance.x >> 6;
		entry->advance.Y = glyph->advance.y >> 6;
		entry->size.X = glyph->bitmap.width;
		entry->size.Y = glyph->bitmap.rows;
		entry->name = newstringfromchar(cText[i]);
		AtlasEntrySet_addEntry(entryset,entry,glyph->bitmap.buffer);
	}
	//for(int i=0;i<256;i++){
	//	for(int j=0;j<256;j++)
	//		atlas->texture[i*256 + j] = (i*j) %2 ? 0 : 127; //checkerboard, to see if fonts twinkle
	//}
	return TRUE;
}
int RenderFontAtlas(AtlasFont *font, AtlasEntrySet *entryset,  char * cText){
	//pass in a string with your alphabet, numbers, symbols or whatever, 
	// and we use freetype2 to render to bitmap, and then tile those little
	// bitmaps into an atlas texture
	//wText is UTF-8 since FreeType expect this	
	int i;
	FT_Face fontFace = font->fontFace;

	for (i = 0; i < strlen(cText); i++) 	
	{ 		
		FT_GlyphSlot glyph;
		FT_Error error;
		AtlasEntry *entry;
		unsigned long c;
		
		c = FT_Get_Char_Index(fontFace, (int) cText[i]); 		
		error = FT_Load_Glyph(fontFace, c, FT_LOAD_RENDER); 	
		if(error) 		
		{ 			
			//Logger::LogWarning("Character %c not found.", wText.GetCharAt(i)); 
			printf("ouch87");
			continue; 		
		}
		glyph = fontFace->glyph;

		entry = malloc(sizeof(AtlasEntry));
		//atlasEntry_init1(entry,names[i*2],(int)cText[i],0,0,16,16);
		entry->ichar = 0;
		if( cText[i] > 31 && cText[i] < 128 ) entry->ichar = cText[i]; //add to fast lookup table if ascii
		entry->pos.X = glyph->bitmap_left;
		entry->pos.Y = glyph->bitmap_top;
		entry->advance.X = glyph->advance.x >> 6;
		entry->advance.Y = glyph->advance.y >> 6;
		entry->size.X = glyph->bitmap.width;
		entry->size.Y = glyph->bitmap.rows;
		entry->name = newstringfromchar(cText[i]);
		AtlasEntrySet_addEntry(entryset,entry,glyph->bitmap.buffer);
	}
	//for(int i=0;i<256;i++){
	//	for(int j=0;j<256;j++)
	//		atlas->texture[i*256 + j] = (i*j) %2 ? 0 : 127; //checkerboard, to see if fonts twinkle
	//}
	return TRUE;
}

int AtlasFont_LoadFont(AtlasFont *font){
	FT_Face fontFace;
	int err;	
	char thisfontname[2048];
	ttglobal tg;
	ppComponent_Text p;
	tg = gglobal();
	p = (ppComponent_Text)tg->Component_Text.prv;

	if(!p->font_directory)
		p->font_directory = makeFontDirectory();

    strcpy (thisfontname, p->font_directory);
	strcat(thisfontname,"/");
	strcat(thisfontname,font->path);

	if(!fontlibrary)
    if ((err = FT_Init_FreeType(&fontlibrary))) {
        fprintf(stderr, "FreeWRL FreeType Initialize error %d\n",err);
        return FALSE;
    }
    err = FT_New_Face(fontlibrary, thisfontname, 0, &fontFace);
    if (err) {
        printf ("FreeType - can not use font %s\n",thisfontname);
        return FALSE;
    } 
	font->fontFace = fontFace;

	if(1){
		int nsizes;
		printf("fontface flags & Scalable? = %d \n",fontFace->face_flags & FT_FACE_FLAG_SCALABLE );
		nsizes = fontFace->num_fixed_sizes;
		printf("num_fixed_sizes = %d\n",nsizes);
	}
	return TRUE;
}
int AtlasFont_setFontSize(AtlasFont *me, int EMpixels, int *rowheight, int *maxadvancepx){
	int err;
	FT_Face fontFace = me->fontFace;

	if(!fontFace) return FALSE;
	#define POINTSIZE 20
	#define XRES 96
	#define YRES 96
	if(0){
		err = FT_Set_Char_Size(fontFace, /* handle to face object           */
								POINTSIZE*64,    /* char width in 1/64th of points  */
								POINTSIZE*64,    /* char height in 1/64th of points */
								XRES,            /* horiz device resolution         */
								YRES);           /* vert device resolution          */
	}
	err = FT_Set_Pixel_Sizes(
		fontFace,   /* handle to face object */
		0,      /* pixel_width           */
		EMpixels );   /* pixel_height          */
    
	if(1){
		int h, max_advance_px;
		printf("spacing between rows = %f\n",fontFace->height);
		h = fontFace->size->metrics.height;
		printf("height(px)= %d.%d x_ppem=%d\n",h >>6,(h<<26)>>26,(unsigned int)fontFace->size->metrics.x_ppem);
		*rowheight = fontFace->size->metrics.height >> 6;
		//fs->EMpixels = (unsigned int)fontFace->size->metrics.x_ppem;
		max_advance_px = fontFace->size->metrics.max_advance >> 6;
		printf("max advance px=%d\n",max_advance_px);
	}
	*maxadvancepx = fontFace->size->metrics.max_advance >> 6;
    if (err) {
        printf ("FreeWRL - FreeType, can not set char size for font %s\n",me->path);
        return FALSE;
	}
	return TRUE;
}
unsigned int upperPowerOfTwo(unsigned int k){
	int ipow;
	unsigned int kk = 1;
	for(ipow=2;ipow<32;ipow++){
		kk = kk << 1;
		if(kk > k) return kk;
	}
	return 1 << 31;
}
void AtlasFont_RenderFontAtlas(AtlasFont *me, int EMpixels, char* alphabet){
	//this method assumes one fontsize per atlas, 
	// and automatically adjusts atlas size to minimize wastage
	int rowheight, maxadvancepx, pixelsNeeded;
	unsigned int dimension;
	AtlasEntrySet *aes;
	char *name;
	Atlas *atlas = NULL;

	printf("start of RenderFontAtlas\n");

	if(!me->fontFace) return; //font .ttf file not loaded (likely not found, or programmer didn't load flont first)

	atlas = GUImalloc(&atlas_table,GUI_ATLAS); //malloc(sizeof(GUIAtlas));
	aes = malloc(sizeof(AtlasEntrySet));
	//GUIFontSize *fsize = malloc(sizeof(GUIFontSize));
	name = malloc(strlen(me->name)+12); //base10 -2B has 11 chars, plus \0
	strcpy(name,me->name);
	//sprintf(&name[strlen(me->name)],"%d",EMpixels); //or itoa()
	_itoa(EMpixels,&name[strlen(name)],10);
	AtlasEntrySet_init(aes,name);
	//somehow, I need the EMsize and rowheight, or a more general function to compute area needed by string
	AtlasFont_setFontSize(me,EMpixels, &rowheight, &maxadvancepx);
	pixelsNeeded = rowheight * EMpixels / 2 * strlen(alphabet);
	dimension = (unsigned int)sqrt((double)pixelsNeeded);
	dimension = upperPowerOfTwo(dimension);
	printf("creating atlas %s with dimension %d advance %d rowheight %d\n",name,dimension,EMpixels/2,rowheight);
	Atlas_init(atlas,dimension,rowheight);
	aes->atlas = atlas;
	aes->EMpixels = EMpixels;
	aes->maxadvancepx = maxadvancepx;
	aes->rowheight = rowheight;
	atlas->name = name;
	aes->atlasName = name;
	//init ConsoleMessage font atlas
	RenderFontAtlas(me,aes,alphabet);
	vector_pushBack(AtlasEntrySet*,&me->atlasSizes,aes);
	///vector_pushBack(GUIAtlas*,atlas_table,atlas); //will have fontnameXX where XX is the EMpixel
	printf("end of RenderFontAtlas\n");
}




int bin2hex(char *inpath, char *outpath){
	// converts any binary file -.ttf, .png etc- into a .c file, so you can compile it in
	// then in your code refer to it:
	// extern unsigned char my_data[];
	// extern int my_size;
	int ncol = 15;
	FILE *fin, *fout;
	fin = fopen(inpath,"r+b");
	fout = fopen(outpath,"w+");

	if(fin && fout){
		char *bufname, *bufdup, *ir, *sep;
		int more, m, j, nc;
		unsigned int hh;
		unsigned char *buf;
		buf = malloc(ncol + 1);
		//convert ..\ProggyClean.ttf to ProggyClean_ttf
		bufname = bufdup = strdup(inpath);
		ir = strrchr(bufname,'\\');
		if(ir) bufname = &ir[1];
		ir = strrchr(bufname,'/');
		if(ir) bufname = &ir[1];
		ir = strrchr(bufname,'.');
		if(ir) ir[0] = '_';
		//print data
		fprintf(fout,"unsigned char %s_data[] = \n",bufname);
		sep = "{";
		more = 1;
		m = 0;
		do{
			nc = ncol;
			nc = fread(buf,1,nc,fin);
			if(nc < ncol) more = 0;
			for(j=0;j<nc;j++){
				fprintf(fout,"%s",sep);
				hh = buf[j];
				fprintf(fout,"0x%.2x",hh);
				sep = ",";
			}
			if(more) fprintf(fout,"\n");
			m += nc;
		}while(more);
		fprintf(fout,"};\n");
		//print size
		fprintf(fout,"int %s_size = %d;\n",bufname,m);
		fclose(fout);
		fclose(fin);
		free(buf);
		free(bufdup);
	}
	return 1;
}
int AtlasFont_LoadFromDotC(AtlasFont *font, unsigned char *start, int size){
	FT_Face fontFace;
	FT_Open_Args args;
	int err;	
	char *fontname;
	fontname = font->path;

	if(!size || !start){
		printf("not compiled in C %s\n", font->name);
		return FALSE;
	}

	if(!fontlibrary)
    if ((err = FT_Init_FreeType(&fontlibrary))) {
        fprintf(stderr, "FreeWRL FreeType Initialize error %d\n",err);
        return FALSE;
    }
	args.flags = FT_OPEN_MEMORY;
	args.memory_base = start;
	args.memory_size = size;
	err = FT_Open_Face(fontlibrary, &args, 0,  &fontFace);
   // err = FT_New_Face(fontlibrary, fontname, 0, &fontFace);
    if (err) {
        printf ("FreeType - can not use font %s\n",fontname);
        return FALSE;
    } 
	font->fontFace = fontFace;

	if(1){
		printf("fontface flags & Scalable? = %d \n",fontFace->face_flags & FT_FACE_FLAG_SCALABLE );
		int nsizes = fontFace->num_fixed_sizes;
		printf("num_fixed_sizes = %d\n",nsizes);
	}
	return TRUE;
}


#define DOTC_NONE 0
#define DOTC_SAVE 1
#define DOTC_LOAD 2


#define FONT_TACTIC DOTC_NONE //DOTC_LOAD
#define ATLAS_TACTIC DOTC_NONE
#if FONT_TACTIC > DOTC_SAVE
	extern unsigned char ProggyClean_ttf_data[];
	extern int ProggyClean_ttf_size;
	extern unsigned char ProggyCrisp_ttf_data[];
	extern int ProggyCrisp_ttf_size;
	extern unsigned char VeraMono_ttf_data[];
	extern int VeraMono_ttf_size;
	extern unsigned char freewrl_widgets_ttf_data[];
	extern int freewrl_widgets_ttf_size;
#else
	unsigned char *ProggyClean_ttf_data;
	int ProggyClean_ttf_size = 0;
	unsigned char *ProggyCrisp_ttf_data;
	int ProggyCrisp_ttf_size = 0;
	unsigned char *VeraMono_ttf_data;
	int VeraMono_ttf_size = 0;
	unsigned char *freewrl_widgets_ttf_data;
	int freewrl_widgets_ttf_size = 0;
#endif

#if ATLAS_TACTIC > DOTC_SAVE
	extern unsigned char widgetAtlas_png_data[];
	extern int widgetAtlas_png_size;
#else
	unsigned char *widgetAtlas_png_data = NULL;
	int widgetAtlas_png_size = 0;
#endif

AtlasFont *searchAtlasTableOrLoad(char *facename, int EMpixels){
	AtlasFont *font;
	font = (AtlasFont*)searchGUItable(font_table,facename);
	if(!font){
		static char * ascii32_126 = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQURSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
		int font_tactic, atlas_tactic, len;
		char* facenamettf;
	
		font = GUImalloc(&font_table,GUI_FONT); //sizeof(GUIFont));
		//AtlasFont_init(font,"ProggyClean","ProggyClean.ttf"); 
		len = strlen(facename) + 7;
		facenamettf = malloc(len);
		strcpy(facenamettf,facename);
		facenamettf = strcat(facenamettf,".ttf");
		AtlasFont_init(font,facename,facenamettf); 

		font_tactic = FONT_TACTIC; //DOTC_NONE, DOTC_SAVE, DOTC_LOAD
		if(font_tactic == DOTC_SAVE) {
			char *facenamettfc;
			facenamettfc = alloca(strlen(facenamettf)+3);
			strcpy(facenamettfc,facenamettf);
			facenamettf[len-7] = '_';
			strcat(facenamettf,".c");
			bin2hex(font->path, facenamettfc); // "ProggyClean_ttf.c");
		}
		if(font_tactic == DOTC_LOAD)
			AtlasFont_LoadFromDotC(font, ProggyClean_ttf_data, ProggyClean_ttf_size);
		if(font_tactic != DOTC_LOAD)
			AtlasFont_LoadFont(font); 

		AtlasFont_RenderFontAtlas(font,EMpixels,ascii32_126);
		font = (AtlasFont*)searchGUItable(font_table,facename);

	}
	if(!font){
		printf("dug9gui: Can't find font %s did you misname the fontface sb Vera or VeraMono etc?\n",facename);
	}
	return font;
}

typedef struct GUIElement 
{
	char *name;
	GUIElementType type;  //element = 0, panel 1, image 2, button 3, checkBox 4, textCaption 5, textPanel 6
	//ivec2 anchors;
	void * userData;
} GUIElement;





typedef struct vec2 {float X; float Y;} vec2;
typedef struct vec4 {float X; float Y; float Z; float W;} vec4;
vec4 vec4_init(float x, float y, float z, float w){
	vec4 ret;
	ret.X = x, ret.Y = y; ret.Z = z; ret.W = w;
	return ret;
}
vec2 vec2_init(float x, float y){
	vec2 ret;
	ret.X = x, ret.Y = y; 
	return ret;
}
typedef struct ivec4 {int X; int Y; int W; int H;} ivec4;
ivec4 ivec4_init(int x, int y, int w, int h);


//static Stack *_vpstack = NULL; //ivec4 in y-down pixel coords - viewport stack used for clipping drawing
struct GUIScreen {
	int X,Y; //placeholder for screen WxH
} screen;
 //singleton, screen allows mouse passthrough (vs panel, captures mouse)

vec2 pixel2normalizedViewportScale( GLfloat x, GLfloat y)
{
	vec2 xy;
	GLfloat yup;
	
	ivec4 currentvp = stack_top(ivec4,gglobal()->Mainloop._vportstack);

	//convert to -1 to 1 range
	xy.X = ((GLfloat)(x)/(GLfloat)currentvp.W) * 2.0f;
	xy.Y = ((GLfloat)(y)/(GLfloat)currentvp.H) * 2.0f;
	return xy;
}
vec2 pixel2normalizedViewport( GLfloat x, GLfloat y){
	ivec4 currentvp = stack_top(ivec4,gglobal()->Mainloop._vportstack);

	vec2 xy;
	xy.X = ((GLfloat)(x - currentvp.X)/(GLfloat)currentvp.W) * 2.0f;
	xy.Y = ((GLfloat)(y - currentvp.Y)/(GLfloat)currentvp.H) * 2.0f;
	xy.X -= 1.0f;
	xy.Y -= 1.0f;
	xy.Y *= -1.0f;
	return xy;
}
void printvpstacktop(Stack *vpstack, int line){
	ivec4 currentvp = stack_top(ivec4,vpstack);
	int n = ((struct Vector*)vpstack)->n;
	int xx = ((ivec4*)((struct Vector*)vpstack)->data)[3].X;
	printf("vp top[%d] = [%d %d %d %d] line %d xx=%d\n",n,currentvp.X,currentvp.Y,currentvp.W,currentvp.H,line,xx);
}

vec2 pixel2normalizedScreenScale( GLfloat x, GLfloat y)
{
	vec2 xy;
	GLfloat yup;
	//convert to -1 to 1 range
	xy.X = ((GLfloat)x/(GLfloat)screen.X) * 2.0f;
	xy.Y = ((GLfloat)y/(GLfloat)screen.Y) * 2.0f;
	return xy;
}
vec2 pixel2normalizedScreen( GLfloat x, GLfloat y){
	vec2 xy = pixel2normalizedScreenScale(x,y);
	xy.X -= 1.0f;
	xy.Y -= 1.0f;
	xy.Y *= -1.0f;
	return xy;
}

static   GLbyte vShaderStr[] =  
      "attribute vec4 a_position;   \n"
      "attribute vec2 a_texCoord;   \n"
      "varying vec2 v_texCoord;     \n"
      "void main()                  \n"
      "{                            \n"
      "   gl_Position = a_position; \n"
      "   v_texCoord = a_texCoord;  \n"
      "}                            \n";

// using Luminance images, you need to set a color in order for it to show up different than white
// and if the luminance is an opacity gray-scale for anti-aliased bitmap patterns ie font glyphs
// then transparency = 1 - opacity

//this shader works in win32 desktop angleproject (gets translated to HLSL), although it's a mystery why/how it works Dec 24, 2014.
//In theory:
// the blend should be 1111 if you want all texture
// and blend should be 0000 if you want all vector color (ie drawing a colored rectangle or border)
// and blend should be 0001 if you have an alpha image (ie font glyph image)- so you ignor .rgb of texture
//In practice: there's a (1-blend.a) and (1-texColor.a) I don't understand
//to use an rgba image's own color, set your blend to 1111 and vector color to 1110
//to colorize a gray rgba image using the rgb as a luminance factor, and vector color as the hue, 
//  set blend to 1111 and vector color to your chosen color
static   GLbyte fShaderStr[] =  
#ifdef GL_ES_VERSION_2_0
      "precision mediump float;                            \n"
#endif //GL_ES_VERSION_2_0
      "varying vec2 v_texCoord;                            \n"
      "uniform sampler2D Texture0;                         \n"
      "uniform vec4 Color4f;                               \n"
	  "uniform vec4 blend;                                 \n"
      "void main()                                         \n"
      "{                                                   \n"
	  "  vec4 texColor = texture2D( Texture0, v_texCoord ); \n"    
	  "  vec4 one = vec4(1.0,1.0,1.0,1.0); \n"
	  "  vec4 omb = vec4(one.rgb - blend.rgb,1.0 - blend.a);                \n"
	  "  vec4 tcolor = omb + (blend*texColor);\n"
	  "  float aa = omb.a*Color4f.a + blend.a*(1.0 -texColor.a);\n"
	  "  tcolor = Color4f * tcolor;\n"
      "  vec4 finalColor = vec4(tcolor.rgb, 1.0 - aa ); \n"  
      "  gl_FragColor = finalColor; \n"
      "}                                                   \n";
//	  "  gl_FragColor = vec4(1.0,1.0,1.0,1.0); \n"
	  //"  texColor.a = one.a*blend.a + (one.a - blend.a)*texColor.a;\n"
   //   "  vec4 finalColor = vec4(Color4f.rgb * texColor.rgb, 1.0 - (1.0 - Color4f.a)*(1.0 - texColor.a)); \n"  //vector rgb color, and vector.a * (1-L) for alpha
	//  "  texColor.rgb = blend.rgb + (one.rgb - blend.rgb)*texColor.rgb;\n"
	  //"  texColor.a = blend.a*Color4f.a + (one.a - blend.a)*texColor.a;\n"
   //   "  vec4 finalColor = vec4(Color4f.rgb * texColor.rgb, texColor.a); \n"  //vector rgb color, and vector.a * (1-L) for alpha
//STATICS
static GLuint positionLoc;
static GLuint texCoordLoc;
static GLuint textureLoc;
static GLuint color4fLoc;
//static GLuint programObject;
static GLuint textureID;
//static GLuint indexBufferID;
static GLuint blendLoc;
static GLuint programObject = 0;
GLuint esLoadProgram ( const char *vertShaderSrc, const char *fragShaderSrc ); //defined in statuasbarHud.c
static void initProgramObject(){
   // Load the shaders and get a linked program object
   programObject = esLoadProgram ( (const char*) vShaderStr, (const char *)fShaderStr );
   // Get the attribute locations
   positionLoc = glGetAttribLocation ( programObject, "a_position" );
   texCoordLoc = glGetAttribLocation ( programObject, "a_texCoord" );
   // Get the sampler location
   textureLoc = glGetUniformLocation ( programObject, "Texture0" );
   color4fLoc = glGetUniformLocation ( programObject, "Color4f" );
   blendLoc = glGetUniformLocation ( programObject, "blend" );
}

void dug9gui_DrawImage(int xpos,int ypos, int width, int height, char *buffer){
//xpos, ypos upper left location of image in pixels, on the screen
// hardwired to draw glyph (1-alpha) images
//  1 - 2 4
//  | / / |    2 triangles, 6 points, in y-up coords
//  0 3 - 5
/*
GLfloat cursorVert[] = {
	  0.0f,  1.0f, 0.0f,
	  0.0f,  0.0f, 0.0f,
	  1.0f,  0.0f, 0.0f,
	  0.0f,  1.0f, 0.0f,
	  1.0f,  0.0f, 0.0f,
	  1.0f,  1.0f, 0.0f,
	  };
*/
GLfloat cursorVert[] = {
	  0.0f,  0.0f, 0.0f,
	  0.0f,  1.0f, 0.0f,
	  1.0f,  1.0f, 0.0f,
	  0.0f,  0.0f, 0.0f,
	  1.0f,  1.0f, 0.0f,
	  1.0f,  0.0f, 0.0f,
	  };

GLfloat cursorTex[] = {
	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	0.0f, 0.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,
	};
	GLushort ind[] = {0,1,2,3,4,5};
	//GLint pos, tex;
	vec2 fxy, fwh;
	ivec2 xy;
	int i,j;
	GLfloat cursorVert2[18];
	//unsigned char buffer2[1024];


    if(0) glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE , GL_UNSIGNED_BYTE, buffer);
	//for(int i=0;i<width*height;i++)
	//	buffer2[i] = 255 - (unsigned char)(buffer[i]); //change from (1-alpha) to alpha image
    if(1) glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA , GL_UNSIGNED_BYTE, buffer);
	glUniform4f(blendLoc,0.0f,0.0f,0.0f,1.0f); //-1 is because glyph grayscale are really (1.0 - alpha)

	if(0){
		//upper left
		fxy = pixel2normalizedScreen((GLfloat)xpos,(GLfloat)ypos);
		fwh = pixel2normalizedScreenScale((GLfloat)width,(GLfloat)height);
		//lower left
		fxy.Y = fxy.Y - fwh.Y;
	}
	if(1){
		//upper left
		//printvpstacktop(__LINE__);
		fxy = pixel2normalizedViewport((GLfloat)xpos,(GLfloat)ypos);
		//printvpstacktop(__LINE__);
		fwh = pixel2normalizedViewportScale((GLfloat)width,(GLfloat)height);
		//printvpstacktop(__LINE__);
		//lower left
		fxy.Y = fxy.Y - fwh.Y;
	}

	//fxy.Y -= 1.0; //DUG9GUI y=0 at top
	//fxy.X -= 1.0;
	memcpy(cursorVert2,cursorVert,2*3*3*sizeof(GLfloat));
	//printvpstacktop(__LINE__);

	for(i=0;i<6;i++){
		cursorVert2[i*3 +0] *= fwh.X;
		cursorVert2[i*3 +0] += fxy.X;
		if(!iyup) cursorVert2[i*3 +1] = 1.0f - cursorVert2[i*3 +1];
		cursorVert2[i*3 +1] *= fwh.Y;
		cursorVert2[i*3 +1] += fxy.Y;
	}
	//printvpstacktop(__LINE__);

	// Set the base map sampler to texture unit to 0
	// Bind the base map - see above
	glActiveTexture ( GL_TEXTURE0 );
	glBindTexture ( GL_TEXTURE_2D, textureID );
	glUniform1i ( textureLoc, 0 );

	glVertexAttribPointer (positionLoc, 3, GL_FLOAT, 
						   GL_FALSE, 0, cursorVert2 );
	// Load the texture coordinate
	glVertexAttribPointer (texCoordLoc, 2, GL_FLOAT, GL_FALSE, 0, cursorTex );  
	glEnableVertexAttribArray (positionLoc );
	glEnableVertexAttribArray (texCoordLoc);
	

	//Q do I need to bind a buffer for indexes, just for glew config?
	//printvpstacktop(__LINE__);
	//char *saveme[4*4*4];
	//memcpy(saveme,_vpstack->data,4*4*4); //glew config overwrites vpstack->data top.X
	glDrawElements ( GL_TRIANGLES, 3*2, GL_UNSIGNED_SHORT, ind );
	//memcpy(_vpstack->data,saveme,4*4*4);
	//printvpstacktop(__LINE__);

}
void dug9gui_DrawSubImage(int xpos,int ypos, int xsize, int ysize, int ix, int iy, int iw, int ih, int width, int height, int bpp, char *buffer){
//xpos, ypos upper left location of where to draw the sub-image, in pixels, on the screen
//xsize,ysize - size to stretch the sub-image to on the screen, in pixels
// ix,iy,iw,ih - position and size in pixels of the subimage in a bigger/atlas image, ix,iy is upper left
// width, height - size of bigger/atlas image
// bpp - bytes per pixel: usually 1 for apha images like freetype antialiased glyph imagery, usually 4 for RGBA from .bmp
// buffer - the bigger/atlas imagery pixels
//  1 - 2 4
//  | / / |    2 triangles, 6 points
//  0 3 - 5
// I might want to split this function, so loading the texture to gpu is outside, done once for a series of sub-images
/*
GLfloat cursorVert[] = {
	  0.0f,  1.0f, 0.0f,
	  0.0f,  0.0f, 0.0f,
	  1.0f,  0.0f, 0.0f,
	  0.0f,  1.0f, 0.0f,
	  1.0f,  0.0f, 0.0f,
	  1.0f,  1.0f, 0.0f};
*/
GLfloat cursorVert[] = {
	  0.0f,  0.0f, 0.0f,
	  0.0f,  1.0f, 0.0f,
	  1.0f,  1.0f, 0.0f,
	  0.0f,  0.0f, 0.0f,
	  1.0f,  1.0f, 0.0f,
	  1.0f,  0.0f, 0.0f};
//remember texture coordinates are 0,0 in lower left of texture image
GLfloat cursorTex[] = {
	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	0.0f, 0.0f,
	1.0f, 1.0f,
	1.0f, 0.0f};
	GLushort ind[] = {0,1,2,3,4,5};
	//GLint pos, tex;
	vec2 fxy, fwh, fixy, fiwh;
	ivec2 xy;
	int i,j;
	GLfloat cursorVert2[18];
	GLfloat cursorTex2[12];


	// Bind the base map - see above
	glActiveTexture ( GL_TEXTURE0 );
	glBindTexture ( GL_TEXTURE_2D, textureID );

	// Set the base map sampler to texture unit to 0
	glUniform1i ( textureLoc, 0 );

	switch(bpp){
		case 1:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA , GL_UNSIGNED_BYTE, buffer);
		//glUniform4f(color4fLoc,1.0f,1.0f,1.0f,0.0f);
		glUniform4f(blendLoc,0.0f,0.0f,0.0f,1.0f); // take color from vector, take alpha from texture2D
		break;
		case 2:
		//doesn't seem to come in here if my .png is gray+alpha on win32
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, width, height, 0, GL_LUMINANCE_ALPHA , GL_UNSIGNED_BYTE, buffer);
		break;
		case 4:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA , GL_UNSIGNED_BYTE, buffer);
		glUniform4f(blendLoc,1.0f,1.0f,1.0f,1.0f); //trust the texture2D color and alpha
		break;
		default:
			return;
	}
	if(0){
		//upper left
		fxy = pixel2normalizedScreen((GLfloat)xpos,(GLfloat)ypos);
		fwh = pixel2normalizedScreenScale((GLfloat)xsize,(GLfloat)ysize);
		//lower left
		fxy.Y = fxy.Y - fwh.Y;
	}
	if(1){
		//upper left
		fxy = pixel2normalizedViewport((GLfloat)xpos,(GLfloat)ypos);
		fwh = pixel2normalizedViewportScale((GLfloat)xsize,(GLfloat)ysize);
		//lower left
		fxy.Y = fxy.Y - fwh.Y;
	}
	
	//fxy.Y -= 1.0; //DUG9GUI y=0 at top
	//fxy.X -= 1.0;
	memcpy(cursorVert2,cursorVert,2*3*3*sizeof(GLfloat));
	for(i=0;i<6;i++){
		cursorVert2[i*3 +0] *= fwh.X;
		cursorVert2[i*3 +0] += fxy.X;
		if(!iyup) cursorVert2[i*3 +1] = 1.0f - cursorVert2[i*3 +1];
		cursorVert2[i*3 +1] *= fwh.Y;
		cursorVert2[i*3 +1] += fxy.Y;
	}

	glVertexAttribPointer (positionLoc, 3, GL_FLOAT, 
						   GL_FALSE, 0, cursorVert2 );
	// Load the texture coordinate
	fixy.X = (float)ix/(float)width;
	fiwh.X = (float)iw/(float)width;
	if(!iyup){
		fixy.Y = (float)iy/(float)height;
		fiwh.Y = (float)ih/(float)height;
	}else{
		fixy.Y = (float)(height -iy)/(float)height;
		fiwh.Y =-(float)ih/(float)height;
	}
	memcpy(cursorTex2,cursorTex,2*3*2*sizeof(GLfloat));
	for(i=0;i<6;i++){
		cursorTex2[i*2 +0] *= fiwh.X;
		cursorTex2[i*2 +0] += fixy.X;
		cursorTex2[i*2 +1] *= fiwh.Y;
		cursorTex2[i*2 +1] += fixy.Y;
	}
	glVertexAttribPointer (texCoordLoc, 2, GL_FLOAT, GL_FALSE, 0, cursorTex2 );  
	glEnableVertexAttribArray (positionLoc );
	glEnableVertexAttribArray (texCoordLoc);

	//// Bind the base map - see above
	//glActiveTexture ( GL_TEXTURE0 );
	//glBindTexture ( GL_TEXTURE_2D, textureID );

	//// Set the base map sampler to texture unit to 0
	//glUniform1i ( textureLoc, 0 );
	glDrawElements ( GL_TRIANGLES, 3*2, GL_UNSIGNED_SHORT, ind ); 


}

typedef struct GUITextCaption
{
	GUIElement super;
	char *caption;
	//FT_Face fontFace;
	AtlasFont *font;
	char *fontname;
	int fontSize;
	AtlasEntrySet *set;
	float percentSize;
	int EMpixels; //how differ from fontSize?
	int maxadvancepx;
	float angle;
	//char *fontAtlasName;
	//GUIAtlas *atlas;
} GUITextCaption;

int render_captiontext(AtlasFont *font, AtlasEntrySet *set, unsigned char * utf8string, vec4 color){
	//pass in a string with your alphabet, numbers, symbols or whatever, 
	// and we use freetype2 to render to bitmpa, and then tile those little
	// bitmaps into an atlas texture
	//wText is UTF-8 since FreeType expect this	 
	//FT_Face fontFace;
	int n, err = 0;	
	int  pen_x, pen_y;
	Stack *vportstack;
	ivec4 ivport;
	ttglobal tg = gglobal();


	if(utf8string == NULL) return FALSE;
	// you need to pre-load the font during layout init
	if(!font) return FALSE;

	finishedWithGlobalShader();
	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	if(!programObject) initProgramObject();

	glUseProgram ( programObject );
	if(!textureID)
		glGenTextures(1, &textureID);

	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST); //GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); //GL_LINEAR);

	glUniform4f(color4fLoc,color.X,color.Y,color.Z,color.W); //0.7f,0.7f,0.9f,1.0f);
	//for caption text, we'll set the font size whether or not we have an atlas set, 
	//because the atlas set may not have all the utf8 chars we need, and so we may need to 
	//render and store new ones, and for that we need to set the font size
	#define POINTSIZE 20
	#define XRES 96
	#define YRES 96
	if(0)
	err = FT_Set_Char_Size(font->fontFace, /* handle to face object           */
							POINTSIZE*64,    /* char width in 1/64th of points  */
							POINTSIZE*64,    /* char height in 1/64th of points */
							XRES,            /* horiz device resolution         */
							YRES);           /* vert device resolution          */
	if(1)
	err = FT_Set_Pixel_Sizes(
		font->fontFace,   /* handle to face object */
		0,      /* pixel_width           */
		set->EMpixels);   /* pixel_height          */
    
	if (err) {
		printf ("FreeWRL - FreeType, can not set char size for font %s\n",font->name);
		return FALSE;
	}


	//pen_x = 0; //(int)me->super.proportions.topLeft.X;
	//pen_y = 0; //(int)me->super.proportions.topLeft.Y + 16;
	vportstack = (Stack*)tg->Mainloop._vportstack;
	ivport = stack_top(ivec4,vportstack);
	pen_x = ivport.X;
	pen_y = ivport.Y + ivport.H - set->EMpixels; //MAGIC FORMULA - I'm not sure what this should be, but got something drawing

	//utf8to32 >>
	unsigned char *start, *end;
	int lenchar, l32;
	lenchar = (int)strlen((const char *)utf8string);
	//if(!strncmp(utf8string,"Gr",2)){
	//	printf("length of Green string=%d\n",lenchar);
	//	for(int i=0;i<lenchar;i++){
	//		printf("%d %c\n",(unsigned int)utf8string[i],utf8string[i]);
	//	}
	//	printf("wait\n");
	//}
	start = utf8string;
	end = (unsigned char *)&utf8string[lenchar];
	l32 = 0;
	//<<utf8to32

	//for (int i = 0; i < strlen(cText); i++) 	
	while(start < end)
	{ 	
		AtlasEntry *entry = NULL;
		//utf8to32>>
		unsigned int ichar;
		int inc = 1;
		if(*start < 0x80)
			ichar = *start; //ascii range
		else
			ichar = utf8_to_utf32_char(start,end,&inc);
		start += inc;
		l32++;
		//<<utif8to32
		if(set){
			//check atlas
			entry = AtlasEntrySet_getEntry(set,ichar);
			if(entry){
				// drawsubimage(destination on screen, source glpyh details, source atlas) 
				dug9gui_DrawSubImage(pen_x + entry->pos.X, pen_y - entry->pos.Y, entry->size.X, entry->size.Y, 
					entry->apos.X, entry->apos.Y, entry->size.X, entry->size.Y,
					set->atlas->size.X,set->atlas->size.Y,set->atlas->bytesperpixel,set->atlas->texture);
				pen_x += entry->advance.X; //glyph->advance.x >> 6;
			}
		}
		if(!entry){
			//use freetype2 to render
			unsigned long c = FT_Get_Char_Index(font->fontFace, ichar); 		
			FT_Error error = FT_Load_Glyph(font->fontFace, c, FT_LOAD_RENDER); 	
			if(error) 		
			{ 			
				//Logger::LogWarning("Character %c not found.", wText.GetCharAt(i)); 
				printf("ouch88");
				continue; 		
			}
			FT_GlyphSlot glyph = font->fontFace->glyph;
			/*
			atlasEntry *entry = malloc(sizeof(atlasEntry));
			entry->pos.X = glyph->bitmap_left;
			entry->pos.Y = glyph->bitmap_top;
			entry->advance.X = glyph->advance.x >> 6;
			entry->advance.Y = glyph->advance.y >> 6;
			entry->size.X = glyph->bitmap.width;
			entry->size.Y = glyph->bitmap.rows;
			entry->name = newstringfromchar(cText[i]);
			atlas_addEntry(atlas,entry,glyph->bitmap.buffer);
			*/
			//render one image
			//statusbarHud_DrawCursor(GLint textureID,pos_x,pox_y);
			//if(1) statusbarHud_DrawImage(pen_x + glyph->bitmap_left, pen_y + glyph->bitmap_top - glyph->bitmap.rows, glyph->bitmap.width, glyph->bitmap.rows, glyph->bitmap.buffer);
			//if(1) dug9gui_DrawImage(pen_x + glyph->bitmap_left, pen_y + glyph->bitmap_top - glyph->bitmap.rows, glyph->bitmap.width, glyph->bitmap.rows, glyph->bitmap.buffer);
			if(1) dug9gui_DrawImage(pen_x + glyph->bitmap_left, pen_y - glyph->bitmap_top, glyph->bitmap.width, glyph->bitmap.rows, glyph->bitmap.buffer);
			pen_x += glyph->advance.x >> 6;
			//pen_y += glyph->advance.y >> 6;
		}
	}

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	restoreGlobalShader();

	return TRUE;
}




void *GUImalloc(struct Vector **guitable, int type){
	void *retval = NULL;
	int size = 0;

	switch(type){
		//auxiliary types
		case GUI_ATLAS:			size = sizeof(Atlas); break;
		case GUI_FONT:			size = sizeof(AtlasFont); break;
		case GUI_ATLASENTRY:	size = sizeof(AtlasEntry); break;
		case GUI_ATLASENTRYSET: size = sizeof(AtlasEntrySet); break;
		default:
			printf("no guielement of this type %d\n",type);
	}
	if(size){
		retval = malloc(size);
		//add to any tables
		if(guitable){
			if(*guitable == NULL) *guitable = newVector(GUIElement*,20);
			vector_pushBack(GUIElement*,*guitable,retval);
		}
	}
	return retval;
}









