#ifndef __GL_MODEL_INCLUDED__
#define __GL_MODEL_INCLUDED__

//?? change this:
#ifdef PROFILE_LOADING
#define START_PROFILE(name)			\
	{								\
		static const char _name[] = #name;\
		const char *_arg = "";		\
		int64 _time = appCycles64();
#define START_PROFILE2(name,arg)	\
	{								\
		static const char _name[] = #name;\
		const char *_arg = arg;		\
		int64 _time = appCycles64();
#define END_PROFILE	\
		int64 _delta = appCycles64() - _time; \
		if (Cvar_VariableInt("r_profile")) \
			appPrintf(S_MAGENTA"%s "S_GREEN"%s"S_CYAN": %.2f ms\n", _name, _arg,\
			appCyclesToMsecf(_delta)); \
	}
#else
#define START_PROFILE(name)
#define START_PROFILE2(name,arg)
#define END_PROFILE
#endif


#include "gl_frontend.h"
#include "gl_light.h"

// declaration of CBspNodeExtra and CBspLeafExtra (should be declared outside of renderer namespace)
struct CBspNodeExtra
{
	bool	haveAlpha;			// true if leaf have surface(s) with translucent shader
	int		visFrame, frame;	// PVS-cull frame, frustum-cull frame
								//?? rename "frame" to "drawFrame" ?
	// BSP draw sequence (dynamic)
	int		drawOrder;			// 0 - first, 1 - next etc.
};

struct CBspLeafExtra : public CBspNodeExtra
{
	byte	frustumMask;
	const CBspLeaf *drawNext;
	// objects, inserted into leaf
	OpenGLDrv::refEntity_t *drawEntity;
	particle_t *drawParticle;
	beam_t	*drawBeam;
	// surfaces
	OpenGLDrv::surfaceBase_t **faces;
};


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

struct vertexMd3_t				//?? == dMd3XyzNormal_t (rename one of types? derive struc with no new fields?)
{
	short	xyz[3];
	short	normal;
};

enum surfaceType_t
{
	SURFACE_BAD,
	SURFACE_PLANAR,				// surfacePlanar_t
	SURFACE_TRISURF,			// surfaceTrisurf_t
	SURFACE_MD3,				// surfaceMd3_t
	SURFACE_POLY,				// surfacePoly_t
	SURFACE_PARTICLE,			// particle_t
	SURFACE_ENTITY				// refEntity_t
};

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
	virtual void Tesselate(refEntity_t &ent) = 0;
	virtual void AddToSky(bool showDebug = false);
};

class surfacePlanar_t : public surfaceBase_t
{
public:
	inline surfacePlanar_t() { type = SURFACE_PLANAR; }
	CBox	bounds;
	CPlane	plane;
	CVec3	axis[2];			// 2 orthogonal identity vectors from surface
	float	mins2[2], maxs2[2];	// 2D-bounds (in "axis" coordinate system)
	int		numVerts, numIndexes;
	vertex_t *verts;
	int		*indexes;
	// Q1/Q2 dynamic lightmap (NULL for Q3)
	dynamicLightmap_t *lightmap;
	CLight	*light;
	surfDlight_t *dlights;
	virtual void Tesselate(refEntity_t &ent);
	virtual void AddToSky(bool showDebug);
};

// Triangular surface: non-planar, every vertex have its own normal vector
class surfaceTrisurf_t : public surfaceBase_t
{
public:
	inline surfaceTrisurf_t() { type = SURFACE_TRISURF; }
	CBox	bounds;
	int		numVerts, numIndexes;
	vertexNormal_t *verts;
	int		*indexes;
	virtual void Tesselate(refEntity_t &ent);
	virtual void AddToSky(bool showDebug);
};

class surfaceMd3_t : public surfaceBase_t
{
public:
	inline surfaceMd3_t() { type = SURFACE_MD3; }

	TString<32> Name;			// for skin application
	// fields, same on all surfaces
	int		numFrames;

	int		numVerts;
	float	*texCoords;			// numVerts*2

	int		numTris;
	int		*indexes;			// numTris*3

	int		numShaders;
	shader_t **shaders;			// [numShaders]

	vertexMd3_t *verts;			// numVerts*numFrames
	virtual void Tesselate(refEntity_t &ent);
};

class surfacePoly_t : public surfaceBase_t
{
public:
	inline surfacePoly_t() { type = SURFACE_POLY; }
	int		numVerts;
	vertexPoly_t verts[1];		// [numVerts]
	virtual void Tesselate(refEntity_t &ent);
};

// dummy surfaces

class surfaceParticle_t : public surfaceBase_t
{
public:
	inline surfaceParticle_t() { type = SURFACE_PARTICLE; }
	particle_t *part;
	virtual void Tesselate(refEntity_t &ent);
};


