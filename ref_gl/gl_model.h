#ifndef __GL_MODEL_INCLUDED__
#define __GL_MODEL_INCLUDED__


#define map gl_worldModel		// short alias

#include "gl_frontend.h"
#include "gl_light.h"

/*--------------------- Lighting --------------------------*/

struct surfDlight_t
{
	float	pos[2];
	float	radius;
	refDlight_t *dlight;
	vec3_t	*axis;
};


/*------------------- Surface types -----------------------*/

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

struct vertexMd3_t	//?? == md3XyzNormal_t
{
	short	xyz[3];
	short	normal;
};

typedef enum
{
	SURFACE_PLANAR,				// surfacePlanar_t
	SURFACE_TRISURF,			// surfaceTrisurf_t
	SURFACE_MD3,				// surfaceMd3_t
	SURFACE_POLY,				// surfacePoly_t
	SURFACE_PARTICLE,			// particle_t
	SURFACE_ENTITY				// refEntity_t
} surfaceType_t;

// Planar surface: same normal for all vertexes
struct surfLight_t;				// forward declaration
struct inlineModel_t;

struct surfacePlanar_t
{
	vec3_t	mins, maxs;
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
};

//!! warning: if shader.fast -- use vertex_t instead of vertexNormal_t (normals are not required for fast shader)
// Triangular surface: non-planar, every vertex have its own normal vector
struct surfaceTrisurf_t
{
	vec3_t	mins, maxs;		//?? origin/radius?
	int		numVerts, numIndexes;
	vertexNormal_t *verts;
	int		*indexes;
};

// frame bounding volume (for culling)
struct md3Frame_t
{
	vec3_t	mins, maxs;
	vec3_t	localOrigin;		// origin for bounding sphere in model coords
	float	radius;				// radius of the bounding sphere
};

struct surfaceMd3_t
{
	int		numFrames;

	int		numVerts;
	float	*texCoords;			// numVerts*2

	int		numTris;
	int		*indexes;			// numTris*3

	int		numShaders;
	shader_t **shaders;			// [numShaders]

	vertexMd3_t *verts;			// numVerts*numFrames
};

struct surfacePoly_t
{
	int		numVerts;
	vertexPoly_t verts[1];		// [numVerts]
};

struct surfaceCommon_t
{
	shader_t *shader;			// ignored for frame models
	int		frame;				// ignored for frame models
	int		dlightFrame;
	unsigned dlightMask;
	int		type;
	inlineModel_t *owner;
	union {
		surfaceTrisurf_t *tri;	// type = SURFACE_PLANAR
		surfacePlanar_t *pl;	// type = SURFACE_TRISURF
		surfaceMd3_t	*md3;	// type = SURFACE_MD3
		surfacePoly_t	*poly;	// type = SURFACE_POLY
		particle_t		*part;	// type = SURFACE_PARTICLE
		refEntity_t		*ent;	// type = SURFACE_ENTITY
	};
};

/*------------------------- BSP ---------------------------*/

struct node_t
{
	int		isNode:1;			// true if this is a leaf
	int		haveAlpha:1;		// true if leaf have surface(s) with translucent shader
	byte	frustumMask;
	int		visFrame, frame;	// PVS-cull frame, frustum-cull frame
	// leaf geometry
	float	mins[3], maxs[3];
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
	surfaceCommon_t **leafFaces;
	int		numLeafFaces;
};


struct inlineModel_t			//?? replace this structure with extended cmodel_t
{
	vec3_t	mins, maxs;
	float	radius;
	int		headnode;			//?? is Q3 inline models have this ?
	surfaceCommon_t **faces;
	int		numFaces;
};


struct gl_flare_t
{
	// position
	vec3_t	origin;
	surfaceCommon_t *surf;
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
	inlineModel_t	*models;
	int		numModels;
	// surfaces
	surfaceCommon_t *faces;
	int		numFaces;
	// leaf surfaces
	surfaceCommon_t **leafFaces;
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


/*------------- Triangle models ------------*/

struct md3Model_t
{
	int		numSurfaces;		// for MD2 = 1
	surfaceCommon_t *surf;		// [numSurfaces];  ->surfaceCommon_t->surfaceMd3_t
	int		numFrames;
	md3Frame_t *frames;			// for culling
};


/*------------ Sprite models ---------------*/

struct sp2Frame_t
{
	float	width, height;
	float	localOrigin[2];
	shader_t *shader;
};

struct sp2Model_t
{
	int		numFrames;
	float	radius;
	sp2Frame_t frames[1];		// [numFrames]
};

/*---------------- Models ------------------*/

typedef enum {
	MODEL_UNKNOWN,
	MODEL_INLINE,
	MODEL_SP2,
	MODEL_MD3
} modelType_t;


// Common struct for all model types
struct model_t
{
	char	name[MAX_QPATH];
	int		size;

	modelType_t	type;
	union {								// MODEL_UNKNOWN: NULL in this field
		inlineModel_t	*inlineModel;	// MODEL_INLINE
		sp2Model_t		*sp2;			// MODEL_SP2
		md3Model_t		*md3;			// MODEL_MD3
	};
};


/*---------------- Variables ---------------*/

extern bspModel_t	gl_worldModel;


/*---------------- Functions ---------------*/

node_t *GL_PointInLeaf (vec3_t p);

void	GL_InitModels (void);
void	GL_ShutdownModels (void);
void	GL_ResetModels (void);

model_t	*GL_FindModel (const char *name);
shader_t *GL_FindSkin (const char *name);
void	GL_LoadWorldMap (const char *name);


#endif
