#include "qcommon.h"
#include "MapBrush.h"


//#define SPLIT_DEBUG		1

// allocated and released outside
CMemoryChain *CBrush::mem;


// Create brush from bounds
CBrush::CBrush (const CBox &Bounds)
{
	cplane_t *p   = new (mem) cplane_t[6];
	CVec3 *vec    = new (mem) CVec3[8];
	CBrushSide *s = new (mem) CBrushSide[6];
	CBrushVert *v = new (mem) CBrushVert[6*4];
	int i;
/*
	                D       F
	                ^      /
	         6+--------------+7
	         /.             /|
	        / .            / |             Y (axis 1)
	       /  .           /  |             A
	      /   .          /   |             |
	     +--------------+    |             |   / Z (axis 2)
	    2|    .        3|    |             |  /
	     |    .         |    |             | /
	 A<  |   4.         |   5|  >B         |/
	     |    ..........|....+             +--------> X (axis 0)
	     |   .          |   /
	     |  .           |  /
	     | .            | /           0-8 - vecs (CVec3)
	    0|.            1|/            A-F - sides (cplane_t, CBrushSide)
	     +--------------+

	     /      V
	    E       C
*/

	// init planes
	for (i = 0; i < 6; i++)
	{
		p[i].normal[i>>1] = (i & 1) ? 1 : -1;
		p[i].dist = (i & 1) ? Bounds.maxs[i>>1] : -Bounds.mins[i>>1];	// note: +maxs[] and -mins[] (because normal[]==-1)
		p[i].SetType ();
		p[i].SetSignbits ();		// unused, but ...
	}
	// init vecs
	for (i = 0; i < 8; i++)
		for (int j = 0; j < 3; j++)
			vec[i][j] = ((i>>j) & 1) ? Bounds.maxs[j] : Bounds.mins[j];
	// init BrushVerts
	for (i = 0; i < 4*6; i++)
	{
		static const byte vert2vec[] = {
			0,4,6,2,	1,3,7,5,	0,1,5,4,
			2,6,7,3,	0,2,3,1,	5,7,6,4
		};
		if ((i+1) & 3)
			v[i].next = &v[i+1];	// not for 3,7,11,15
		v[i].v = &vec[vert2vec[i]];
	}
	// init sides
	for (i = 0; i < 6; i++)
	{
		if (i < 5)
			s[i].next = &s[i+1];	// last have no "next"
		s[i].plane    = &p[i];		// 1st plane for 1st side, 2nd ...
		s[i].verts    = &v[i*4];	// 4 for 1st side, 4 for 2nd side ...
//		s[i].back     = false;		// our generated planes are "front"
	}
	// init brush itself
	sides    = s;
	contents = CONTENTS_SOLID;		//?? do outside?
}


#define EPSILON			0.01f
#define MAX_SIDE_VERTS	1024


class CAllocVertexPool
{
private:
	CVec3 *alloced[MAX_SIDE_VERTS];
	int numAllocedVecs;
public:
	CAllocVertexPool ()
	:	numAllocedVecs (0)
	{}
	CVec3 *GetVec (const CVec3 &v)
	{
		// find same (similar) vec in already allocated
		for (int i = 0; i < numAllocedVecs; i++)
		{
			CVec3 &v2 = *alloced[i];
			if (fabs (v2[0] - v[0]) < EPSILON &&
				fabs (v2[1] - v[1]) < EPSILON &&
				fabs (v2[2] - v[2]) < EPSILON)
			return &v2;
		}
		// alloc new vec
		CVec3 *v2 = new (CBrush::mem) CVec3;
		*v2 = v;
		if (numAllocedVecs < MAX_SIDE_VERTS)
			alloced[numAllocedVecs++] = v2;
		return v2;
	}
};

static CAllocVertexPool *vecPool;

// buffer for holding pointers to verts, placed on splitting plane
static CVec3 **planeVecs;	// [MAX_SIDE_VERTS]
static int numPlaneVecs;

