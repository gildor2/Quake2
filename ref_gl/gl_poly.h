#ifndef __GL_POLY_INCLUDED__
#define __GL_POLY_INCLUDED__

namespace OpenGLDrv {


#define MAX_POLYVERTS	1024


struct poly_t
{
	int		numIndexes;
	int		*indexes;
#ifdef POLY_DEBUG
	int		maxIndexes;
#endif
	poly_t	*next;
};

extern int subdivNumVerts;

int SubdividePlane (vec3_t **verts, int numVerts, float tessSize);
void FreeSubdividedPlane (void);
void GetSubdivideIndexes (int *pindex);

int RemoveCollinearPoints (vec3_t **pverts, int numVerts);
float GetPolyArea (vec3_t **pverts, int numVerts);


} // namespace

#endif
