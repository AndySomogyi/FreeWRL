#ifndef OPENGL_VIRT
#define OPENGL_VIRT
typedef struct OpenGLVTab
	{
#define VFUNC(type,name,mem,args) type (*mem) args;
#define VVAR(type,name,mem)       type (*mem);
#include "OpenGL.vf"
#undef VFUNC
#undef VVAR
} OpenGLVTab;
extern OpenGLVTab *OpenGLVPtr;
#define D_OPENGL OpenGLVTab *OpenGLVPtr
#define I_OPENGL OpenGLVPtr = (OpenGLVTab *) SvIV(perl_get_sv("VRML::OpenGLVPtr",5));   
#ifndef OPENGL_NOVIRT
#define glClearIndex (*OpenGLVPtr->V_glClearIndex)
#define glClearColor (*OpenGLVPtr->V_glClearColor)
#define glClear (*OpenGLVPtr->V_glClear)
#define glIndexMask (*OpenGLVPtr->V_glIndexMask)
#define glColorMask (*OpenGLVPtr->V_glColorMask)
#define glAlphaFunc (*OpenGLVPtr->V_glAlphaFunc)
#define glBlendFunc (*OpenGLVPtr->V_glBlendFunc)
#define glLogicOp (*OpenGLVPtr->V_glLogicOp)
#define glCullFace (*OpenGLVPtr->V_glCullFace)
#define glFrontFace (*OpenGLVPtr->V_glFrontFace)
#define glPointSize (*OpenGLVPtr->V_glPointSize)
#define glLineWidth (*OpenGLVPtr->V_glLineWidth)
#define glLineStipple (*OpenGLVPtr->V_glLineStipple)
#define glPolygonMode (*OpenGLVPtr->V_glPolygonMode)
#define glPolygonStipple (*OpenGLVPtr->V_glPolygonStipple)
#define glGetPolygonStipple (*OpenGLVPtr->V_glGetPolygonStipple)
#define glEdgeFlag (*OpenGLVPtr->V_glEdgeFlag)
#define glEdgeFlagv (*OpenGLVPtr->V_glEdgeFlagv)
#define glScissor (*OpenGLVPtr->V_glScissor)
#define glClipPlane (*OpenGLVPtr->V_glClipPlane)
#define glGetClipPlane (*OpenGLVPtr->V_glGetClipPlane)
#define glDrawBuffer (*OpenGLVPtr->V_glDrawBuffer)
#define glReadBuffer (*OpenGLVPtr->V_glReadBuffer)
#define glEnable (*OpenGLVPtr->V_glEnable)
#define glDisable (*OpenGLVPtr->V_glDisable)
#define glIsEnabled (*OpenGLVPtr->V_glIsEnabled)
#define glGetBooleanv (*OpenGLVPtr->V_glGetBooleanv)
#define glGetDoublev (*OpenGLVPtr->V_glGetDoublev)
#define glGetFloatv (*OpenGLVPtr->V_glGetFloatv)
#define glGetIntegerv (*OpenGLVPtr->V_glGetIntegerv)
#define glPushAttrib (*OpenGLVPtr->V_glPushAttrib)
#define glPopAttrib (*OpenGLVPtr->V_glPopAttrib)
#define glRenderMode (*OpenGLVPtr->V_glRenderMode)
#define glGetError (*OpenGLVPtr->V_glGetError)
#define glGetString (*OpenGLVPtr->V_glGetString)
#define glFinish (*OpenGLVPtr->V_glFinish)
#define glFlush (*OpenGLVPtr->V_glFlush)
#define glHint (*OpenGLVPtr->V_glHint)
#define glClearDepth (*OpenGLVPtr->V_glClearDepth)
#define glDepthFunc (*OpenGLVPtr->V_glDepthFunc)
#define glDepthMask (*OpenGLVPtr->V_glDepthMask)
#define glDepthRange (*OpenGLVPtr->V_glDepthRange)
#define glClearAccum (*OpenGLVPtr->V_glClearAccum)
#define glAccum (*OpenGLVPtr->V_glAccum)
#define glMatrixMode (*OpenGLVPtr->V_glMatrixMode)
#define glOrtho (*OpenGLVPtr->V_glOrtho)
#define glFrustum (*OpenGLVPtr->V_glFrustum)
#define glViewport (*OpenGLVPtr->V_glViewport)
#define glPushMatrix (*OpenGLVPtr->V_glPushMatrix)
#define glPopMatrix (*OpenGLVPtr->V_glPopMatrix)
#define glLoadIdentity (*OpenGLVPtr->V_glLoadIdentity)
#define glLoadMatrixd (*OpenGLVPtr->V_glLoadMatrixd)
#define glLoadMatrixf (*OpenGLVPtr->V_glLoadMatrixf)
#define glMultMatrixd (*OpenGLVPtr->V_glMultMatrixd)
#define glMultMatrixf (*OpenGLVPtr->V_glMultMatrixf)
#define glRotated (*OpenGLVPtr->V_glRotated)
#define glRotatef (*OpenGLVPtr->V_glRotatef)
#define glScaled (*OpenGLVPtr->V_glScaled)
#define glScalef (*OpenGLVPtr->V_glScalef)
#define glTranslated (*OpenGLVPtr->V_glTranslated)
#define glTranslatef (*OpenGLVPtr->V_glTranslatef)
#define glIsList (*OpenGLVPtr->V_glIsList)
#define glDeleteLists (*OpenGLVPtr->V_glDeleteLists)
#define glGenLists (*OpenGLVPtr->V_glGenLists)
#define glGenTextures (*OpenGLVPtr->V_glGenTextures)
#define glDeleteTextures (*OpenGLVPtr->V_glDeleteTextures)
#define glBindTexture (*OpenGLVPtr->V_glBindTexture)
#define glNewList (*OpenGLVPtr->V_glNewList)
#define glEndList (*OpenGLVPtr->V_glEndList)
#define glCallList (*OpenGLVPtr->V_glCallList)
#define glCallLists (*OpenGLVPtr->V_glCallLists)
#define glListBase (*OpenGLVPtr->V_glListBase)
#define glBegin (*OpenGLVPtr->V_glBegin)
#define glEnd (*OpenGLVPtr->V_glEnd)
#define glVertex2d (*OpenGLVPtr->V_glVertex2d)
#define glVertex2f (*OpenGLVPtr->V_glVertex2f)
#define glVertex2i (*OpenGLVPtr->V_glVertex2i)
#define glVertex2s (*OpenGLVPtr->V_glVertex2s)
#define glVertex3d (*OpenGLVPtr->V_glVertex3d)
#define glVertex3f (*OpenGLVPtr->V_glVertex3f)
#define glVertex3i (*OpenGLVPtr->V_glVertex3i)
#define glVertex3s (*OpenGLVPtr->V_glVertex3s)
#define glVertex4d (*OpenGLVPtr->V_glVertex4d)
#define glVertex4f (*OpenGLVPtr->V_glVertex4f)
#define glVertex4i (*OpenGLVPtr->V_glVertex4i)
#define glVertex4s (*OpenGLVPtr->V_glVertex4s)
#define glVertex2dv (*OpenGLVPtr->V_glVertex2dv)
#define glVertex2fv (*OpenGLVPtr->V_glVertex2fv)
#define glVertex2iv (*OpenGLVPtr->V_glVertex2iv)
#define glVertex2sv (*OpenGLVPtr->V_glVertex2sv)
#define glVertex3dv (*OpenGLVPtr->V_glVertex3dv)
#define glVertex3fv (*OpenGLVPtr->V_glVertex3fv)
#define glVertex3iv (*OpenGLVPtr->V_glVertex3iv)
#define glVertex3sv (*OpenGLVPtr->V_glVertex3sv)
#define glVertex4dv (*OpenGLVPtr->V_glVertex4dv)
#define glVertex4fv (*OpenGLVPtr->V_glVertex4fv)
#define glVertex4iv (*OpenGLVPtr->V_glVertex4iv)
#define glVertex4sv (*OpenGLVPtr->V_glVertex4sv)
#define glNormal3b (*OpenGLVPtr->V_glNormal3b)
#define glNormal3d (*OpenGLVPtr->V_glNormal3d)
#define glNormal3f (*OpenGLVPtr->V_glNormal3f)
#define glNormal3i (*OpenGLVPtr->V_glNormal3i)
#define glNormal3s (*OpenGLVPtr->V_glNormal3s)
#define glNormal3bv (*OpenGLVPtr->V_glNormal3bv)
#define glNormal3dv (*OpenGLVPtr->V_glNormal3dv)
#define glNormal3fv (*OpenGLVPtr->V_glNormal3fv)
#define glNormal3iv (*OpenGLVPtr->V_glNormal3iv)
#define glNormal3sv (*OpenGLVPtr->V_glNormal3sv)
#define glIndexd (*OpenGLVPtr->V_glIndexd)
#define glIndexf (*OpenGLVPtr->V_glIndexf)
#define glIndexi (*OpenGLVPtr->V_glIndexi)
#define glIndexs (*OpenGLVPtr->V_glIndexs)
#define glIndexdv (*OpenGLVPtr->V_glIndexdv)
#define glIndexfv (*OpenGLVPtr->V_glIndexfv)
#define glIndexiv (*OpenGLVPtr->V_glIndexiv)
#define glIndexsv (*OpenGLVPtr->V_glIndexsv)
#define glColor3b (*OpenGLVPtr->V_glColor3b)
#define glColor3d (*OpenGLVPtr->V_glColor3d)
#define glColor3f (*OpenGLVPtr->V_glColor3f)
#define glColor3i (*OpenGLVPtr->V_glColor3i)
#define glColor3s (*OpenGLVPtr->V_glColor3s)
#define glColor3ub (*OpenGLVPtr->V_glColor3ub)
#define glColor3ui (*OpenGLVPtr->V_glColor3ui)
#define glColor3us (*OpenGLVPtr->V_glColor3us)
#define glColor4b (*OpenGLVPtr->V_glColor4b)
#define glColor4d (*OpenGLVPtr->V_glColor4d)
#define glColor4f (*OpenGLVPtr->V_glColor4f)
#define glColor4i (*OpenGLVPtr->V_glColor4i)
#define glColor4s (*OpenGLVPtr->V_glColor4s)
#define glColor4ub (*OpenGLVPtr->V_glColor4ub)
#define glColor4ui (*OpenGLVPtr->V_glColor4ui)
#define glColor4us (*OpenGLVPtr->V_glColor4us)
#define glColor3bv (*OpenGLVPtr->V_glColor3bv)
#define glColor3dv (*OpenGLVPtr->V_glColor3dv)
#define glColor3fv (*OpenGLVPtr->V_glColor3fv)
#define glColor3iv (*OpenGLVPtr->V_glColor3iv)
#define glColor3sv (*OpenGLVPtr->V_glColor3sv)
#define glColor3ubv (*OpenGLVPtr->V_glColor3ubv)
#define glColor3uiv (*OpenGLVPtr->V_glColor3uiv)
#define glColor3usv (*OpenGLVPtr->V_glColor3usv)
#define glColor4bv (*OpenGLVPtr->V_glColor4bv)
#define glColor4dv (*OpenGLVPtr->V_glColor4dv)
#define glColor4fv (*OpenGLVPtr->V_glColor4fv)
#define glColor4iv (*OpenGLVPtr->V_glColor4iv)
#define glColor4sv (*OpenGLVPtr->V_glColor4sv)
#define glColor4ubv (*OpenGLVPtr->V_glColor4ubv)
#define glColor4uiv (*OpenGLVPtr->V_glColor4uiv)
#define glColor4usv (*OpenGLVPtr->V_glColor4usv)
#define glTexCoord1d (*OpenGLVPtr->V_glTexCoord1d)
#define glTexCoord1f (*OpenGLVPtr->V_glTexCoord1f)
#define glTexCoord1i (*OpenGLVPtr->V_glTexCoord1i)
#define glTexCoord1s (*OpenGLVPtr->V_glTexCoord1s)
#define glTexCoord2d (*OpenGLVPtr->V_glTexCoord2d)
#define glTexCoord2f (*OpenGLVPtr->V_glTexCoord2f)
#define glTexCoord2i (*OpenGLVPtr->V_glTexCoord2i)
#define glTexCoord2s (*OpenGLVPtr->V_glTexCoord2s)
#define glTexCoord3d (*OpenGLVPtr->V_glTexCoord3d)
#define glTexCoord3f (*OpenGLVPtr->V_glTexCoord3f)
#define glTexCoord3i (*OpenGLVPtr->V_glTexCoord3i)
#define glTexCoord3s (*OpenGLVPtr->V_glTexCoord3s)
#define glTexCoord4d (*OpenGLVPtr->V_glTexCoord4d)
#define glTexCoord4f (*OpenGLVPtr->V_glTexCoord4f)
#define glTexCoord4i (*OpenGLVPtr->V_glTexCoord4i)
#define glTexCoord4s (*OpenGLVPtr->V_glTexCoord4s)
#define glTexCoord1dv (*OpenGLVPtr->V_glTexCoord1dv)
#define glTexCoord1fv (*OpenGLVPtr->V_glTexCoord1fv)
#define glTexCoord1iv (*OpenGLVPtr->V_glTexCoord1iv)
#define glTexCoord1sv (*OpenGLVPtr->V_glTexCoord1sv)
#define glTexCoord2dv (*OpenGLVPtr->V_glTexCoord2dv)
#define glTexCoord2fv (*OpenGLVPtr->V_glTexCoord2fv)
#define glTexCoord2iv (*OpenGLVPtr->V_glTexCoord2iv)
#define glTexCoord2sv (*OpenGLVPtr->V_glTexCoord2sv)
#define glTexCoord3dv (*OpenGLVPtr->V_glTexCoord3dv)
#define glTexCoord3fv (*OpenGLVPtr->V_glTexCoord3fv)
#define glTexCoord3iv (*OpenGLVPtr->V_glTexCoord3iv)
#define glTexCoord3sv (*OpenGLVPtr->V_glTexCoord3sv)
#define glTexCoord4dv (*OpenGLVPtr->V_glTexCoord4dv)
#define glTexCoord4fv (*OpenGLVPtr->V_glTexCoord4fv)
#define glTexCoord4iv (*OpenGLVPtr->V_glTexCoord4iv)
#define glTexCoord4sv (*OpenGLVPtr->V_glTexCoord4sv)
#define glRasterPos2d (*OpenGLVPtr->V_glRasterPos2d)
#define glRasterPos2f (*OpenGLVPtr->V_glRasterPos2f)
#define glRasterPos2i (*OpenGLVPtr->V_glRasterPos2i)
#define glRasterPos2s (*OpenGLVPtr->V_glRasterPos2s)
#define glRasterPos3d (*OpenGLVPtr->V_glRasterPos3d)
#define glRasterPos3f (*OpenGLVPtr->V_glRasterPos3f)
#define glRasterPos3i (*OpenGLVPtr->V_glRasterPos3i)
#define glRasterPos3s (*OpenGLVPtr->V_glRasterPos3s)
#define glRasterPos4d (*OpenGLVPtr->V_glRasterPos4d)
#define glRasterPos4f (*OpenGLVPtr->V_glRasterPos4f)
#define glRasterPos4i (*OpenGLVPtr->V_glRasterPos4i)
#define glRasterPos4s (*OpenGLVPtr->V_glRasterPos4s)
#define glRasterPos2dv (*OpenGLVPtr->V_glRasterPos2dv)
#define glRasterPos2fv (*OpenGLVPtr->V_glRasterPos2fv)
#define glRasterPos2iv (*OpenGLVPtr->V_glRasterPos2iv)
#define glRasterPos2sv (*OpenGLVPtr->V_glRasterPos2sv)
#define glRasterPos3dv (*OpenGLVPtr->V_glRasterPos3dv)
#define glRasterPos3fv (*OpenGLVPtr->V_glRasterPos3fv)
#define glRasterPos3iv (*OpenGLVPtr->V_glRasterPos3iv)
#define glRasterPos3sv (*OpenGLVPtr->V_glRasterPos3sv)
#define glRasterPos4dv (*OpenGLVPtr->V_glRasterPos4dv)
#define glRasterPos4fv (*OpenGLVPtr->V_glRasterPos4fv)
#define glRasterPos4iv (*OpenGLVPtr->V_glRasterPos4iv)
#define glRasterPos4sv (*OpenGLVPtr->V_glRasterPos4sv)
#define glRectd (*OpenGLVPtr->V_glRectd)
#define glRectf (*OpenGLVPtr->V_glRectf)
#define glRecti (*OpenGLVPtr->V_glRecti)
#define glRects (*OpenGLVPtr->V_glRects)
#define glRectdv (*OpenGLVPtr->V_glRectdv)
#define glRectfv (*OpenGLVPtr->V_glRectfv)
#define glRectiv (*OpenGLVPtr->V_glRectiv)
#define glRectsv (*OpenGLVPtr->V_glRectsv)
#define glShadeModel (*OpenGLVPtr->V_glShadeModel)
#define glLightf (*OpenGLVPtr->V_glLightf)
#define glLighti (*OpenGLVPtr->V_glLighti)
#define glLightfv (*OpenGLVPtr->V_glLightfv)
#define glLightiv (*OpenGLVPtr->V_glLightiv)
#define glGetLightfv (*OpenGLVPtr->V_glGetLightfv)
#define glGetLightiv (*OpenGLVPtr->V_glGetLightiv)
#define glLightModelf (*OpenGLVPtr->V_glLightModelf)
#define glLightModeli (*OpenGLVPtr->V_glLightModeli)
#define glLightModelfv (*OpenGLVPtr->V_glLightModelfv)
#define glLightModeliv (*OpenGLVPtr->V_glLightModeliv)
#define glMaterialf (*OpenGLVPtr->V_glMaterialf)
#define glMateriali (*OpenGLVPtr->V_glMateriali)
#define glMaterialfv (*OpenGLVPtr->V_glMaterialfv)
#define glMaterialiv (*OpenGLVPtr->V_glMaterialiv)
#define glGetMaterialfv (*OpenGLVPtr->V_glGetMaterialfv)
#define glGetMaterialiv (*OpenGLVPtr->V_glGetMaterialiv)
#define glColorMaterial (*OpenGLVPtr->V_glColorMaterial)
#define glPixelZoom (*OpenGLVPtr->V_glPixelZoom)
#define glPixelStoref (*OpenGLVPtr->V_glPixelStoref)
#define glPixelStorei (*OpenGLVPtr->V_glPixelStorei)
#define glPixelTransferf (*OpenGLVPtr->V_glPixelTransferf)
#define glPixelTransferi (*OpenGLVPtr->V_glPixelTransferi)
#define glPixelMapfv (*OpenGLVPtr->V_glPixelMapfv)
#define glPixelMapuiv (*OpenGLVPtr->V_glPixelMapuiv)
#define glPixelMapusv (*OpenGLVPtr->V_glPixelMapusv)
#define glGetPixelMapfv (*OpenGLVPtr->V_glGetPixelMapfv)
#define glGetPixelMapuiv (*OpenGLVPtr->V_glGetPixelMapuiv)
#define glGetPixelMapusv (*OpenGLVPtr->V_glGetPixelMapusv)
#define glBitmap (*OpenGLVPtr->V_glBitmap)
#define glReadPixels (*OpenGLVPtr->V_glReadPixels)
#define glDrawPixels (*OpenGLVPtr->V_glDrawPixels)
#define glCopyPixels (*OpenGLVPtr->V_glCopyPixels)
#define glStencilFunc (*OpenGLVPtr->V_glStencilFunc)
#define glStencilMask (*OpenGLVPtr->V_glStencilMask)
#define glStencilOp (*OpenGLVPtr->V_glStencilOp)
#define glClearStencil (*OpenGLVPtr->V_glClearStencil)
#define glTexGend (*OpenGLVPtr->V_glTexGend)
#define glTexGenf (*OpenGLVPtr->V_glTexGenf)
#define glTexGeni (*OpenGLVPtr->V_glTexGeni)
#define glTexGendv (*OpenGLVPtr->V_glTexGendv)
#define glTexGenfv (*OpenGLVPtr->V_glTexGenfv)
#define glTexGeniv (*OpenGLVPtr->V_glTexGeniv)
#define glGetTexGendv (*OpenGLVPtr->V_glGetTexGendv)
#define glGetTexGenfv (*OpenGLVPtr->V_glGetTexGenfv)
#define glGetTexGeniv (*OpenGLVPtr->V_glGetTexGeniv)
#define glTexEnvf (*OpenGLVPtr->V_glTexEnvf)
#define glTexEnvi (*OpenGLVPtr->V_glTexEnvi)
#define glTexEnvfv (*OpenGLVPtr->V_glTexEnvfv)
#define glTexEnviv (*OpenGLVPtr->V_glTexEnviv)
#define glGetTexEnvfv (*OpenGLVPtr->V_glGetTexEnvfv)
#define glGetTexEnviv (*OpenGLVPtr->V_glGetTexEnviv)
#define glTexParameterf (*OpenGLVPtr->V_glTexParameterf)
#define glTexParameteri (*OpenGLVPtr->V_glTexParameteri)
#define glTexParameterfv (*OpenGLVPtr->V_glTexParameterfv)
#define glTexParameteriv (*OpenGLVPtr->V_glTexParameteriv)
#define glGetTexParameterfv (*OpenGLVPtr->V_glGetTexParameterfv)
#define glGetTexParameteriv (*OpenGLVPtr->V_glGetTexParameteriv)
#define glGetTexLevelParameterfv (*OpenGLVPtr->V_glGetTexLevelParameterfv)
#define glGetTexLevelParameteriv (*OpenGLVPtr->V_glGetTexLevelParameteriv)
#define glTexImage1D (*OpenGLVPtr->V_glTexImage1D)
#define glTexImage2D (*OpenGLVPtr->V_glTexImage2D)
#define glGetTexImage (*OpenGLVPtr->V_glGetTexImage)
#define glMap1d (*OpenGLVPtr->V_glMap1d)
#define glMap1f (*OpenGLVPtr->V_glMap1f)
#define glMap2d (*OpenGLVPtr->V_glMap2d)
#define glMap2f (*OpenGLVPtr->V_glMap2f)
#define glGetMapdv (*OpenGLVPtr->V_glGetMapdv)
#define glGetMapfv (*OpenGLVPtr->V_glGetMapfv)
#define glGetMapiv (*OpenGLVPtr->V_glGetMapiv)
#define glEvalCoord1d (*OpenGLVPtr->V_glEvalCoord1d)
#define glEvalCoord1f (*OpenGLVPtr->V_glEvalCoord1f)
#define glEvalCoord1dv (*OpenGLVPtr->V_glEvalCoord1dv)
#define glEvalCoord1fv (*OpenGLVPtr->V_glEvalCoord1fv)
#define glEvalCoord2d (*OpenGLVPtr->V_glEvalCoord2d)
#define glEvalCoord2f (*OpenGLVPtr->V_glEvalCoord2f)
#define glEvalCoord2dv (*OpenGLVPtr->V_glEvalCoord2dv)
#define glEvalCoord2fv (*OpenGLVPtr->V_glEvalCoord2fv)
#define glMapGrid1d (*OpenGLVPtr->V_glMapGrid1d)
#define glMapGrid1f (*OpenGLVPtr->V_glMapGrid1f)
#define glMapGrid2d (*OpenGLVPtr->V_glMapGrid2d)
#define glMapGrid2f (*OpenGLVPtr->V_glMapGrid2f)
#define glEvalPoint1 (*OpenGLVPtr->V_glEvalPoint1)
#define glEvalPoint2 (*OpenGLVPtr->V_glEvalPoint2)
#define glEvalMesh1 (*OpenGLVPtr->V_glEvalMesh1)
#define glEvalMesh2 (*OpenGLVPtr->V_glEvalMesh2)
#define glFogf (*OpenGLVPtr->V_glFogf)
#define glFogi (*OpenGLVPtr->V_glFogi)
#define glFogfv (*OpenGLVPtr->V_glFogfv)
#define glFogiv (*OpenGLVPtr->V_glFogiv)
#define glFeedbackBuffer (*OpenGLVPtr->V_glFeedbackBuffer)
#define glPassThrough (*OpenGLVPtr->V_glPassThrough)
#define glSelectBuffer (*OpenGLVPtr->V_glSelectBuffer)
#define glInitNames (*OpenGLVPtr->V_glInitNames)
#define glLoadName (*OpenGLVPtr->V_glLoadName)
#define glPushName (*OpenGLVPtr->V_glPushName)
#define glPopName (*OpenGLVPtr->V_glPopName)
#define gluPerspective (*OpenGLVPtr->V_gluPerspective)
#define gluProject (*OpenGLVPtr->V_gluProject)
#define gluUnProject (*OpenGLVPtr->V_gluUnProject)
#define gluErrorString (*OpenGLVPtr->V_gluErrorString)
#define gluScaleImage (*OpenGLVPtr->V_gluScaleImage)
#define gluBuild1DMipmaps (*OpenGLVPtr->V_gluBuild1DMipmaps)
#define gluBuild2DMipmaps (*OpenGLVPtr->V_gluBuild2DMipmaps)
#define gluNewQuadric (*OpenGLVPtr->V_gluNewQuadric)
#define gluQuadricCallback (*OpenGLVPtr->V_gluQuadricCallback)
#define gluNewNurbsRenderer (*OpenGLVPtr->V_gluNewNurbsRenderer)
#define gluNewTess (*OpenGLVPtr->V_gluNewTess)
#define gluTessCallback (*OpenGLVPtr->V_gluTessCallback)
#define gluDeleteTess (*OpenGLVPtr->V_gluDeleteTess)
#define gluBeginPolygon (*OpenGLVPtr->V_gluBeginPolygon)
#define gluEndPolygon (*OpenGLVPtr->V_gluEndPolygon)
#define gluNextContour (*OpenGLVPtr->V_gluNextContour)
#define gluTessVertex (*OpenGLVPtr->V_gluTessVertex)
#define gluGetString (*OpenGLVPtr->V_gluGetString)
#define glXChooseVisual (*OpenGLVPtr->V_glXChooseVisual)
#define glXDestroyContext (*OpenGLVPtr->V_glXDestroyContext)
#define glXMakeCurrent (*OpenGLVPtr->V_glXMakeCurrent)
#define glXCreateGLXPixmap (*OpenGLVPtr->V_glXCreateGLXPixmap)
#define glXDestroyGLXPixmap (*OpenGLVPtr->V_glXDestroyGLXPixmap)
#define glXQueryExtension (*OpenGLVPtr->V_glXQueryExtension)
#define glXQueryVersion (*OpenGLVPtr->V_glXQueryVersion)
#define glXIsDirect (*OpenGLVPtr->V_glXIsDirect)
#define glXGetConfig (*OpenGLVPtr->V_glXGetConfig)
#define glXGetCurrentContext (*OpenGLVPtr->V_glXGetCurrentContext)
#define glXGetCurrentDrawable (*OpenGLVPtr->V_glXGetCurrentDrawable)
#define glXWaitGL (*OpenGLVPtr->V_glXWaitGL)
#define glXWaitX (*OpenGLVPtr->V_glXWaitX)
#define glXUseXFont (*OpenGLVPtr->V_glXUseXFont)
#endif
#endif
