/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

#ifndef __HEADERS_H__
#define __HEADERS_H__
 	 
#include "Structs.h"

#include "LinearAlgebra.h"
#include "constants.h"

#ifndef AQUA
#include <GL/glu.h>
#else
#include <glu.h>
#include <CGLTypes.h>
#include "aquaInt.h"
extern CGLContextObj aqglobalContext;
#endif

/* number of tesselated coordinates allowed */
#define TESS_MAX_COORDS  500

#define offset_of(p_type,field) ((unsigned int)(&(((p_type)NULL)->field)-NULL))

#ifndef FALSE
#define FALSE 0
#endif
 	 
#ifndef TRUE
#define TRUE 1
#endif

#define UNUSED(v) ((void) v)

#define BOOL_STRING(b) (b ? "TRUE" : "FALSE")


#ifdef M_PI
#define PI M_PI
#else
#define PI 3.141592653589793
#endif


/* defines for raycasting: */
#define APPROX(a,b) (fabs(a-b)<0.00000001)
#define NORMAL_VECTOR_LENGTH_TOLERANCE 0.00001
/* (test if the vector part of a rotation is normalized) */
#define IS_ROTATION_VEC_NOT_NORMAL(rot)        ( \
       fabs(1-sqrt(rot.r[0]*rot.r[0]+rot.r[1]*rot.r[1]+rot.r[2]*rot.r[2])) \
               >NORMAL_VECTOR_LENGTH_TOLERANCE \
)

/* defines for raycasting: */
#define XEQ (APPROX(t_r1.x,t_r2.x))
#define YEQ (APPROX(t_r1.y,t_r2.y))
#define ZEQ (APPROX(t_r1.z,t_r2.z))
/* xrat(a) = ratio to reach coordinate a on axis x */
#define XRAT(a) (((a)-t_r1.x)/(t_r2.x-t_r1.x))
#define YRAT(a) (((a)-t_r1.y)/(t_r2.y-t_r1.y))
#define ZRAT(a) (((a)-t_r1.z)/(t_r2.z-t_r1.z))
/* mratx(r) = x-coordinate gotten by multiplying by given ratio */
#define MRATX(a) (t_r1.x + (a)*(t_r2.x-t_r1.x))
#define MRATY(a) (t_r1.y + (a)*(t_r2.y-t_r1.y))
#define MRATZ(a) (t_r1.z + (a)*(t_r2.z-t_r1.z))
/* trat: test if a ratio is reasonable */
#undef TRAT
#define TRAT(a) 1
#undef TRAT
#define TRAT(a) ((a) > 0 && ((a) < hpdist || hpdist < 0))



/* POLYREP stuff */
#define POINT_FACES	16 /* give me a point, and it is in up to xx faces */

/* Function Prototypes */

void render_node(void *node);

void rayhit(float rat, float cx,float cy,float cz, float nx,float ny,float nz,
float tx,float ty, char *descr) ;

void fwnorprint (float *norm);


/* not defined anywhere: */
/* void Extru_init_tex_cap_vals(); */


/* from the PNG examples */
unsigned char  *readpng_get_image(double display_exponent, int *pChannels,
		                       unsigned long *pRowbytes);

/* Used to determine in Group, etc, if a child is a DirectionalLight; do comparison with this */
void DirectionalLight_Rend(void *nod_);


void normalize_ifs_face (float *point_normal,
                         struct pt *facenormals,
                         int *pointfaces,
                        int mypoint,
                        int curpoly,
                        float creaseAngle);


void FW_rendertext(unsigned int numrows,SV **ptr,char *directstring, unsigned int nl, double *length,
                double maxext, double spacing, double mysize, unsigned int fsparam,
                struct VRML_PolyRep *rp);


/* Triangulator extern defs - look in CFuncs/Tess.c */
extern struct VRML_PolyRep *global_tess_polyrep;
extern GLUtriangulatorObj *global_tessobj;
extern int global_IFS_Coords[];
extern int global_IFS_Coord_count;

/* do we have to do textures?? */
#define HAVETODOTEXTURES (last_bound_texture != 0)

extern int _fw_pipe, _fw_FD;
#define RUNNINGASPLUGIN (_fw_pipe != 0)

