#ifndef __GL_MODEL_INCLUDED__
#define __GL_MODEL_INCLUDED__

//?? change this:
#ifdef PROFILE_LOADING
#define START_PROFILE(name)			\
	{								\
		static const char _name[] = #name;\
		const char *_arg = "";		\
		unsigned _time = appCycles();
#define START_PROFILE2(name,arg)	\
	{								\
		static const char _name[] = #name;\
		const char *_arg = arg;		\
		unsigned _time = appCycles();
#define END_PROFILE	\
		if (Cvar_VariableInt("r_profile")) \
			Com_Printf(S_MAGENTA"%s "S_GREEN"%s"S_CYAN": %.2f ms\n", _name, _arg,\
			appDeltaCyclesToMsecf(appCycles() - _time)); \
	}
#else
#define START_PROFILE(name)
#define START_PROFILE2(name,arg)
#define END_PROFILE
#endif


#include "gl_frontend.h"
#include "gl_light.h"


namespace OpenGLDrv {


/*-----------------------------------------------------------------------------
	Surfaces
-----------------------------------------------------------------------------*/

// Vertex without normal (for planar surface)
struct vertex_t
{
	CVec3	xyz;				// vertex itself
	float	st[2], lm[2];		// texture coordinates (TCGEN_TEXTURE/TCGEN_LIGHTMAP)
	float	lm2[2];				// for vertex color calculation: coords in surface lightmap rect (non-normalized)
	float	pos[2];				// 2D position in plane axis
	color_t	c;					// vertex color
};

// Vertex with stored normal
struct vertexNormal_t
{
	CVec3	xyz;
	float	st[2], lm[2];
	color_t	c;
	CVec3	normal;
};

struct vertexPoly_t
{
	CVec3	xyz;
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
	virtual void Tesselate (refEntity_t &ent) = NULL;
};

class surfacePlanar_t : public surfaceBase_t
{
public:
	inline surfacePlanar_t () { type = SURFACE_PLANAR; };
	CBox	bounds;
	cplane_t plane;
	CVec3	axis[2];			// 2 orthogonal identity vectors from surface
	float	mins2[2], maxs2[2];	// 2D-bounds (in "axis" coordinate system)
	int		numVerts, numIndexes;
	vertex_t *verts;
	int		*indexes;
	// Q2/HL dynamic lightmap (NULL for Q3 ??)
	dynamicLightmap_t *lightmap;
	surfLight_t *light;
	surfDlight_t *dlights;
	virtual void Tesselate (refEntity_t &ent);
};

//!! warning: if shader.fast -- use vertex_t instead of vertexNormal_t (normals are not required for fast shader)
// Triangular surface: non-planar, every vertex have its own normal vector
class surfaceTrisurf_t : public surfaceBase_t
{
public:
	inline surfaceTrisurf_t () { type = SURFACE_TRISURF; };
	CBox	bounds;		//?? origin/radius?
	int		numVerts, numIndexes;
	vertexNormal_t *verts;
	int		*indexes;
//!!	virtual void Tesselate (refEntity_t &ent);
};

class surfaceMd3_t : public surfaceBase_t
{
public:
	inline surfaceMd3_t () { type = SURFACE_MD3; };

	char	name[MAX_QPATH];	// for skin application
	// fields, same on all surfaces
	int		numFrames;

	int		numVerts;
	float	*texCoords;			// numVerts*2

	int		numTris;
	int		*indexes;			// numTris*3

	int		numShaders;
	shader_t **shaders;			// [numShaders]

	vertexMd3_t *verts;			// numVerts*numFrames
	virtual void Tesselate (refEntity_t &ent);
};

class surfacePoly_t : public surfaceBase_t
{
public:
	inline surfacePoly_t () { type = SURFACE_POLY; };
	int		numVerts;
	vertexPoly_t verts[1];		// [numVerts]
	virtual void Tesselate (refEntity_t &ent);
};

// dummy surfaces

class surfaceParticle_t : public surfaceBase_t
{
public:
	inline surfaceParticle_t () { type = SURFACE_PARTICLE; };
	particle_t *part;
	virtual void Tesselate (refEntity_t &ent);
};


//!! rename
class surfaceEntity_t : public surfaceBase_t
{
public:
	inline surfaceEntity_t () { type = SURFACE_ENTITY; };
	refEntity_t *entity;
	virtual void Tesselate (refEntity_t &ent);
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
	CVec3	origin;
	surfaceBase_t *surf;
	inlineModel_t *owner;
	node_t	*leaf;
	// size
	float	size;
	float	radius;
	// lighting
	color_t	color;
	byte	style;
	double	lastTime;
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
	CVec3	sunColor;
	CVec3	sunVec;
	bool	haveSunAmbient;
	CVec3	sunAmbient;
	CVec3	ambientLight;
};


extern bspModel_t	map;


/*-----------------------------------------------------------------------------
	Models
-----------------------------------------------------------------------------*/

typedef enum {	//?? eliminate
	MODEL_UNKNOWN,
	MODEL_INLINE,
	MODEL_SPR,
	MODEL_MD3
} modelType_t;


// Common struct for all model types
class model_t : public CRenderModel
{
public:
	modelType_t	type;
	int		size;				// in memory
	inline model_t () { type = MODEL_UNKNOWN; };
	virtual bool LerpTag (int frame1, int frame2, float lerp, const char *tagName, CCoords &tag) const;
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
	CVec3	localOrigin;		// origin for bounding sphere in model coords; computed: (mins+maxs)/2
	float	radius;				// radius of the bounding sphere
};


class md3Model_t : public model_t
{
public:
	int		numSurfaces;		// for MD2 = 1
	surfaceMd3_t *surf;			// [numSurfaces]
	int		numFrames;
	md3Frame_t *frames;			// [numFrames]
	int		numTags;
	char	*tagNames;			// [MAX_QPATH][numTags]
	CCoords	*tags;				// [numFrames][numTags]
	inline md3Model_t () { type = MODEL_MD3; };
	// funcs for OpenGLDrv
	virtual void InitEntity (entity_t *ent, refEntity_t *out);
	virtual void AddSurfaces (refEntity_t *e);
	virtual void DrawLabel (refEntity_t *e);
	virtual node_t *GetLeaf (refEntity_t *e);
	// funcs for scene manager
	virtual bool LerpTag (int frame1, int frame2, float lerp, const char *tagName, CCoords &tag) const;
};


struct sprFrame_t
{
	float	width, height;
	float	localOrigin[2];
	shader_t *shader;
};

class sprModel_t : public model_t
{
public:
	int		numFrames;
	float	radius;
	sprFrame_t frames[1];		// [numFrames]
	inline sprModel_t () { type = MODEL_SPR; };
	virtual void InitEntity (entity_t *ent, refEntity_t *out);
	virtual void AddSurfaces (refEntity_t *e);
	virtual node_t *GetLeaf (refEntity_t *e);
};


/*-----------------------------------------------------------------------------
	Functions
-----------------------------------------------------------------------------*/

#define MAX_GLMODELS	1024
extern model_t *modelsArray[MAX_GLMODELS];
extern int	modelCount;

// common
void	InitModels ();
void	ShutdownModels ();
model_t	*FindModel (const char *name);
void	FreeModels ();

// bsp model
node_t *PointInLeaf (const CVec3 &p);
void	LoadWorldMap (const char *name);	//?? rename

// triangle models
md3Model_t *LoadMd2 (const char *name, byte *buf, unsigned len);
md3Model_t *LoadMd3 (const char *name, byte *buf, unsigned len);
sprModel_t *LoadSp2 (const char *name, byte *buf, unsigned len);
sprModel_t *LoadSpr (const char *name, byte *buf, unsigned len);


} // namespace

#endif
