#include "gl_local.h"
#include "gl_poly.h"


//#define POLY_DEBUG
#define SUBDIV_ERROR	0.2		// max deviation from splitting plane

// verts
int subdivNumVerts;							// number of vertexes in a new surface (find a way to make this static !!)
static vec3_t **psubdivVerts;				// pointer to a destination "vector pointer" array
static int subdivNumVerts2;					// number of NEW vertexes
static vec3_t subdivVerts[MAX_POLYVERTS];	// place for a NEW vertexes
// polys
static void *subdivPolyChain;
static poly_t *subdivPolys;


static int NewVert (float x, float y, float z)
{
	int		i;
	vec3_t	*v;

//	DebugPrintf ("  NewVert(%g, %g, %g)", x, y, z);
	for (i = 0; i < subdivNumVerts; i++)
	{
		v = psubdivVerts[i];
		if ((*v)[0] == x && (*v)[1] == y && (*v)[2] == z)
		{
//			DebugPrintf ("  -- already, %d\n", i);
			return i;	// already have this vertex
		}
	}
	if (subdivNumVerts >= MAX_POLYVERTS)
		Com_Error (ERR_DROP, "SubdividePlane: MAX_POLYVERTS hit");

	// alloc vertex
	v = &subdivVerts[subdivNumVerts2++];
	// "i" and "v" points to a place for new vertex
	(*v)[0] = x;
	(*v)[1] = y;
	(*v)[2] = z;
	psubdivVerts[i] = v;
	subdivNumVerts++;	// == i-1
//	DebugPrintf (" -- new, %d\n", i);
	return i;
}


static poly_t *NewPoly (int numVerts)
{
	poly_t	*p;

//	DebugPrintf ("NewPoly(%d)\n", numVerts);
	// alloc poly
	p = AllocChainBlock (subdivPolyChain, sizeof(poly_t) + 4 * numVerts);
	p->indexes = (int*)(p+1);
	p->numIndexes = 0;
#ifdef POLY_DEBUG
	p->maxIndexes = numVerts;
#endif
	p->next = NULL;

	return p;
}


static void AddPointToPoly (poly_t *poly, int index)
{
#ifdef POLY_DEBUG
	if (poly->numIndexes >= poly->maxIndexes)
		Com_Error (ERR_DROP, "AddPointToPoly: index error");
#endif
	poly->indexes[poly->numIndexes] = index;
	poly->numIndexes++;
}


static void SubdividePoly (poly_t *poly, poly_t *poly1, poly_t *poly2, int axis, float value, float delta)
{
	int		i, lastIndex, idx1, idx2;
	vec3_t	*v1, *v2;
	float	value1, value2;

//	DebugPrintf ("SubdividePoly: %d inds, axis = %d, value = %g, delta = %g\n", poly->numIndexes, axis, value, delta);
	lastIndex = poly->numIndexes - 1;
	if (lastIndex < 0)
	{
//		DebugPrintf ("...empty!\n");
		return;		// empty poly
	}

	value1 = value - delta;
	value2 = value + delta;

	idx2 = poly->indexes[0];
	v2 = psubdivVerts[idx2];
	for (i = 0; i <= lastIndex; i++)
	{
		int		side1, side2;

		// process next point
		v1 = v2;
		idx1 = idx2;
		idx2 = poly->indexes[i == lastIndex ? 0 : i + 1];
		v2 = psubdivVerts[idx2];

		// check point side
		if ((*v1)[axis] <= value1)			side1 = 1;
		else if ((*v1)[axis] >= value2)		side1 = 2;
		else								side1 = 3;
		// check next point side
		if ((*v2)[axis] <= value1)			side2 = 1;
		else if ((*v2)[axis] >= value2)		side2 = 2;
		else								side2 = 3;
		// add point to the corresponding poly
		if (side1 == 3 && side2 == 3)
		{
			// both points are on divider -- add point only to the one poly
			AddPointToPoly ((*v1)[axis] < (*v2)[axis] ? poly1 : poly2, idx1);
			// NOTE: can be error here (non-convex polygon)
		}
		else
		{
			// at least one of points not on divider
			if (side1 & 1)	AddPointToPoly (poly1, idx1);
			if (side1 & 2)	AddPointToPoly (poly2, idx1);
			// if points are on the different sides -- split line
			if (!(side1 & side2))
			{
				float	frac;
				vec3_t	mid;

				// calculate midpoint
				frac = (value - (*v1)[axis]) / ((*v2)[axis] - (*v1)[axis]);
				VectorSubtract ((*v2), (*v1), mid);
				VectorScale (mid, frac, mid);
				VectorAdd (mid, (*v1), mid);
				idx1 = NewVert (VECTOR_ARGS(mid));
				// add it to both polys
				AddPointToPoly (poly1, idx1);
				AddPointToPoly (poly2, idx1);
			}
		}
//		DebugPrintf ("  point #%d: (%g, %g, %g), side: %d\n",
//			poly->indexes[i], (*v1)[0], (*v1)[1], (*v1)[2], side1);
	}
/*	DebugPrintf ("...OK: %d and %d inds (", poly1->numIndexes, poly2->numIndexes);
	for (i = 0; i < poly1->numIndexes; i++)
		DebugPrintf (" %d ", poly1->indexes[i]);
	DebugPrintf (") and (");
	for (i = 0; i < poly2->numIndexes; i++)
		DebugPrintf (" %d ", poly2->indexes[i]);
	DebugPrintf (")\n");
*/
}