/* appearance does material depending on last texture depth */
extern int last_texture_depth;


/* what is the max texture size as set by FreeWRL? */
extern GLint global_texSize;


/* Text node system fonts. On startup, freewrl checks to see where the fonts
 * are stored
 */
#define fp_name_len 256
extern char sys_fp[fp_name_len];


extern float AC_LastDuration[];

extern int SoundEngineStarted;

/* Material optimizations */
void do_shininess (float shininess);
void do_glMaterialfv (GLenum face, GLenum pname, GLfloat *param);

/* Some drivers give a GL error if doing a glIsEnabled when creating
   display lists, so we just use an already created variable. */
extern GLuint last_bound_texture;

/* used to determine whether we have transparent materials. */
extern int have_transparency;


/* current time */
extern double TickTime;


/* Transform node optimizations */
int verify_rotate(GLfloat *params);
int verify_translate(GLfloat *params);
int verify_scale(GLfloat *params);

/* C routes */
#define MAXJSVARIABLELENGTH 25	/* variable name length can be this long... */

void mark_event (unsigned int from, unsigned int fromoffset);

/* saved rayhit and hyperhit */
extern struct SFColor ray_save_posn, hyp_save_posn, hyp_save_norm;

/* set a node to be sensitive */
void setSensitive(void *ptr,int datanode,char *type);

/* bindable nodes */
extern GLint viewport[];
extern GLdouble fieldofview;
extern int found_vp;
extern struct pt ViewerUpvector;
extern struct sNaviInfo naviinfo;


/* Sending events back to Browser (eg, Anchor) */
extern int BrowserAction;
extern struct Multi_String Anchor_url;


/* Scripting Routing interfaces */

#define SFUNKNOWN 0
#define SFBOOL 	1
#define SFCOLOR 2
#define SFFLOAT 3
#define SFTIME 	4
#define SFINT32 5
#define SFSTRING 6
#define SFNODE	7
#define SFROTATION 8
#define SFVEC2F	9
#define SFIMAGE	10

#define MFCOLOR 11
#define MFFLOAT 12
#define MFTIME 	13
#define MFINT32 14
#define MFSTRING 15
#define MFNODE	16
#define MFROTATION 17
#define MFVEC2F	18
#define MFVEC3F 19
#define SFVEC3F 20


#define FIELD_TYPE_STRING(f) ( \
	f == SFBOOL ? "SFBool" : ( \
	f == SFCOLOR ? "SFColor" : ( \
	f == SFVEC3F ? "SFVec3f" : ( \
	f == SFFLOAT ? "SFFloat" : ( \
	f == SFTIME ? "SFTime" : ( \
	f == SFINT32 ? "SFInt32" : ( \
	f == SFSTRING ? "SFString" : ( \
	f == SFNODE ? "SFNode" : ( \
	f == SFROTATION ? "SFRotation" : ( \
	f == SFVEC2F ? "SFVec2f" : ( \
	f == SFIMAGE ? "SFImage" : ( \
	f == MFCOLOR ? "MFColor" : ( \
	f == MFVEC3F ? "MFVec3f" : ( \
	f == MFFLOAT ? "MFFloat" : ( \
	f == MFTIME ? "MFTime" : ( \
	f == MFINT32 ? "MFInt32" : ( \
	f == MFSTRING ? "MFString" : ( \
	f == MFNODE ? "MFNode" : ( \
	f == MFROTATION ? "MFRotation" : ( \
	f == MFVEC2F ? "MFVec2f" : ( \
	f == MFVEC3F ? "MFVec3f" : ( \
	f == MFROTATION ? "MFRotation" : ( \
	f == SFVEC2F ? "SFVec2f" : "unknown field type")))))))))))))))))))))))


void CRoutes_js_new (int num,int scriptType, unsigned int cx, unsigned int glob, unsigned int brow);
void gatherScriptEventOuts(int script, int ignore);
void getMFNodetype (char *strp, struct Multi_Node *ch, int ar);

void update_node(void *ptr);

extern int CRVerbose, JSVerbose;

int JSparamIndex (char *name, char *type);

