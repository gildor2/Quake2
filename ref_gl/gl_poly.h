#ifndef __GL_POLY_INCLUDED__
#define __GL_POLY_INCLUDED__


#define MAX_POLYVERTS	1024


typedef struct poly_s
{
	int		numIndexes;
	int		*indexes;
#ifdef POLY_DEBUG
	int		maxIndexes;
#endif
	struct poly_s *next;
} poly_t;

extern int subdivNumVerts;

int SubdividePlane (vec3_t **verts, int numVerts, float tessSize);
void FreeSubdividedPlane (void);
void GetSubdivideIndexes (int *pindex);

int RemoveCollinearPoints (vec3_t **pverts, int numVerts);
float GetPolyArea (vec3_t **pverts, int numVerts);


#endif
