#ifndef __GL_MODEL_INCLUDED__
#define __GL_MODEL_INCLUDED__


/*-------------------- Lightmaps --------------------------*/

#define MAX_LIGHTMAPS	64	// max lightmap textures
#define LIGHTMAP_SIZE	256	// width and height of lightmap texture (square)

typedef struct
{
	int		index;			// "*lightmap%d"
	image_t	*image;			// required for uploading (updating) lightmap
	int		*allocated;		// [LIGHTMAP_SIZE]
	byte	*pic;			// [LIGHTMAP_SIZE*LIGHTMAP_SIZE*4]
	qboolean empty;
	qboolean filled;
} lightmapBlock_t;

typedef struct dynamicLightmap_s
{
	// style info
	int		numStyles;		// 1..4
	byte	style[4];		// style numbers
	float	modulate[4][3];	// current (uploaded) state of styles
	// source
	byte	*source;		// points to "byte rgb[w*h*numStyles][3]"
	int		w, h;			// size of source
	// info for uploading lightmap
	int		s, t;			// glTexSubimage to (s,t)-(s+w,t+h)
	lightmapBlock_t *block;
} dynamicLightmap_t;


/*------------------- Surface types -----------------------*/

// Vertex without normal
typedef struct
{
	vec3_t	xyz;			// vertex itself
	float	st[2], lm[2];	// texture coordinates (TCGEN_TEXTURE/TCGEN_LIGHTMAP)
	float	lm2[2];			// for vertex color calculation: coords in surface lightmap rect (non-normalized)
	union {
		byte	c[4];		// vertex color
		int		rgba;
	};
} vertex_t;

// Vertex with stored normal
typedef struct
{
	vec3_t	xyz;
	float	st[2], lm[2];
	union {
		byte	c[4];
		int		rgba;
	};
	vec3_t	normal;
} vertexNormal_t;

typedef struct
{
	short	xyz[3];
	short	normal;			//?? unused now; byte[2] -- angles
} vertexMd3_t;	//?? == md3XyzNormal_t

typedef enum
{
	SURFACE_PLANAR,			// surfacePlanar_t
	SURFACE_TRISURF,		// surfaceTrisurf_t
	SURFACE_MD3,			// surfaceMd3_t
	SURFACE_PARTICLE
} surfaceType_t;

// Planar surface: same normal for all vertexes
typedef struct surfacePlanar_s
{
	vec3_t	mins, maxs;
	cplane_t plane;
	int		numVerts, numIndexes;
	vertex_t *verts;
	int		*indexes;
	// Q2/HL dynamic lightmap (NULL for Q3)
	dynamicLightmap_t *lightmap;
} surfacePlanar_t;

//!! warning: if shader.fast -- use vertex_t instead of vertexNormal_t (anyway, normals are not required)
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
	vec3_t	localOrigin;	// origin for bounding sphere in model coords
	float	radius;			// radius of the bounding sphere
} md3Frame_t;

typedef struct
{
	int		numFrames;

	int		numVerts;
	float	*texCoords;		// numVerts*2

	int		numTris;
	int		*indexes;		// numTris*3

	int		numShaders;
	shader_t **shaders;		// [numShaders]

	vertexMd3_t *verts;		// numVerts*numFrames
} surfaceMd3_t;

typedef struct surfaceCommon_s
{
	shader_t *shader;		// ignored for models
	int		frame;			// ignored for models
	//int		fogNum;
	int		type;
	union {
		surfaceTrisurf_t *tri;	// type = SURFACE_PLANAR
		surfacePlanar_t *pl;	// type = SURFACE_TRISURF
		surfaceMd3_t	*md3;	// type = SURFACE_MD3
		particle_t		*part;	// type = SURFACE_PARTICLE
	};
} surfaceCommon_t;

/*------------------------- BSP ---------------------------*/

typedef struct node_s
{
	qboolean isNode;
	byte	frustumMask;
	int		frame;
	// leaf geometry
	float	mins[3], maxs[3];
	cplane_t *plane;
	// tree structure
	struct node_s *parent, *children[2];
	// BSP draw sequence (dynamic)
	struct node_s *drawNext;
	refEntity_t *drawEntity;
	particle_t *drawParticle;
	// visibility params
	int		cluster, area;
	// surfaces (only if isNode==false)
	surfaceCommon_t **leafFaces;
	int		numLeafFaces;
} node_t;


typedef struct
{
	float	mins[3], maxs[3];
	vec3_t	center;
	float	radius;
	surfaceCommon_t *faces;
	int		numFaces;
} inlineModel_t;


typedef struct
{
	char	name[MAX_QPATH];
	void	*hunk;
	//?? shaders, inline models, fog, lightGrid (Q3)
	//!! entity string
	// planes
	cplane_t *planes;
	int		numPlanes;
	// bsp
	node_t	*nodes;		// nodes[numNodes], leafs[numLeafsNodes-numNodes]
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
	byte	*noVis;		// used for r_novis or (clusterNum < 0)
} bspModel_t;


/*------------- Triangle models ------------*/

typedef struct
{
	int		numSurfaces;	// for MD2 = 1
	surfaceCommon_t *surf;	// [numSurfaces];  ->surfaceCommon_t->surfaceMd3_t
	int		numFrames;
	md3Frame_t *frames;		// for culling
} md3Model_t;


/*---------------- Models ------------------*/

typedef enum {
	MODEL_INLINE,
	MODEL_SP2,		//!! not implemented
	MODEL_MD3
} modelType_t;


// Common struct for all model types
typedef struct model_s
{
	char	name[MAX_QPATH];

	modelType_t	type;
	union {
		inlineModel_t	*inlineModel;	// MODEL_INLINE
		md3Model_t		*md3;			// MODEL_MD3
	};
} model_t;


/*---------------- Variables ---------------*/

extern bspModel_t	gl_worldModel;


/*---------------- Functions ---------------*/

node_t *GL_PointInLeaf (vec3_t p);


#endif
