#ifndef __GL_POLY_INCLUDED__
#define __GL_POLY_INCLUDED__

namespace OpenGLDrv {


#define MAX_POLYVERTS	1024


extern int subdivNumVerts;

int SubdividePlane(CVec3 **verts, int numVerts, float tessSize);
void FreeSubdividedPlane();
void GetSubdivideIndexes(int *pindex);

int RemoveCollinearPoints(CVec3 **pverts, int numVerts);
float GetPolyArea(CVec3 **pverts, int numVerts);


} // namespace

#endif