//!! rename
class surfaceEntity_t : public surfaceBase_t
{
public:
	inline surfaceEntity_t() { type = SURFACE_ENTITY; }
	refEntity_t *entity;
	virtual void Tesselate(refEntity_t &ent);
};


/*-----------------------------------------------------------------------------
	BSP model (world)
-----------------------------------------------------------------------------*/

struct gl_flare_t
{
	// position
	CVec3	origin;
	surfaceBase_t *surf;
	inlineModel_t *owner;
	const CBspLeaf *leaf;
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
	CMemoryChain *dataChain;
	//?? shaders, fog, lightGrid (Q3)
	// inline models
	inlineModel_t *models;
	int		numModels;
	// surfaces
	surfaceBase_t **faces;
	int		numFaces;
	// leaf surfaces
	surfaceBase_t **leafFaces;
	int		numLeafFaces;
	// lights
	bool	monoLightmap;		// "true" for q1 maps
	CMemoryChain *lightGridChain;
	lightCell_t **lightGrid;
	int		numLightCells;
	int		gridMins[3], mapGrid[3];	// start/size of world in lightgrid units
	gl_flare_t *flares;
	CLight	*lights;			// static lights
	CLight	*flashLights;		// lights with dynamic lightstyle; rename !!
	int		numFlares;
	// sky
	bool	haveSkySurfaces;
	TString<64> SkyShader;		// name of texture for sky (when taken directly from map surfaces)
	// sun
	CVec3	sunSurfColor;		// color of sun surface (default sun color)
	CVec3	sunAvgColor;		// for sun ambient
	bool	haveSunAmbient;
	CVec3	sunAmbient;
	CVec3	ambientLight;
};


extern bspModel_t	map;


/*-----------------------------------------------------------------------------
	Models
-----------------------------------------------------------------------------*/

enum modelType_t	//?? eliminate
{
	MODEL_UNKNOWN,
	MODEL_INLINE,
	MODEL_SPR,
	MODEL_MD3
};


// Common struct for all model types
class model_t : public CRenderModel
{
public:
	modelType_t	type;
	int		size;				// in memory
	inline model_t() { type = MODEL_UNKNOWN; };
	virtual bool LerpTag(int frame1, int frame2, float lerp, const char *tagName, CCoords &tag) const;
	// when return false, entity will not be added to world
	virtual bool InitEntity(entity_t *ent, refEntity_t *out);
	virtual void AddSurfaces(refEntity_t *e);
	virtual void DrawLabel(refEntity_t *e);
	virtual const CBspLeaf *GetLeaf(refEntity_t *e);	// NULL when culled
};


class inlineModel_t : public model_t, public CBspModel
{
public:
	//?? can add link to CBspModel instead of copying most fields
	surfaceBase_t **faces;
	int		numFaces;
	inline inlineModel_t() { type = MODEL_INLINE; };
	virtual bool InitEntity(entity_t *ent, refEntity_t *out);
	virtual void AddSurfaces(refEntity_t *e);
	virtual void DrawLabel(refEntity_t *e);
	virtual const CBspLeaf *GetLeaf(refEntity_t *e);
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
	inline md3Model_t() { type = MODEL_MD3; };
	// funcs for OpenGLDrv
	virtual bool InitEntity(entity_t *ent, refEntity_t *out);
	virtual void AddSurfaces(refEntity_t *e);
	virtual void DrawLabel(refEntity_t *e);
	virtual const CBspLeaf *GetLeaf(refEntity_t *e);
	// funcs for scene manager
	virtual bool LerpTag(int frame1, int frame2, float lerp, const char *tagName, CCoords &tag) const;
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
	inline sprModel_t() { type = MODEL_SPR; };
	virtual bool InitEntity(entity_t *ent, refEntity_t *out);
	virtual void AddSurfaces(refEntity_t *e);
	virtual const CBspLeaf *GetLeaf(refEntity_t *e);
};


/*-----------------------------------------------------------------------------
	Functions
-----------------------------------------------------------------------------*/

#define MAX_GLMODELS	1024
extern model_t *modelsArray[MAX_GLMODELS];
extern int	modelCount;

// common
void	InitModels();
void	ShutdownModels();
model_t	*FindModel(const char *name);
void	FreeModels();

// bsp model
void	LoadWorldMap();

// triangle models
md3Model_t *LoadMd2(const char *name, byte *buf, unsigned len);
md3Model_t *LoadMd3(const char *name, byte *buf, unsigned len);
sprModel_t *LoadSp2(const char *name, byte *buf, unsigned len);
sprModel_t *LoadSpr(const char *name, byte *buf, unsigned len);


} // namespace

#endif
