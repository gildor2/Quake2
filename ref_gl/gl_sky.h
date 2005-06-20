#ifndef __GL_SKY_INCLUDED__
#define __GL_SKY_INCLUDED__

namespace OpenGLDrv {


#define SKY_FRUSTUM		1
#define SKY_SURF		2

void ClearSky ();
void AddSkySurface (surfacePlanar_t *pl, byte flag = SKY_SURF);
void DrawSky ();


} // namespace

#endif