// In:	poly, plane
// Out:	front, back
//		returns 'false' when poly placed on plane (cannot divide)
// `poly' will be damaged (altered)
// When poly on a single side of plane, it will not be modified (but returned as 'front' or 'back')
static bool SplitPoly (CBrushVert *poly, cplane_t *plane, CBrushVert* &front, CBrushVert* &back)
{
	guardSlow(SplitPoly);

	assert(vecPool);

	front = back = NULL;
	if (!poly) return true;

	float dist[MAX_SIDE_VERTS];
	byte  side[MAX_SIDE_VERTS];
	int   numVerts = 0;
	CBrushVert *v;
	int i;

	// check whole poly side
	int sides = 0;									// OR of all vertex sides
	int numOnPlane = 0;								// number of verts, placed on plane
	for (v = poly; v; v = v->next)
	{
		if (numVerts >= MAX_SIDE_VERTS)
			Com_DropError ("SplitPoly: MAX_SIDE_VERTS");
		float d = plane->DistanceTo (*v->v);
		byte s = (d >  EPSILON) ? 0 : (				// front
				 (d < -EPSILON) ? 1 :				// back
				 2 );								// both
		dist[numVerts] = d;
		side[numVerts] = s;
		sides |= 1 << s;
		numVerts++;
		if (s == 2) numOnPlane++;
	}
	if (sides == 4)									// poly placed on plane
	{
		front = back = NULL;
		return false;
	}
	if (sides == 5 || sides == 6)					// touching plane, but not crossing plane (7)
	{
		assert(numOnPlane <= 2);
		// when plane crossing brush on edge, surface will be touched in 2 points;
		// note: there will be 2 touched surfaces at the same points, so, we should
		// remember only one of them (either surface, placed on front side of plane,
		// or on back side of plane)
		if (sides == 5 && numOnPlane == 2)			// use one side; ignore when touched 1 vertex only
		{
			for (i = 0, v = poly; v; v = v->next, i++)
			{
				if (side[i] == 2)
					planeVecs[numPlaneVecs++] = v->v;
			}
		}
		sides &= 3;									// clear "on plane" flag
	}
	if (sides == 1)									// front or front+both (1+4)
	{
		front = poly;
		back  = NULL;
		return true;
	}
	if (sides == 2)									// back or back+both (2+4)
	{
		front = NULL;
		back  = poly;
		return true;
	}
	// here: sides = 3 (both sides) or 7 (all cases)

	CBrushVert *sidesFirst[2], *sidesLast[2];
	sidesFirst[0] = sidesFirst[1] = sidesLast[0] = sidesLast[1] = NULL;

#if SPLIT_DEBUG
	appPrintf ("split by {%g %g %g} %g:\n", VECTOR_ARG(plane->normal), plane->dist);
	int ii;
	for (ii = 0, v = poly; v; v = v->next, ii++)
		appPrintf ("... %g %g %g = %d\n", VECTOR_ARG((*v->v)), side[ii]);
#endif

	// split poly
	CBrushVert *next;
	for (i = 0, v = poly; v; v = next, i++)
	{
		// get next element: it may be altered during this iteration
		next = v->next;

		// prepare current segment
		CVec3 &v1   = *v->v;
		float dist1 = dist[i];
		int   side1 = side[i];

		CVec3 &v2   = next ? *next->v  : *poly->v;
		float dist2 = next ? dist[i+1] : dist[0];
		int   side2 = next ? side[i+1] : side[0];

#define ADD_TO_SIDE(v,n)	\
	{						\
		if (!sidesFirst[n]) sidesFirst[n] = v;		\
		else				sidesLast[n]->next = v;	\
		sidesLast[n] = v;	\
		v->next = NULL;		\
	}
		/*----------------- Possible cases: -------------------
		 *  point A/point B
		 *	0/0, 1/1 - on the same side			-- add A side
		 *	0/1, 1/0 - on a different sides		-- create midpoint, add A to side, mid to both
		 *	0/2, 1/2 - 1st point on a side, 2nd - on a plane -- add A to side (B - in next iteration)
		 *	2/0, 2/1 - 1st point on a plane, 2nd - on a side -- add A to both
		 *	2/2		 - both points on a plane	-- add to both sides
		 * So:  - when 0/1 or 1/0			=> A to side + midpoint to both
		 *		- when 0/0, 1/1, 0/2, 1/2	=> A to side
		 *		- when 2/0, 2/1, 2/2		=> A to both
		 *---------------------------------------------------*/
		CBrushVert *clone;
		if (side1 != 2)
		{
			// 0/X, 1/X
			ADD_TO_SIDE(v,side1);
			if ((side1 == 0 && side2 == 1) || (side1 == 1 && side2 == 0))
			{
				// 0/1, 1/0
				// find middle point
				ADD_TO_SIDE(v,side1);
				// create midpoint, placed on a plane
				float frac = dist1 / (dist1 - dist2);
				CVec3 m;
				Lerp (v1, v2, frac, m);
				CVec3 *mid = vecPool->GetVec (m);
				// add to both sides
				clone = new (CBrush::mem) CBrushVert;
				clone->v = mid;
				ADD_TO_SIDE(clone,0);
				clone = new (CBrush::mem) CBrushVert;
				clone->v = mid;
				ADD_TO_SIDE(clone,1);
				// remember in planeVecs[]
				planeVecs[numPlaneVecs++] = mid;
#if SPLIT_DEBUG
				appPrintf ("... new: %g %g %g\n", VECTOR_ARG(m));
#endif
			}
		}
		else
		{
			// 2/X
			// point placed on both sides
			clone = new (CBrush::mem) CBrushVert;
			clone->v = v->v;
			// add to both sides
			ADD_TO_SIDE(v,0);
			ADD_TO_SIDE(clone,1);
			// remember in planeVecs[]
			planeVecs[numPlaneVecs++] = v->v;
#if SPLIT_DEBUG
			appPrintf ("... on-plane: %g %g %g\n", VECTOR_ARG((*v->v)));
#endif
		}
	}

#undef ADD_TO_SIDE
	front = sidesFirst[0];
	back  = sidesFirst[1];
	return true;

	unguardSlow;
}


