/*


X3D Texturing3D Component

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
#include "../opengl/OpenGL_Utils.h"
#include "../opengl/Textures.h"
#include "../scenegraph/Component_Shape.h"
#include "../scenegraph/RenderFuncs.h"

/*
http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/texture3D.html

Texturing3D > Volumetric Image Formats
http://www.volumesoffun.com/voldat-format/
https://graphics.stanford.edu/data/voldata/
http://paulbourke.net/dataformats/pvl/
http://www.ncbi.nlm.nih.gov/pmc/articles/PMC2954506/#!po=59.5238
https://www.blender.org/manual/render/blender_render/textures/types/volume/voxel_data.html

http://paulbourke.net/dataformats/volumetric/
-Simplest 3d texture file if you write your own images
http://www.web3d.org/x3d/content/examples/Basic/VolumeRendering/
- Volumetric Rendering Samples use nrrd format
- http://teem.sourceforge.net/index.html
- http://teem.sourceforge.net/nrrd/index.html
-- Nrrd lib LGPL
https://msdn.microsoft.com/en-us/library/windows/desktop/bb205579(v=vs.85).aspx
- DDS Volume Texture Example

What is Texturing3D used for? a few links:
https://msdn.microsoft.com/en-us/library/windows/desktop/ff476906(v=vs.85).aspx
- difference between composed slices and volume image
http://docs.nvidia.com/gameworks/content/gameworkslibrary/graphicssamples/opengl_samples/texturearrayterrainsample.htm
- an application for rendering terrain



Fuzzy Design:
- load various ways into a buffer representing contiguous voxels, with width, height, depth available
- somewhere in geometry rendering - polyrep stuff? - give it 3D or 4D-homogenous texture coordinates 
	instead of regular 2D. Hint: look at per-vertex RGBA floats in polyrep?
- somewhere in sending geom to shader, send the 3D/4D texture coords (and/or Matrix?)
- in shader, detect if its 3D texture, and use 3D texture lookup 

Q. is Texture3D available in GLES2?
	https://www.khronos.org/opengles/sdk/docs/reference_cards/OpenGL-ES-2_0-Reference-card.pdf
	- at bottom shows texture2D etc sampler functions
	instead of  vec3 texture2D(sampler2D,coord2D) 
	it would be vec4 texture3D(sampler3D sampler, vec3 coord) ?

GLES2 texture3D: first edition doesn't have sampler3D/texture3D
https://www.khronos.org/registry/gles/extensions/OES/OES_texture_3D.txt
- there's a proposed extension
-Q.how to test for it at runtime?
-A. worst case: frag shader has a compile error:
-- if I put Texture3D uniform, and sampler3D(texture3d,vec3(coords2D,0.0)) then:
	- desktop windows opengl frag shader compiles
	x uwp windows (using angle over dx/hlsl) frag shader doesn't compile
	x android frag shader doesn't compile
http://stackoverflow.com/questions/14150941/webgl-glsl-emulate-texture3d
http://stackoverflow.com/questions/16938422/3d-texture-emulation-in-shader-subpixel-related
- hackers emulating with texture2D tiles and lerps
https://android.googlesource.com/platform/external/chromium_org/third_party/angle/+/0027fa9%5E!/
- looks like chromium's ANGLE^ emulates cube as 6 2Ds as of 2014
  ^(gles2 over DX/HLSL for webgl in windows chrome browser) 
Michalis: "Note that Castle Game Engine does not support 3D textures in
	OpenGLES now (so we don't use OES_texture_3D), but only on desktop
	OpenGL now (available through EXT_texture3D, or in standard OpenGL >=
	1.2). But that's simply due to my lack of time to patch necessary
	things:) From the first glance, using OES_texture_3D to achieve the
	same functionality on OpenGLES should be easy."
	https://github.com/castle-engine/demo-models > texturing_advanced/tex3d_*

Example use: if you have 3 images for terrain - white for mountain peaks, brown for mountain sides, green for valleys
- then combine them into a nxmx3 volume image?
- then you can use the terrain height to compute an R value ?
- cube sampler should in theory interpolate between the 3 layers?

Design:
Texture coordinates:
I think the geometry vertices, scaled into the range 0-1 on each axis, 
could/should serve as default 3D texture coordinates.
algorithm:
a) interpolate through varying the vertex xyz you want as usual
b) transform xyz into 0-1 range 
	-would need to know bounding box, or have pre-computed, or assume -1 to 1, or in textureTransform3D
Option: convert xyz vertex to 0-1 range in vertex shader, 
	and do varying interpolation in 0-1 range for varying textureCoords3D aka tc3d
c) sample the texture
A. if have texture3D/sampler3D/EXT_texture3D/OES_texture3D: 
	sample directly tc3d.xyz (0-1): frag = sampler3D(texture3D, vec3 tc3d.xyz)
B. else emulate texture3D/sampler3D with 2D tiled texture:
	single texture2D: sample the texture at xy[z] on the 2D image plane, z rounded 'above' and 'below'
	Q. how many z layers, and does it have to be same as rows & cols?
	nx,ny,nz = size of image in pixels
	nz == number of z levels
	nzr = sqrt(nz) so if 256 z levels, it would be sqrt(256) = 16. 16x16 tiles in 2D plane
	x,y,z - 0-1 range 3D image texture coords from tc3d.xyz
	vec2 xyi = xy[i] = f(xyz) = [x/nzr + (i / nzr), y/nzr + (i % nzr)]
	vec2 xyj = xy[j] ... "
	where
	 i = floor(z*nz)
	 j = ceil(z*nz)
	fxyi = sampler2D(texture2D,xyi)
	fxyj = sampler2D(texture2D,xyj)
	lerp between layers xy[i],xy[j]: frag = lerp(fxyi,fxyj,(z*nz-i)/(j-i))
	Note: i,j don't need to be ints, nothing above needs to be int, can all be float
	Option1: strip of tiles - instead of maintaining square 2D texture, 
		make strip texture nx x (ny * nz)
		then no re-arranging needed when sending uniform (parsed into strip)
		and xyi = [x, y + i*nz]
		    xyj = [x, y + j*nz]
		hack test on windows, uwp, and androidndk/nexus: they can all handle the oblong texture
	Option2: lerp vs floor effects: 
		lerp - smooth transition between layers, good for 3-layer valley, hillside, mountain peak 
		floor - (frag = fxyi): gives abrupt change between layers, good for brick effects
	Option3: compute 2D-ized texture coordinates on CPU, and use ordinary texture2D sampling (no lerp)
		problem: geometry node doesn't know if shape>appearance has texture3D, so would need to pre-compute like flat coords

Sept 13, 2016
	.web3dit image file format developeed which includes volume L,LA,RGB,RGBA and int/float, Up/Down options

4 scenarios:
	1. GL has texture3D/EXT_texture3D/OES_texture3D
	2. GL no texture3D - emulate
	A. 3D image: source imagery is i) 3D image or ii) composed image with image layers all same size
	B. 2D layers: source imagery is composed image with z < 7 layers and layers can be different sizes
		1					2
	A	vol texture3D		tiled texture2D	
	B	multi texure2D		multi texture2D

*/

