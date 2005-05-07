#ifndef __GL_SKY_INCLUDED__
#define __GL_SKY_INCLUDED__

#include "gl_backend.h"
#include "gl_model.h"


namespace OpenGLDrv {


#define SKY_FRUSTUM		1
#define SKY_SURF		2

void ClearSkyBox (void);
void SetSkyRotate (float angle, vec3_t axis);
bool SkyVisible (void);
void AddSkySurface (surfacePlanar_t *pl, vec3_t vieworg, byte flag);
int TesselateSkySide (int side, bufVertex_t *verts, bufTexCoord_t *tex, float zFar);


} // namespace

#endif
