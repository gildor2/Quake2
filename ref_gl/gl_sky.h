#ifndef __GL_SKY_INCLUDED__
#define __GL_SKY_INCLUDED__

#include "gl_backend.h"
#include "gl_model.h"


#define SKY_FRUSTUM		1
#define SKY_SURF		2

void GL_ClearSkyBox (void);
void GL_SetSkyRotate (float angle, vec3_t axis);
bool GL_SkyVisible (void);
void GL_AddSkySurface (surfacePlanar_t *pl, vec3_t vieworg, byte flag);
int GL_TesselateSkySide (int side, bufVertex_t *verts, bufTexCoord_t *tex, float zFar);


#endif