/* setting script eventIns from routing table or EAI */
void Set_one_MultiElementtype (int tn, int tptr, void *fn, unsigned len);
void set_one_ECMAtype (int tonode, int toname, int dataType, void *Data, unsigned datalen);
void mark_script (int num);


/* structure for rayhits */
struct currayhit {
	void *node; /* What node hit at that distance? */
	GLdouble modelMatrix[16]; /* What the matrices were at that node */
	GLdouble projMatrix[16];
};



struct CRscriptStruct {
	/* type */
	int thisScriptType;

	/* Javascript parameters */
	unsigned int	cx;	/* JSContext		*/
	unsigned int	glob;	/* JSGlobals		*/
	unsigned int	brow;	/* BrowserIntern	*/

	/* Java .CLASS parameters */
	unsigned int 	initialized; 	/* has initialize been sent? */
	int listen_fd, send_fd;		/* socket descriptors */
	char NodeID[20];		/* combo Perl NODEXXX, and CNODE */

};
void JSMaxAlloc(void);
void cleanupDie(int num, char *msg);
void shutdown_EAI(void);
unsigned int EAI_GetNode(char *str);
void EAI_GetType (unsigned int uretval,
        char *ctmp, char *dtmp,
        int *ra, int *rb,
        int *rc, int *rd, int *re);


void setECMAtype(int num);
int get_touched_flag(int fptr, int actualscript);
void getMultiElementtype(char *strp, struct Multi_Vec3f *tn, int eletype);
void setMultiElementtype(int num);
void Multimemcpy(void *tn, void *fn, int len);
void CRoutes_Register(int adrem,        unsigned int from,
                                 int fromoffset,
                                 unsigned int to_count,
                                 char *tonode_str,
                                 int length,
                                 void *intptr,
                                 int scrdir,
                                 int extra);
void CRoutes_free(void);
void mark_script(int num);
void propagate_events(void);
void sendScriptEventIn(int num);
void add_first(char *clocktype,unsigned int node);
void do_first(void);
void process_eventsProcessed(void);



extern struct CRscriptStruct *ScriptControl; /* Script invocation parameters */
extern int *scr_act;    /* script active array - defined in CRoutes.c */
extern int *thisScriptType;    /* what kind of script this is - in CRoutes.c */
extern int JSMaxScript;  /* defined in JSscipts.c; maximum size of script arrays */
void render_status(void); /* status bar */
void update_status(void); 	/* update status bar */
void viewer_type_status(int x);		/* tell status bar what kind of viewer */
void viewpoint_name_status(char *str); /* tell status bar name of current vp 	*/
int convert_typetoInt (char *type);	/* convert a string, eg "SFBOOL" to type, eg SFBOOL */

extern double BrowserFPS;
void render_polyrep(void *node,
        int npoints, struct SFColor *points,
        int ncolors, struct SFColor *colors,
        int nnormals, struct SFColor *normals,
        int ntexcoords, struct SFVec2f *texcoords);


extern int CRoutesExtra;		// let EAI see param of routing table - Listener data.

unsigned EAI_do_ExtraMemory (int size,SV *data,char *type);

/* types of scripts. */
#define NOSCRIPT 	0
#define JAVASCRIPT	1
#define	CLASSSCRIPT	2
#define	PERLSCRIPT	3
#define SHADERSCRIPT	4

//printf is defined by perl; causes segfault in threaded freewrl
#ifdef printf
#undef printf 
#endif
#ifdef die
#undef die
#endif


/* types to tell the Perl thread what to handle */
#define FROMSTRING 	1
#define	FROMURL		2
#define INLINE		3
#define CALLMETHOD	4   /* Javascript... 	*/
#define CALLMETHODVA    5   /* Javascript... 	*/
#define EAIGETNODE      6   /* EAI getNode      	*/
#define EAIGETTYPE	7   /* EAI getType	*/
#define EAIREPWORLD     8   /* EAI replace world */
#define EAIROUTE	9   /* EAI add/del route */
#define EAIGETVALUE	10  /* get a value of a node */
#define EAIGETTYPENAME	11  /* get the type name for a  node */