// In: *verts[numVerts]
// Out: updated numVerts and filled verts[old_numVerts+1..new_numVerts]
// Returns number of triangles in new surface
int SubdividePlane (vec3_t **verts, int numVerts, float tessSize)
{
	int		axis, i, numTris;
	poly_t	*poly, *firstPoly, *lastPoly;
	float	tessError;		// deviation from splitting plane

//	DebugPrintf ("SubdividePlane: %d verts, %g tessSize\n", numVerts, tessSize);
	/*------ initialization ---------*/
	psubdivVerts = verts;
	subdivNumVerts = numVerts;
	subdivNumVerts2 = 0;
	subdivPolyChain = CreateMemoryChain ();
	tessError = tessSize * SUBDIV_ERROR;

	/*---- generate initial poly ----*/
	firstPoly = poly = NewPoly (numVerts);
	for (i = 0; i < numVerts; i++)
		poly->indexes[i] = i;
	poly->numIndexes = numVerts;

	/*------- subdivide polys -------*/
	for (axis = 0; axis < 3; axis++)
	{
		poly = firstPoly;
		firstPoly = NULL;
		while (poly)
		{
			vec3_t	mins, maxs;
			int		numIndexes;
			float	value, min, max;
			poly_t	*workPoly, *poly1, *poly2;

//			DebugPrintf ("processing axis %d: %d indexes ...\n", axis, poly->numIndexes);
			numIndexes = poly->numIndexes;
			if (!numIndexes) continue;	// skip empty poly

			// calculate poly bounds
			ClearBounds (mins, maxs);
			for (i = 0; i < numIndexes; i++)
				AddPointToBounds (*psubdivVerts[poly->indexes[i]], mins, maxs);
//			DebugPrintf ("bounds: (%g, %g, %g) - (%g, %g, %g)\n", VECTOR_ARGS(mins), VECTOR_ARGS(maxs));

			// mins/maxs, aligned to tessSize grid and shifted to the poly center by tessSize
			min = Q_floor((mins[axis] + tessSize + tessError) / tessSize) * tessSize;
			max = Q_ceil((maxs[axis] - tessSize - tessError) / tessSize) * tessSize;
//			DebugPrintf ("... stepping from %g to %g with step %g\n", min, max, tessSize);

			// shred workPoly
			workPoly = poly;
			for (value = min; value <= max; value += tessSize)
			{
				numIndexes = workPoly->numIndexes;
				// alloc new polys
				poly1 = NewPoly (numIndexes+1);		// subdivision of convex polygon can produce up to numVerts+1 verts
				poly2 = NewPoly (numIndexes+1);
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
	numTris = 0;
	for (poly = firstPoly; poly; poly = poly->next)
		numTris += poly->numIndexes - 2;	// numTris = numVerts - 2
//	DebugPrintf ("SubdividePlane: OK\n--------------------------\n");

	return numTris;
}


void FreeSubdividedPlane (void)
{
	if (subdivPolyChain)
	{
		FreeMemoryChain (subdivPolyChain);
		subdivPolyChain = NULL;
	}
}


void GetSubdivideIndexes (int *pindex)
{
	poly_t	*poly;

	for (poly = subdivPolys; poly; poly = poly->next)
	{
		int		i;

		for (i = 0; i < poly->numIndexes - 2; i++)	// numTris; this will also reject polys with 2 vertexes
		{
			*pindex++ = poly->indexes[0];
			*pindex++ = poly->indexes[i+1];
			*pindex++ = poly->indexes[i+2];
		}
	}
}


/*-------------------------------------------------------*/

// Gets array of pointers to vectors, removes collinear points and returns new number of verts
int RemoveCollinearPoints (vec3_t **pverts, int numVerts)
{
	int		i, prevVert, validVerts;

	pverts[numVerts] = pverts[0];			// make a loop
	prevVert = 0;
	for (i = 1; i < numVerts; i++)
	{
		vec3_t	v1, v2;

		VectorSubtract ((*pverts[prevVert]), (*pverts[i]), v1);
		VectorSubtract ((*pverts[i]), (*pverts[i+1]), v2);
		VectorNormalize (v1);
		VectorNormalize (v2);
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
	validVerts = 0;
	for (i = 0; i < numVerts; i++)
	{
		if (pverts[i])
			pverts[validVerts++] = pverts[i];
	}
	return validVerts;
}


/*-------------------------------------------------------*/

float GetPolyArea (vec3_t **pverts, int numVerts)
{
	float	area;
	int		i;

	area = 0;
	for (i = 1; i < numVerts - 1; i++)
	{
		vec3_t	d1, d2, cross;

		VectorSubtract ((*pverts[0]), (*pverts[i]), d1);
		VectorSubtract ((*pverts[0]), (*pverts[i+1]), d2);
		CrossProduct (d1, d2, cross);
		area += VectorLength (cross) / 2;
	}
	return area;
}
