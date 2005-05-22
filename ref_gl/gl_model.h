#ifndef __GL_MODEL_INCLUDED__
#define __GL_MODEL_INCLUDED__


#include "gl_frontend.h"
#include "gl_light.h"


namespace OpenGLDrv {


/*-----------------------------------------------------------------------------
	Surfaces
-----------------------------------------------------------------------------*/

// Vertex without normal (for planar surface)
struct vertex_t
{
	vec3_t	xyz;				// vertex itself
	float	st[2], lm[2];		// texture coordinates (TCGEN_TEXTURE/TCGEN_LIGHTMAP)
	float	lm2[2];				// for vertex color calculation: coords in surface lightmap rect (non-normalized)
	float	pos[2];				// 2D position in plane axis
	color_t	c;					// vertex color
};

// Vertex with stored normal
struct vertexNormal_t
{
	vec3_t	xyz;
	float	st[2], lm[2];
	color_t	c;
	vec3_t	normal;
};

struct vertexPoly_t
{
	vec3_t	xyz;
	float	st[2];
	color_t	c;
};

struct vertexMd3_t	//?? == dMd3XyzNormal_t (rename one of types? derive struc with no new fields?)
{
	short	xyz[3];
	short	normal;
};

typedef enum
{
	SURFACE_PLANAR,				// surfacePlanar_t
	SURFACE_TRISURF,			// surfaceTrisurf_t (unused??)
	SURFACE_MD3,				// surfaceMd3_t
	SURFACE_POLY,				// surfacePoly_t
	SURFACE_PARTICLE,			// particle_t
	SURFACE_ENTITY				// refEntity_t
} surfaceType_t;

// Planar surface: same normal for all vertexes
class inlineModel_t;			// forward

class surfaceBase_t
{
public:
	int		type;				//?? eliminate
	shader_t *shader;			// ignored for frame models
	int		frame;				// ignored for frame models
	int		dlightFrame;
	unsigned dlightMask;
	inlineModel_t *owner;
	virtual void Tesselate () = NULL;
};

class surfacePlanar_t : public surfaceBase_t
{
public:
	inline surfacePlanar_t () { type = SURFACE_PLANAR; };
	CBox	bounds;
	cplane_t plane;
	vec3_t	axis[2];			// 2 orthogonal identity vectors from surface
	float	mins2[2], maxs2[2];	// 2D-bounds (in "axis" coordinate system)
	int		numVerts, numIndexes;
	vertex_t *verts;
	int		*indexes;
	// Q2/HL dynamic lightmap (NULL for Q3 ??)
	dynamicLightmap_t *lightmap;
	surfLight_t *light;
	surfDlight_t *dlights;
	virtual void Tesselate ();
};

//!! warning: if shader.fast -- use vertex_t instead of vertexNormal_t (normals are not required for fast shader)
// Triangular surface: non-planar, every vertex have its own normal vector
class surfaceTrisurf_t : public surfaceBase_t
{
public:
	inline surfaceTrisurf_t () { type = SURFACE_TRISURF; };
	vec3_t	mins, maxs;		//?? origin/radius?
	int		numVerts, numIndexes;
	vertexNormal_t *verts;
	int		*indexes;
//!!	virtual void Tesselate ();
};

class surfaceMd3_t : public surfaceBase_t
{
public:
	inline surfaceMd3_t () { type = SURFACE_MD3; };
	int		numFrames;			// same value in owner md3Model_t.numFrames

	int		numVerts;
	float	*texCoords;			// numVerts*2

	int		numTris;
	int		*indexes;			// numTris*3

	int		numShaders;
	shader_t **shaders;			// [numShaders]