extern int rootNode;
extern int isPerlParsing(void);
extern int isTextureParsing(void);
extern void loadInline(struct VRML_Inline *node);
extern void loadImageTexture(struct VRML_ImageTexture *node);
extern void loadPixelTexture(struct VRML_PixelTexture *node);
extern void loadMovieTexture(struct VRML_MovieTexture *node);
extern void loadBackgroundTextures (struct VRML_Background *node);
extern GLfloat boxtex[], boxnorms[], BackgroundVert[];
extern GLfloat Backtex[], Backnorms[];

extern void new_tessellation(void);
extern void initializePerlThread(char *perlpath);
extern PerlInterpreter *my_perl;
extern void setGeometry (char *optarg);
extern void setPluginPipe(char *optarg);
extern void setPluginFD(char *optarg);
extern void setPluginInstance(char *optarg);

/* shutter glasses, stereo view  from Mufti@rus */
extern void setShutter (void);
#ifndef AQUA
extern int shutter;
#endif
extern void setScreenDist (char *optArg);
extern void setStereoParameter (char *optArg);
extern void setEyeDist (char *optArg);

extern int isPerlinitialized(void);
extern char *BrowserName, *BrowserVersion, *BrowserURL; // defined in VRMLC.pm
extern int display_status;		// toggle status bar - defined in VRMLC.pm
extern int be_collision;		// toggle collision detection - defined in VRMLC.pm
extern double hpdist;			// in VRMLC.pm
extern struct pt hp;			// in VRMLC.pm
extern void *hypersensitive; 		// in VRMLC.pm
extern int hyperhit;			// in VRMLC.pm
extern struct pt r1, r2;		// in VRMLC.pm
extern struct sCollisionInfo CollisionInfo;
extern struct currayhit rh,rph,rhhyper;
extern int smooth_normals;
extern void xs_init(void);
extern int navi_tos;
extern void initializeTextureThread(void);
extern int isTextureinitialized(void);
extern int fileExists(char *fname, char *firstBytes);
extern void checkAndAllocMemTables(int *texture_num, int increment);
extern void   storeMPGFrameData(int latest_texture_number, int h_size, int v_size,
        int mt_repeatS, int mt_repeatT, char *Image);
void mpg_main(char *filename, int *x,int *y,int *depth,int *frameCount,char *ptr);
void makeAbsoluteFileName(char *filename, char *pspath,char *thisurl);

#ifndef AQUA
extern int wantEAI;
#endif
void create_EAI(void);
int EAI_CreateVrml(char *type, char *str, unsigned int *retarr, int retarrsize);
void EAI_Route(char cmnd, char *tf);
void EAI_replaceWorld(char *inputstring);

void render_hier(void *p, int rwhat);
void handle_EAI(void);

extern int screenWidth, screenHeight;

/* SD AQUA FUNCTIONS */
#ifdef AQUA
extern int getOffset();
extern void initGL();
extern void setSnapSeq();
extern void setSeqFile(char* file);
extern void setSnapFile(char* file);
extern void setMaxImages(int max);
extern void setButDown(int button, int value);
extern void setBrowserURL(char *str);
extern void setCurXY(int x, int y);
extern void setScreenDim(int w, int h);
extern void setLastMouseEvent(int etype);
extern void initFreewrl();
extern void setSeqTemp(char* file);
extern void aqDisplayThread();
#endif

extern char *getLibVersion();
extern void doQuit(void);
extern void doBrowserAction ();


extern char *myPerlInstallDir;

extern void freewrlDie (const char *format);

/* Java CLASS invocation */
int newJavaClass(int scriptInvocationNumber,char * nodestr,char *node);
int initJavaClass(int scriptno); 

char *EAI_GetTypeName (unsigned int uretval);
char *EAI_GetValue (unsigned int uretval,
		        char *ctmp, char *dtmp);
void setCLASStype (int num); 
void sendCLASSEvent(int fn, int scriptno, char *fieldName, int type, int len);
void processClassEvents(int scriptno, int startEntry, int endEntry); 
char *processThisClassEvent (unsigned int fn, int startEntry, int endEntry, char *buf);
int ScanValtoBuffer(int len, int type, char *buf, void *memptr, int buflen); 
void getCLASSMultNumType (char *buf, int bufSize, struct Multi_Vec3f *tn, int eletype, int addChild) ;


#endif /* __HEADERS_H__ */
