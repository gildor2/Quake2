#ifndef __GL_MODEL_INCLUDED__
#define __GL_MODEL_INCLUDED__


#define map gl_worldModel		// short alias

/*--------------------- Lighting --------------------------*/

typedef struct
{
	float	pos[2];
	float	radius;
	refDlight_t *dlight;
	vec3_t	*axis;
} surfDlight_t;


/*------------------- Surface types -----------------------*/

// Vertex without normal (for planar surface)
typedef struct
{
	vec3_t	xyz;				// vertex itself
	float	st[2], lm[2];		// texture coordinates (TCGEN_TEXTURE/TCGEN_LIGHTMAP)
	float	lm2[2];				// for vertex color calculation: coords in surface lightmap rect (non-normalized)
	float	pos[2];				// 2D position in plane axis
	color_t	c;					// vertex color
} vertex_t;

// Vertex with stored normal
typedef struct
{
	vec3_t	xyz;
	float	st[2], lm[2];
	color_t	c;
	vec3_t	normal;
} vertexNormal_t;

typedef struct
{
	vec3_t	xyz;
	float	st[2];
	color_t	c;
} vertexPoly_t;

typedef struct
{
	short	xyz[3];
	short	normal;			//?? unused now; byte[2] -- angles
} vertexMd3_t;	//?? == md3XyzNormal_t

typedef enum
{
	SURFACE_PLANAR,				// surfacePlanar_t
	SURFACE_TRISURF,			// surfaceTrisurf_t
	SURFACE_MD3,				// surfaceMd3_t
	SURFACE_POLY,				// surfacePoly_t
	SURFACE_PARTICLE			// particle_t
} surfaceType_t;

// Planar surface: same normal for all vertexes
struct surfLight_s;				// forward declaration
struct inlineModel_s;

typedef struct surfacePlanar_s
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
	struct surfLight_s *light;
	surfDlight_t *dlights;
} surfacePlanar_t;

//!! warning: if shader.fast -- use vertex_t instead of vertexNormal_t (normals are not required for fast shader)
// Triangular surface: non-planar, every vertex have its own normal vector
typedef struct
{
	vec3_t	mins, maxs;		//?? origin/radius?
	int		numVerts, numIndexes;
	vertexNormal_t *verts;
	int		*indexes;
} surfaceTrisurf_t;

// frame bounding volume (for culling)
typedef struct
{
	vec3_t	mins, maxs;
	vec3_t	localOrigin;		// origin for bounding sphere in model coords
	float	radius;				// radius of the bounding sphere
} md3Frame_t;

typedef struct
{
	int		numFrames;

	int		numVerts;
	float	*texCoords;			// numVerts*2

	int		numTris;
	int		*indexes;			// numTris*3

	int		numShaders;
	shader_t **shaders;			// [numShaders]

	vertexMd3_t *verts;			// numVerts*numFrames
} surfaceMd3_t;

typedef struct
{
	int		numVerts;
	vertexPoly_t verts[0];		// [numVerts]
} surfacePoly_t;

typedef struct surfaceCommon_s
{
	shader_t *shader;			// ignored for frame models
	int		frame;				// ignored for frame models
	int		dlightFrame;
	unsigned dlightMask;
	int		type;
	struct inlineModel_s *owner;
	union {
		surfaceTrisurf_t *tri;	// type = SURFACE_PLANAR
		surfacePlanar_t *pl;	// type = SURFACE_TRISURF
		surfaceMd3_t	*md3;	// type = SURFACE_MD3
		surfacePoly_t	*poly;	// type = SURFACE_POLY
		particle_t		*part;	// type = SURFACE_PARTICLE
	};
} surfaceCommon_t;

/*------------------------- BSP ---------------------------*/

typedef struct node_s
{
	byte	isNode;				// true if this is a leaf
	byte	haveAlpha;			// true if leaf have surface(s) with translucent shader
	byte	frustumMask;
	int		visFrame, frame;	// PVS-cull frame, frustum-cull frame
	// leaf geometry
	float	mins[3], maxs[3];
	cplane_t *plane;
	// tree structure
	struct node_s *parent, *children[2];
	// BSP draw sequence (dynamic)
	int		drawOrder;			// 0 - first, 1 - next etc.
	struct node_s *drawNext;
	refEntity_t *drawEntity;
	particle_t *drawParticle;
	// visibility params
	int		cluster, area;
	// surfaces (only if isNode==false)
	surfaceCommon_t **leafFaces;
	int		numLeafFaces;
} node_t;


typedef struct inlineModel_s	//?? replace this structure with extended cmodel_t
{
	vec3_t	mins, maxs;
	vec3_t	center;				// model center (in world coordinate system)
	float	radius;
	int		headnode;			//?? is Q3 inline models have this ?
	surfaceCommon_t **faces;
	int		numFaces;
} inlineModel_t;


typedef struct gl_flare_s
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
	struct gl_flare_s *next;
} gl_flare_t;


// point light / spot light
typedef struct
{
	slightType_t type;
	byte	spot;				// bool
	byte	style;
	vec3_t	origin;
	vec3_t	color;
	float	intens;
	int		cluster;
	vec3_t	spotDir;
	float	spotDot;
	float	focus;
	float	fade;
} gl_slight_t;


typedef struct surfLight_s
{
	qboolean sky;
	vec3_t	color;
	float	intens;
	int		cluster;
	surfacePlanar_t *pl;		// used for normal/axis/bounds
	struct surfLight_s *next;
} surfLight_t;


#define LIGHTGRID_STEP	32

typedef struct
{
	byte	c[6][3];
} lightCell_t;


typedef struct
{
	char	name[MAX_QPATH];
	void	*hunk;
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
	void	*lightGridChain;
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
	qboolean haveSunAmbient;
	vec3_t	sunAmbient;
	vec3_t	ambientLight;
} bspModel_t;


/*------------- Triangle models ------------*/

typedef struct
{
	int		numSurfaces;		// for MD2 = 1
	surfaceCommon_t *surf;		// [numSurfaces];  ->surfaceCommon_t->surfaceMd3_t
	int		numFrames;
	md3Frame_t *frames;			// for culling
} md3Model_t;


/*------------ Sprite models ---------------*/

typedef struct
{
	float	width, height;
	float	localOrigin[2];
	shader_t *shader;
} sp2Frame_t;

typedef struct
{
	int		numFrames;
	float	radius;
	sp2Frame_t frames[0];		// [numFrames]
} sp2Model_t;

/*---------------- Models ------------------*/

typedef enum {
	MODEL_INLINE,
	MODEL_SP2,
	MODEL_MD3
} modelType_t;


// Common struct for all model types
typedef struct model_s
{
	char	name[MAX_QPATH];

	modelType_t	type;
	union {
		inlineModel_t	*inlineModel;	// MODEL_INLINE
		sp2Model_t		*sp2;			// MODEL_SP2
		md3Model_t		*md3;			// MODEL_MD3
	};
} model_t;


/*---------------- Variables ---------------*/

extern bspModel_t	gl_worldModel;


/*---------------- Functions ---------------*/

node_t *GL_PointInLeaf (vec3_t p);


#endif