	vertexMd3_t *verts;			// numVerts*numFrames
	virtual void Tesselate ();
};

class surfacePoly_t : public surfaceBase_t
{
public:
	inline surfacePoly_t () { type = SURFACE_POLY; };
	int		numVerts;
	vertexPoly_t verts[1];		// [numVerts]
	virtual void Tesselate ();
};

// dummy surfaces

class surfaceParticle_t : public surfaceBase_t
{
public:
	inline surfaceParticle_t () { type = SURFACE_PARTICLE; };
	particle_t *part;
	virtual void Tesselate ();
};


//!! rename
class surfaceEntity_t : public surfaceBase_t
{
public:
	inline surfaceEntity_t () { type = SURFACE_ENTITY; };
	refEntity_t *ent;
	virtual void Tesselate ();
};


/*-----------------------------------------------------------------------------
	BSP model (world)
-----------------------------------------------------------------------------*/

struct node_t
{
	int		isNode:1;			// true if this is a leaf
	int		haveAlpha:1;		// true if leaf have surface(s) with translucent shader
	byte	frustumMask;
	int		visFrame, frame;	// PVS-cull frame, frustum-cull frame
	// leaf geometry
	CBox	bounds;
	cplane_t *plane;
	// tree structure
	node_t	*parent, *children[2];
	// BSP draw sequence (dynamic)
	int		drawOrder;			// 0 - first, 1 - next etc.
	node_t	*drawNext;
	refEntity_t *drawEntity;
	particle_t *drawParticle;
	beam_t	*drawBeam;
	// visibility params
	int		cluster, area;
	// surfaces (only if isNode==false)
	surfaceBase_t **leafFaces;
	int		numLeafFaces;
};


struct gl_flare_t
{
	// position
	vec3_t	origin;
	surfaceBase_t *surf;
	inlineModel_t *owner;
	node_t	*leaf;
	// size
	float	size;
	float	radius;
	// lighting
	color_t	color;
	byte	style;				//!! unused now (is it needed ?)
	float	lastTime;
	gl_flare_t *next;
};


struct bspModel_t				//?? really needs as separate struc? (only one instance at a time)
{
	char	name[MAX_QPATH];
	CMemoryChain *dataChain;
	//?? shaders, fog, lightGrid (Q3)
	// planes
	cplane_t *planes;
	int		numPlanes;
	// bsp
	node_t	*nodes;				// nodes[numNodes], leafs[numLeafsNodes-numNodes]
	int		numNodes;
	int		numLeafNodes;
	// inline models
	inlineModel_t *models;
	int		numModels;
	// surfaces
	surfaceBase_t **faces;
	int		numFaces;
	// leaf surfaces
	surfaceBase_t **leafFaces;
	int		numLeafFaces;
	// visibility
	int		numClusters;
	int		visRowSize;
	byte	*visInfo;
	// lights
	CMemoryChain *lightGridChain;
	lightCell_t **lightGrid;
	int		numLightCells;
	int		gridMins[3], mapGrid[3];	// start/size of world in lightgrid units
	gl_flare_t *flares;
	int		numSlights;			//?? rename slight -> plight (or light)
	gl_slight_t *slights;
	int		numSurfLights;
	surfLight_t *surfLights;
	int		numFlares;
	// sun
	float	sunLight;			// intensity; 0 if no sun
	vec3_t	sunColor;
	vec3_t	sunVec;
	bool	haveSunAmbient;
	vec3_t	sunAmbient;
	vec3_t	ambientLight;
};


extern bspModel_t	map;


/*-----------------------------------------------------------------------------
	Models
-----------------------------------------------------------------------------*/

typedef enum {	//?? eliminate
	MODEL_UNKNOWN,
	MODEL_INLINE,
	MODEL_SP2,
	MODEL_MD3
} modelType_t;


// Common struct for all model types
class model_t : public CRenderModel
{
public:
	modelType_t	type;
	int		size;				// in memory
	inline model_t () { type = MODEL_UNKNOWN; };
	virtual void InitEntity (entity_t *ent, refEntity_t *out);
	virtual void AddSurfaces (refEntity_t *e);
	virtual void DrawLabel (refEntity_t *e);
	virtual node_t *GetLeaf (refEntity_t *e);	// NULL when culled
};


class inlineModel_t : public model_t
{
public:
	CBox	bounds;
	float	radius;
	int		headnode;			//?? is Q3 inline models have this ?; used for trace functions only
	surfaceBase_t **faces;
	int		numFaces;
	inline inlineModel_t () { type = MODEL_INLINE; };
	virtual void InitEntity (entity_t *ent, refEntity_t *out);
	virtual void AddSurfaces (refEntity_t *e);
	virtual void DrawLabel (refEntity_t *e);
	virtual node_t *GetLeaf (refEntity_t *e);
};


// frame bounding volume (for culling)
struct md3Frame_t
{
	CBox	bounds;
	vec3_t	localOrigin;		// origin for bounding sphere in model coords; computed: (mins+maxs)/2
	float	radius;				// radius of the bounding sphere
};

//!!!!
struct md3TagFrame_t
{
	vec3_t	origin;
	vec3_t	axis[3];
};

struct md3Tag_t
{
	char	name[MAX_QPATH];	// tag name; case-sensitive
//	md3TagFrame_t frames[1];	// [md3.numFrames]
	//?!!! remove:
	vec3_t	origin;
	vec3_t	axis[3];
};

class md3Model_t : public model_t
{
public:
	int		numSurfaces;		// for MD2 = 1
	surfaceMd3_t *surf;			// [numSurfaces]
	int		numFrames;
	md3Frame_t *frames;			// [numFrames]
	int		numTags;
	md3Tag_t *tags;				// [numTags]
	inline md3Model_t () { type = MODEL_MD3; };
	virtual void InitEntity (entity_t *ent, refEntity_t *out);
	virtual void AddSurfaces (refEntity_t *e);
	virtual void DrawLabel (refEntity_t *e);
	virtual node_t *GetLeaf (refEntity_t *e);
};


struct sp2Frame_t
{
	float	width, height;
	float	localOrigin[2];
	shader_t *shader;
};

class sp2Model_t : public model_t
{
public:
	int		numFrames;
	float	radius;
	sp2Frame_t frames[1];		// [numFrames]
	inline sp2Model_t () { type = MODEL_SP2; };
	virtual void InitEntity (entity_t *ent, refEntity_t *out);
	virtual void AddSurfaces (refEntity_t *e);
	virtual node_t *GetLeaf (refEntity_t *e);
};


/*-----------------------------------------------------------------------------
	Functions
-----------------------------------------------------------------------------*/

node_t *PointInLeaf (vec3_t p);

void	InitModels (void);
void	ShutdownModels (void);

model_t	*FindModel (const char *name);
shader_t *FindSkin (const char *name);
void	LoadWorldMap (const char *name);	//?? rename


} // namespace

#endif
