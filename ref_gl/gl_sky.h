#ifndef __GL_SKY_INCLUDED__
#define __GL_SKY_INCLUDED__

#include "gl_backend.h"
#include "gl_model.h"


void GL_ClearSkyBox (void);
void GL_ShowWholeSky (void);
void GL_AddSkySurface (surfacePlanar_t *pl, vec3_t vieworg);
int GL_TesselateSkySide (int side, bufVertex_t *verts, bufTexCoord_t *tex, float zFar);


#endif