// should remember on-plane verts from SplitPoly() (new verts+old on-plane verts), analyze them and
// combine in CBrush::Split() into new side

CBrush *CBrush::Split (cplane_t *plane)
{
	guard(CBrush::Split);

	// prepare some global buffers
	CAllocVertexPool VertPool;
	CVec3 *newPoly[MAX_SIDE_VERTS];
	vecPool      = &VertPool;
	planeVecs    = newPoly;
	numPlaneVecs = 0;

	CBrushSide *sidesFirst[2], *sidesLast[2];
	sidesFirst[0] = sidesFirst[1] = sidesLast[0] = sidesLast[1] = NULL;

	CBrushSide *next;
	for (CBrushSide *s = sides; s; s = next)
	{
		next = s->next;

		// check for available space in planeVecs[] array
		if (numPlaneVecs >= MAX_SIDE_VERTS-2)
			Com_DropError ("CBrush::Split(): intersection is too complex");

		CBrushVert *front, *back;
		if (!SplitPoly (s->verts, plane, front, back))
		{
#if SPLIT_DEBUG
			if (sidesFirst[0] && sidesFirst[1])
				Com_DropError ("CBrush::Split(): non-convex volume detected");
#endif
			//?? add support for brush, touched by plane (but: this is incorrect BSP)
			vecPool = NULL;
			return NULL;	// brush is not split: one of its sides touching plane
		}

#define ADD_TO_BRUSH(b,n)	\
	{						\
		if (!sidesFirst[n]) sidesFirst[n] = b;		\
		else				sidesLast[n]->next = b;	\
		sidesLast[n] = b;	\
		b->next = NULL;		\
	}

		assert(front==s->verts || back==s->verts);

		if (!front || !back)
		{
			// surface placed on a single side of plane
			if (front)
				ADD_TO_BRUSH(s,0)
			else
				ADD_TO_BRUSH(s,1);
		}
		else
		{
			// here: front!=NULL & back!=NULL
			s->verts = front;
			ADD_TO_BRUSH(s,0);
			// create new BrushSide, which is placed on the same plane
			CBrushSide *clone = new (CBrush::mem) CBrushSide;
			clone->verts = back;
			clone->plane = s->plane;
			clone->back  = s->back;
			ADD_TO_BRUSH(clone,1);
		}
	}
	// create one more side on 'plane', if (sidesFirst[0] && sidesFirst[1])
	// each splitted brush side will produce 2 verts for new side
	if (numPlaneVecs & 1)
		Com_DropError ("CBrush::Split(): odd numPlaneVecs");
#if MAX_DEBUG
	if (numPlaneVecs < 6)
		Com_DropError ("split: numPlaneVecs = %d\n", numPlaneVecs);
#endif
	if (numPlaneVecs)				// else - brush not splitted ...
	{
		int i;

#if SPLIT_DEBUG
		// check distance from all planeVecs[] to plane (should be 0)
		for (i = 0; i < numPlaneVecs; i++)
		{
			float f = plane->DistanceTo (*planeVecs[i]);
			if (f < -EPSILON || f > EPSILON)
				Com_DropError ("split: dist = %g", f);
			appPrintf("%g %g %g: %g\n",VECTOR_ARG((*planeVecs[i])), f);
		}
#endif

		// NOTE: Exchange() will work with pointers to CVec3, not with CVec3 itself
		for (i = 0; i < numPlaneVecs-2; i += 2)
		{
			CVec3 *v1 = planeVecs[i];
			CVec3 *v2 = planeVecs[i+1];
			bool found = false;
			int j;
			bool swapI = false, swapJ = false;
			for (j = i+2; j < numPlaneVecs; j += 2)
			{
				CVec3 *v3 = planeVecs[j];
				CVec3 *v4 = planeVecs[j+1];
				// if v1-v2 and v3-v4 connected, make v2==v3 (v1 and v4 are different points)
				// so, later, 2 connected will be adjancent in planeVecs[] array
				if (v2 == v3)
				{
					found = true;
					break;
				}
				if (v1 == v3)
				{
					swapI = true;
					found = true;
					break;
				}
				if (v2 == v4)
				{
					swapJ = true;
					found = true;
					break;
				}
				if (v1 == v4)
				{
					swapI = swapJ = true;
					found = true;
					break;
				}
			}
			if (!found)
			{
#if SPLIT_DEBUG
				for (int i = 0; i < numPlaneVecs; i += 2)
					appPrintf(S_RED"%d  {%g %g %g}  {%g %g %g}\n",i,VECTOR_ARG((*planeVecs[i])),VECTOR_ARG((*planeVecs[i+1])));
#endif
				Com_DropError ("CBrush::Split(): cannot build poly");
			}
			// perform pointer swapping
			if (swapI) Exchange (planeVecs[i], planeVecs[i+1]);
			if (swapJ) Exchange (planeVecs[j], planeVecs[j+1]);
			// here: found connected verts, j==index
			// now: make 'j'-vecs to come directly after i
			if (j != i+2)
			{
				Exchange (planeVecs[j],   planeVecs[i+2]);
				Exchange (planeVecs[j+1], planeVecs[i+3]);
			}
		}
		// here: vec[0] should be equal to vec[last]
		if (*planeVecs[0] != *planeVecs[numPlaneVecs-1])
			Com_DropError ("CBrush::Split(): no loop in poly");

		// build 2 new sides
		CBrushSide *s1 = new (CBrush::mem) CBrushSide;
		CBrushSide *s2 = new (CBrush::mem) CBrushSide;
		for (i = 0; i < numPlaneVecs; i += 2)
		{
			// create verts for both sides and insert them
			CBrushVert *v;
			v = new (CBrush::mem) CBrushVert;
			v->v      = planeVecs[i];
			v->next   = s1->verts;
			s1->verts = v;
			v = new (CBrush::mem) CBrushVert;
			v->v      = planeVecs[i];
			v->next   = s2->verts;
			s2->verts = v;
		}
		// both sides are on the same plane, but on opposite sides
		// front brush will use back side of splitting plane, because
		// plane normal should point outside the brush volume
		s1->plane = s2->plane = plane;
		s1->back = true;
		ADD_TO_BRUSH(s1,0);
		ADD_TO_BRUSH(s2,1);
	}

#undef ADD_TO_BRUSH
	if (!sidesFirst[0] || !sidesFirst[1])	// bad BSP ?
		Com_DropError ("CBrush::Split(): plane does not intersects brush volume");

	sides = sidesFirst[0];

	CBrush *backBrush = NULL;
	if (sidesFirst[1])
	{
		backBrush = new (CBrush::mem) CBrush;
		backBrush->sides = sidesFirst[1];
	}

#if SPLIT_DEBUG
	int ns = 0;
	for (s = sides; s; s = s->next)
	{
		ns++;
		int n = 0;
		for (CBrushVert *v = s->verts; v; v = v->next)
			n++;
		if (n < 3) Com_DropError ("nv<3");
	}
	if (ns < 3) Com_DropError ("ns<3");
#endif

	vecPool = NULL;
	return backBrush;

	unguard;
}