int isTex3D(struct X3D_Node *node){
	int ret = 0;
	if(node)
	switch(node->_nodeType){
		case NODE_PixelTexture3D:
		case NODE_ComposedTexture3D:
		case NODE_ImageTexture3D:
			ret = TRUE; break;
		default:
			ret = FALSE;break;
	}
	return ret;
}

void render_PixelTexture3D (struct X3D_PixelTexture3D *node) {
	loadTextureNode(X3D_NODE(node),NULL);
	gglobal()->RenderFuncs.textureStackTop=1; /* not multitexture - should have saved to boundTextureStack[0] */
}
void move_texture_to_opengl(textureTableIndexStruct_s* me);
void render_ImageTexture3D (struct X3D_ImageTexture3D *node) {
	/* printf ("render_ImageTexture, global Transparency %f\n",getAppearanceProperties()->transparency); */
	loadTextureNode(X3D_NODE(node),NULL);
	gglobal()->RenderFuncs.textureStackTop=1; /* not multitexture - should have saved to boundTextureStack[0] */
}
textureTableIndexStruct_s *getTableTableFromTextureNode(struct X3D_Node *textureNode);
void render_ComposedTexture3D (struct X3D_ComposedTexture3D *node) {
	/* printf ("render_ComposedTexture, global Transparency %f\n",getAppearanceProperties()->transparency); */
	if(node && node->_nodeType == NODE_ComposedTexture3D){
		int i, allLoaded; //ntextures, 
		struct Multi_Node *tex = &node->texture;
		//Sep 14, 2016 we assume all ComposedTexture3D are 'LAYERED' TEX3D_LAYER_SHADER / TEX3DLAY
		if(0){
			//could switch from layered to volume, if layers all same size?
			//somewhere around here we could detect if all images are loaded,
			// and if so, check/test:
			// A. do they all have the same xy size? if same size,
			//   then they could be converted to single volume texture
			//   and i) texture3D or ii) tiled texture2D
			//   and set shader flag TEX3D_VOLUME_SHADER / TEX3D, TEX3DVOL
			// B. if different sizes, and z < 7, then treat like multi-texture:
			//   ii) do sampler2D on either size of z
			//   and set shader flag TEX3D_LAYER_SHADER / TEX3D, TEX3DLAY
			// either way if ii)  we have 2 values in frag shader: take floor, ceil or lerp between
			//
			int nx, ny, nfound, nsamesize;
			nfound = nsamesize = 0;
			for(i=0;i<tex->n;i++){
				if(tex->p[i] ){
					textureTableIndexStruct_s *tti;
					tti = getTableTableFromTextureNode(tex->p[i]);
					if(tti){
						if(tti->status == TEX_LOADED){
							nfound++;
							if(nfound == 1){
								nx = tti->x; ny = tti->y;
								nsamesize = 1;
							}else{
								if(tti->x == nx && tti->y == ny) nsamesize++;
							}
						}
					}
				}
			}
			if(nsamesize == nfound && nfound == tex->n){
				//all 2D textures are loadded and the same size
				// -can combine into single volume texture
				printf("ComposedTexture3D all same size textures n %d x %d y %d\n",tex->n,nx,ny);
				//COMBINE?
				//TEX3DVOL
			}else{
				//not all loaded (so can't trust all same size) or not all same size
				//so we'll do up to 6 'layers' or texture2Ds, and lerp between floor/ceil in the shader 
				printf("ComposedTexture3D not all same size textures \n");
				//TEX3DLAY
			}
		}
		allLoaded = TRUE;
       	gglobal()->RenderFuncs.textureStackTop = 0;
		for(i=0;i<min(tex->n,MAX_MULTITEXTURE);i++){
			//doing layers, we can only handle up to MAX_MULTITEXTURE textures in the shader
			//gglobal()->RenderFuncs.textureStackTop = ntextures; /* not multitexture - should have saved to boundTextureStack[0] */
			loadTextureNode(X3D_NODE(tex->p[i]),NULL);
			allLoaded = allLoaded && getTableTableFromTextureNode(X3D_NODE(tex->p[i]))->status >= TEX_LOADED;
        	gglobal()->RenderFuncs.textureStackTop++;
			//loadMultiTexture(node);
		}
		//move_texture_to_opengl(getTableTableFromTextureNode(X3D_NODE(node))); //load composed texture properties

		if(allLoaded){
			textureTableIndexStruct_s *tti;
			tti = getTableTableFromTextureNode(X3D_NODE(node));
			tti->status = max(TEX_NEEDSBINDING, tti->status);
			loadTextureNode(X3D_NODE(node),NULL);
		}
	}
}

//void loadImageTexture3D(struct X3D_ImageTexture3D *node){
//	//we don't seem to call or need this
//}