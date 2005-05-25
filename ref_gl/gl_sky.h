#ifndef __GL_SKY_INCLUDED__
#define __GL_SKY_INCLUDED__

#include "gl_backend.h"
#include "gl_model.h"


namespace OpenGLDrv {


#define SKY_FRUSTUM		1
#define SKY_SURF		2

void ClearSkyBox ();
void SetSkyRotate (float angle, const CVec3 &axis);
bool SkyVisible ();
void AddSkySurface (surfacePlanar_t *pl, const CVec3 &vieworg, byte flag);	//?? vieworg is always vp.view.origin
int TesselateSkySide (int side, bufVertex_t *verts, bufTexCoord_t *tex, float zFar);


} // namespace

#endif