void CBrush::AddBevels (cplane_t* (*GetPlane)(const CVec3&, float))
{
	guard(CBrush::AddBevels);

	for (CBrushSide *s1 = sides; s1; s1 = s1->next)
	{
		// find all adjancent sides
		for (CBrushSide *s2 = s1->next; s2; s2 = s2->next)
		{
			// find 1 common vert
			CVec3 *vec = NULL;
			for (CBrushVert *v1 = s1->verts; v1; v1 = v1->next)
				for (CBrushVert *v2 = s2->verts; v2; v2 = v2->next)
					if (v1->v == v2->v)		// compare pointers
					{
						vec = v1->v;
						break;
					}
			if (!vec) continue;
			// found: should check requirement of new side creation
			CVec3 norm1 = s1->plane->normal;
			if (s1->back) norm1.Negate ();
			CVec3 norm2 = s2->plane->normal;
			if (s2->back) norm2.Negate ();
			CVec3 norm3;
			VectorAdd (norm1, norm2, norm3);
			float len = dot (norm3, norm3);
			// when norm1 looks almost to norm2 direction, angle between them will be < 90grad,
			// and dot(norm3,norm3) will be >2 (when angle is 90grad, sum of two igentity vecs
			// will produce vec of len sqrt(2); dot() is square of len ==> 2); otherwise, dot()
			// will be 0 <= dot < 2
			if (len >= 2) continue;			// continue to next side - angle between this sides as more than 90
											// (angle between normals is less than 90)
			if (len == 0) continue;			// two opposite planes ... should not happens in BSP
			// create new side
			norm3.Normalize ();
			float dist = dot (norm3, *vec);
			CBrushSide *newSide = new (CBrush::mem) CBrushSide;
			newSide->plane = GetPlane (norm3, dist);
			// insert into list
			newSide->next  = sides;
			sides = newSide;
			// no verts for this side
		}
	}

	unguard;
}
