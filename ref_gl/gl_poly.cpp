#include "OpenGLDrv.h"
#include "gl_poly.h"


//?? may be moved outside OpenGLDrv -- math functions
namespace OpenGLDrv {


static CMemoryChain *subdivPolyChain;

#if MAX_DEBUG
#define POLY_DEBUG		1
#endif

struct poly_t
{
	int		numIndexes;
	int		*indexes;
#if POLY_DEBUG
	int		maxIndexes;
#endif
	poly_t	*next;
	void AddPoint (int index)
	{
#if POLY_DEBUG
		if (numIndexes >= maxIndexes)
			Com_DropError ("poly_t::AddPoint: index overflow");
#endif
		indexes[numIndexes] = index;
		numIndexes++;
	}
	poly_t (int numVerts)
//	:	numIndexes (0)
//	,	next (NULL)
	{
		// alloc poly
		indexes    = new (subdivPolyChain) int[numVerts];
#if POLY_DEBUG
		maxIndexes = numVerts;
#endif
	}
};


#define SUBDIV_ERROR	0.2					// max deviation from splitting plane


//?? can create class CVertexPool with method NewVert(), buffer (memory chain) and allocated verts

// verts
int subdivNumVerts;							// number of vertexes in a new surface (find a way to make this static !!)
static CVec3 **psubdivVerts;				// pointer to a destination "vector pointer" array
// polys
static poly_t *subdivPolys;


static int NewVert (const CVec3 &vec)
{
	int i;
	for (i = 0; i < subdivNumVerts; i++)
		if (*psubdivVerts[i] == vec) return i;	// already have this vertex
	if (subdivNumVerts >= MAX_POLYVERTS)	// prevent psubdivVerts[] array overflow
		Com_DropError ("SubdividePlane: MAX_POLYVERTS hit");

	// alloc vertex
	CVec3 *v = new (subdivPolyChain) CVec3;
	*v = vec;
	psubdivVerts[i] = v;
	subdivNumVerts++;						// == i-1
	return i;								// index of new vertex
}


static void SubdividePoly (poly_t *poly, poly_t *poly1, poly_t *poly2, int axis, float value, float delta)
{
	int		idx1, idx2;
	CVec3	*v1, *v2;

	int lastIndex = poly->numIndexes - 1;
	if (lastIndex < 0)
		return;		// empty poly

	float value1 = value - delta;
	float value2 = value + delta;

	idx2 = poly->indexes[0];
	v2   = psubdivVerts[idx2];
	for (int i = 0; i <= lastIndex; i++)
	{
		// find next point
		idx1 = idx2;
		v1   = v2;
		idx2 = poly->indexes[i == lastIndex ? 0 : i + 1];
		v2   = psubdivVerts[idx2];

		int side1, side2;
		// check point side
		if ((*v1)[axis] <= value1)		side1 = 1;
		else if ((*v1)[axis] >= value2)	side1 = 2;
		else							side1 = 3;
		// check next point side
		if ((*v2)[axis] <= value1)		side2 = 1;
		else if ((*v2)[axis] >= value2)	side2 = 2;
		else							side2 = 3;
		// add point to the corresponding poly
		if (side1 == 3 && side2 == 3)
		{
			// both points are on divider -- add point only to the one poly
			if ((*v1)[axis] < (*v2)[axis])
				poly1->AddPoint (idx1);
			else
				poly2->AddPoint (idx1);
			// NOTE: can be error (non-convex polygon) ??
		}
		else
		{
			// at least one of points not on divider
			if (side1 & 1)	poly1->AddPoint (idx1);
			if (side1 & 2)	poly2->AddPoint (idx1);
			// if points are on the different sides -- split line
			if (!(side1 & side2))
			{
				// calculate midpoint
				float frac = (value - (*v1)[axis]) / ((*v2)[axis] - (*v1)[axis]);
				CVec3 mid;
				Lerp (*v1, *v2, frac, mid);
				idx1 = NewVert (mid);
				// add it to both polys
				poly1->AddPoint (idx1);
				poly2->AddPoint (idx1);
			}
		}
	}
}


// In: *verts[numVerts]
// Out: updated numVerts and filled verts[old_numVerts+1..new_numVerts]
// Returns number of triangles in new surface
int SubdividePlane (CVec3 **verts, int numVerts, float tessSize)
{
	int		i;
	poly_t	*poly, *firstPoly, *lastPoly;

	/*------ initialization ---------*/
	psubdivVerts    = verts;
	subdivNumVerts  = numVerts;
	subdivPolyChain = new CMemoryChain;
	float tessError = tessSize * SUBDIV_ERROR;		// deviation from splitting plane

	/*---- generate initial poly ----*/
	firstPoly = poly = new (subdivPolyChain) poly_t (numVerts);
	for (i = 0; i < numVerts; i++)
		poly->indexes[i] = i;
	poly->numIndexes = numVerts;

	/*------- subdivide polys -------*/
	for (int axis = 0; axis < 3; axis++)
	{
		poly = firstPoly;
		firstPoly = NULL;
		while (poly)
		{
			if (!poly->numIndexes)
			{
				poly = poly->next;
				continue;		// skip empty poly
			}

			// calculate poly bounds
			CBox bounds;
			bounds.Clear ();
			for (i = 0; i < poly->numIndexes; i++)
				bounds.Expand (*psubdivVerts[poly->indexes[i]]);

			// mins/maxs, aligned to tessSize grid and shifted to the poly center by tessSize
			float min = appFloor((bounds.mins[axis] + tessSize + tessError) / tessSize) * tessSize;
			float max = appCeil ((bounds.maxs[axis] - tessSize - tessError) / tessSize) * tessSize;

			// shred workPoly
			poly_t *workPoly = poly;
			for (float value = min; value <= max; value += tessSize)
			{
				int numIndexes = workPoly->numIndexes;
				// alloc new polys
				poly_t *poly1 = new (subdivPolyChain) poly_t (numIndexes+1); // subdivision of convex polygon can produce up to numVerts+1 verts
				poly_t *poly2 = new (subdivPolyChain) poly_t (numIndexes+1);
				// add poly1 to chain
				if (!firstPoly)
					firstPoly = poly1;
				else
					lastPoly->next = poly1;
				lastPoly = poly1;
				// subdivide: split workPoly to poly1 and poly2
				SubdividePoly (workPoly, poly1, poly2, axis, value, tessError);
				// switch to poly2
				workPoly = poly2;
			}
			// add workPoly to chain (rest of the splitting poly)
			if (!firstPoly)
				firstPoly = workPoly;
			else
				lastPoly->next = workPoly;
			lastPoly = workPoly;

			poly = poly->next;
			workPoly->next = NULL;	// in a case when no subdivide loop (with workPoly) performed -- keep original poly
		}
	}
	subdivPolys = firstPoly;
	// calculate number of triangles in a resulting surface
	int numTris = 0;
	for (poly = firstPoly; poly; poly = poly->next)
		numTris += poly->numIndexes - 2;			// numTris = numVerts - 2

	return numTris;
}


void FreeSubdividedPlane ()
{
	if (subdivPolyChain)
	{
		delete subdivPolyChain;
		subdivPolyChain = NULL;
	}
}


void GetSubdivideIndexes (int *pindex)
{
	for (poly_t *poly = subdivPolys; poly; poly = poly->next)
	{
		for (int i = 0; i < poly->numIndexes - 2; i++)		// numTris; this will also reject polys with 2 vertexes
		{
			*pindex++ = poly->indexes[0];
			*pindex++ = poly->indexes[i+1];
			*pindex++ = poly->indexes[i+2];
		}
	}
}


/*-----------------------------------------------------------------------------
	In:		array of pointers to vectors
	Out:	removes collinear points in this array and returns new number of
			verts
-----------------------------------------------------------------------------*/
int RemoveCollinearPoints (CVec3 **pverts, int numVerts)
{
	int		i;

	pverts[numVerts] = pverts[0];							// make a loop
	int prevVert = 0;
	for (i = 1; i < numVerts; i++)
	{
		CVec3 v1, v2;

		VectorSubtract ((*pverts[prevVert]), (*pverts[i]),   v1);
		VectorSubtract ((*pverts[i]),        (*pverts[i+1]), v2);
		v1.Normalize ();
		v2.Normalize ();
#define CHECK_AXIS(i)	(fabs(v1[i]-v2[i]) < 0.001f)
		if (CHECK_AXIS(0) && CHECK_AXIS(1) && CHECK_AXIS(2))
		{	// remove point
			pverts[i] = NULL;
		}
		else
		{	// hold point
			prevVert = i;
		}
#undef CHECK_AXIS
	}
	/*----- compress pverts and compute new numVerts ------*/
	int validVerts = 0;
	for (i = 0; i < numVerts; i++)
	{
		if (pverts[i])
			pverts[validVerts++] = pverts[i];
	}
	return validVerts;
}


/*-------------------------------------------------------*/

float GetPolyArea (CVec3 **pverts, int numVerts)	//?? should be "const CVec3 **pverts" ?
{
	float area = 0;
	for (int i = 1; i < numVerts - 1; i++)
	{
		CVec3	d1, d2, cr;

		VectorSubtract ((*pverts[0]), (*pverts[i]),   d1);
		VectorSubtract ((*pverts[0]), (*pverts[i+1]), d2);
		cross (d1, d2, cr);
		// we use a property of cross product vector length = parallelogram area = 2 * triangle area
		area += VectorLength (cr) / 2;
	}
	return area;
}


} // namespace
