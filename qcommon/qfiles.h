/*-----------------------------------------------------------------------------
	.MD2 model file format
-----------------------------------------------------------------------------*/

#define MD2_IDENT			BYTES4('I','D','P','2')
#define MD2_VERSION			8

/*
#define	MAX_TRIANGLES		4096
#define MAX_VERTS			2048
#define MAX_FRAMES			512
#define MD2_MAX_SKINS		32
*/
#define	MD2_MAX_SKINNAME	64

#if 0
// was used for software rendering only
struct dMd2St_t
{
	short	s;
	short	t;
};

struct dTriangle_t
{
	short	indexXyz[3];
	short	indexSt[3];
};
#endif

struct dMd2Vert_t
{
	byte	v[3];			// scaled byte to fit in frame mins/maxs
	byte	lightnormalindex;
};

struct dMd2Frame_t
{
	float	scale[3];		// multiply byte verts by this
	float	translate[3];	// then add this
	char	name[16];		// frame name from grabbing
	dMd2Vert_t verts[1];	// variable sized
};


/* glcmd format:
 *   - read int x
 *     - if x > 0 -- triangle strip of x verts
 *     - if x < 0 -- triangle fan of x verts
 *     - if x == 0 -- end of list
 *   - repeat (if not end)
 * Vertex format:
 *   float s, t
 *   int   vertexIndex
 */

struct dMd2_t
{
	int		ident;			// MD2_IDENT
	int		version;		// MD2_VERSION

	int		skinWidth;
	int		skinHeight;
	int		frameSize;		// byte size of each frame

	int		numSkins;
	int		numXyz;
	int		numSt;			// greater than num_xyz for seams
	int		numTris;
	int		numGlcmds;		// dwords in strip/fan command list
	int		numFrames;

	int		ofsSkins;		// char [MD2_MAX_SKINNAME][numSkins]
	int		ofsSt;			// unused; offset -> dMd2St_t [numSt]
	int		ofsTris;		// unused; offset -> dTriangle_t [numTris]
	int		ofsFrames;		// offset -> dMd2Frame_t [numFrames]
	int		ofsGlcmds;		// offset -> glcmd stream
	int		ofsEnd;			// end of file
};


/*-----------------------------------------------------------------------------
	.MD3 model file format
-----------------------------------------------------------------------------*/

#define MD3_IDENT			BYTES4('I','D','P','3')
#define MD3_VERSION			15

#define MD3_MAX_LODS		4
#define	MD3_MAX_TRIANGLES	8192		// per surface
#define MD3_MAX_VERTS		4096		// per surface

#define MD3_MAX_SHADERS		256			// per surface
#define MD3_MAX_FRAMES		1024		// per model
#define	MD3_MAX_SURFACES	32			// per model
#define MD3_MAX_TAGS		16			// per frame


#define	MD3_XYZ_SCALE		(1.0f/64)	// vertex scales

struct dMd3Frame_t
{
	CBox	bounds;
	CVec3	localOrigin;				// NOTE: q3 model converter tool always sets this to (0,0,0)
	float	radius;						//		 and this is a maximal distance to all bounds[] vectors
	char	name[16];
};

struct dMd3Tag_t
{
	char	name[MAX_QPATH];			// tag name
	CCoords	tag;						// CVec3 origin + axis[3]
};

struct dMd3Shader_t
{
	char	name[MAX_QPATH];
	int		shaderIndex;	//?? for in-game use
};

struct dMd3Triangle_t
{
	int		indexes[3];
};

struct dMd3St_t
{
	float	st[2];
};

struct dMd3XyzNormal_t
{
	short	xyz[3];
	short	normal;
};

/*
 * header			dMd3Surface_t
 * shaders			dMd3Shader_t [numShaders]
 * triangles		dMd3Triangle_t [numTriangles]
 * st				dMd3St_t [numVerts]
 * XyzNormals		dMd3XyzNormal_t [numVerts*numFrames]
 */
struct dMd3Surface_t
{
	int		ident;			//?? md3 conversion tool set this to MD3_IDENT
	char	name[MAX_QPATH]; //?? can be "tag_..."
	int		flags;			//?? useless

	int		numFrames;		// same for all md3 surfaces; == model.numFrames
	int		numShaders;		// same for all md3 surfaces
	int		numVerts;
	int		numTriangles;

	// offsets from start of the current surface
	int		ofsTriangles;	// offset -> dMd3Triangle_t [numTriangles]
	int		ofsShaders;		// offset -> dMd3Shader_t [numShaders]
	int		ofsSt;			// offset -> dMd3St_t [numVerts]
	int		ofsXyzNormals;	// offset -> dMd3XyzNormal_t [numVerts*numFrames]

	int		ofsEnd;			// offset -> next surface
};

struct dMd3_t
{
	int		ident;			// MD3_IDENT
	int		version;		// MD3_VERSION

	char	name[MAX_QPATH]; //??
	int		flags;			//?? useless

	int		numFrames;
	int		numTags;
	int		numSurfaces;

	int		numSkins;		//?? useless

	int		ofsFrames;		// offset -> dMd3Frame_t [numFrames]
	int		ofsTags;		// offset -> dMd3Tag_t [numTags*numFrames]
	int		ofsSurfaces;	// offset -> dMd3Surface_t [numSurfaces]

	int		ofsEnd;			// end of file
};


/*-----------------------------------------------------------------------------
	.SP2 sprite file format
-----------------------------------------------------------------------------*/

#define SP2_IDENT			BYTES4('I','D','S','2')
#define SP2_VERSION			2

struct dSp2Frame_t
{
	int		width, height;
	int		origin_x, origin_y;		// raster coordinates inside pic
	char	name[MD2_MAX_SKINNAME];	// name of pcx file
};

struct dSp2_t
{
	int		ident;
	int		version;
	int		numframes;
	dSp2Frame_t frames[1];			// variable sized
};


/*-----------------------------------------------------------------------------
	.WAL texture file format
-----------------------------------------------------------------------------*/

#define	MIPLEVELS	4
typedef struct
{
	char	name[32];
	unsigned width, height;
	unsigned offsets[MIPLEVELS];	// 4 mip maps stored
	char	animname[32];			// next frame in animation chain
	int		flags;
	int		contents;
	int		value;
} miptex_t;



/*-----------------------------------------------------------------------------
	Quake2 .BSP file format
-----------------------------------------------------------------------------*/

#define BSP2_IDENT			BYTES4('I','B','S','P')
#define BSP2_VERSION		38


// upper design bounds
// leaffaces, leafbrushes, planes, and verts are still bounded by
// 16 bit short limits
//#define	MAX_MAP_MODELS		1024
//#define	MAX_MAP_BRUSHES		8192
//#define	MAX_MAP_ENTITIES	2048
//#define	MAX_MAP_ENTSTRING	0x40000
//#define	MAX_MAP_TEXINFO		8192

//--#define	MAX_MAP_AREAS		256 --> moved to qcommon.h
#define	MAX_MAP_AREAPORTALS	1024
//#define	MAX_MAP_PLANES		65536
//#define	MAX_MAP_NODES		65536
//#define	MAX_MAP_BRUSHSIDES	65536
#define	MAX_MAP_LEAFS		65536
//#define	MAX_MAP_VERTS		65536
#define	MAX_MAP_FACES		65536
//#define	MAX_MAP_LEAFFACES	65536
//#define	MAX_MAP_LEAFBRUSHES 65536
//??#define	MAX_MAP_PORTALS		65536 -- what is it?
//#define	MAX_MAP_EDGES		128000
//#define	MAX_MAP_SURFEDGES	256000
//#define	MAX_MAP_LIGHTING	0x200000
//#define	MAX_MAP_VISIBILITY	0x100000


typedef struct
{
	int		fileofs, filelen;
} lump_t;


typedef enum
{
	LUMP_ENTITIES,
	LUMP_PLANES,
	LUMP_VERTEXES,
	LUMP_VISIBILITY,
	LUMP_NODES,
	LUMP_TEXINFO,
	LUMP_FACES,
	LUMP_LIGHTING,
	LUMP_LEAFS,
	LUMP_LEAFFACES,
	LUMP_LEAFBRUSHES,
	LUMP_EDGES,
	LUMP_SURFEDGES,
	LUMP_MODELS,
	LUMP_BRUSHES,
	LUMP_BRUSHSIDES,
	LUMP_POP,
	LUMP_AREAS,
	LUMP_AREAPORTALS,

	LUMP_COUNT				// must be last
} q2bsplumps_t;


typedef struct
{
	int		ident;
	int		version;
	lump_t	lumps[LUMP_COUNT];
} dBsp2Hdr_t;

typedef struct
{
	CBox	bounds;
	CVec3	origin2;				// unused! (for sounds or lights ?)
	int		headnode;
	int		firstface, numfaces;	// submodels just draw faces without walking the bsp tree
} dmodel_t;


typedef struct
{
	CVec3	point;
} dvertex_t;


// planes (x&~1) and (x&~1)+1 are always opposites

typedef struct
{
	CVec3	normal;
	float	dist;
	int		type;					// PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
} dplane_t;


#define MAX_TREE_DEPTH	512


typedef struct
{
	int		planenum;
	int		children[2];		// negative numbers are -(leafs+1), not nodes
	short	mins[3];			// for frustom culling
	short	maxs[3];
	unsigned short firstface;
	unsigned short numfaces;	// counting both sides
} dnode_t;


typedef struct
{
	struct {					// axis for s/t computation
		CVec3	vec;
		float	offset;
	} vecs[2];
	int		flags;				// miptex flags + overrides
	int		value;				// light emission, etc
	char	texture[32];		// texture name (textures/*.wal)
	int		nexttexinfo;		// for animations, -1 = end of chain
} texinfo_t;


// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
typedef struct
{
	unsigned short	v[2];		// vertex numbers
} dedge_t;

#define	MAXLIGHTMAPS	4
typedef struct
{
	unsigned short planenum;
	short	side;

	int		firstedge;			// we must support > 64k edges
	short	numedges;
	short	texinfo;

	// lighting info
	byte	styles[MAXLIGHTMAPS];
	int		lightofs;			// start of [numstyles*surfsize] samples
} dface_t;

typedef struct
{
	int		contents;			// OR of all brushes (not needed?)

	short	cluster;
	short	area;

	short	mins[3];			// for frustum culling
	short	maxs[3];

	unsigned short firstleafface;
	unsigned short numleaffaces;

	unsigned short firstleafbrush;
	unsigned short numleafbrushes;
} dleaf_t;

typedef struct
{
	unsigned short planenum;	// facing out of the leaf
	short	texinfo;
} dbrushside_t;

typedef struct
{
	int		firstside;
	int		numsides;
	int		contents;
} dbrush_t;

#define	ANGLE_UP	-1
#define	ANGLE_DOWN	-2


// the visibility lump consists of a header with a count, then
// byte offsets for the PVS and PHS of each cluster, then the raw
// compressed bit vectors
#define	DVIS_PVS	0
#define	DVIS_PHS	1
typedef struct
{
	int		numclusters;
	int		bitofs[8][2];	// bitofs[numclusters][2]
} dvis_t;

// each area has a list of portals that lead into other areas
// when portals are closed, other areas may not be visible or
// hearable even if the vis info says that it should be
typedef struct
{
	int		portalnum;
	int		otherarea;
} dareaportal_t;

typedef struct
{
	int		numareaportals;
	int		firstareaportal;
} darea_t;


/*============================================================================

	FOREIGN MAP SUPPORT STRUCTURES/FLAGS

============================================================================*/

//................ Kingpin stuff .......................
// materials

#define SURF_WATER		0x00080000
#define SURF_CONCRETE	0x00100000
#define SURF_FABRIC		0x00200000
#define SURF_GRAVEL		0x00400000
#define SURF_METAL		0x00800000
#define SURF_METAL_L	0x01000000
#define SURF_SNOW		0x02000000
#define SURF_TILE		0x04000000
#define SURF_WOOD		0x08000000

#define SURF_KP_MATERIAL	0x0FF80000

//............... Half-Life stuff ......................
#define BSPHL_VERSION	30

/* Marks:
 * "*" - same as Quake2
 * "?" - almost as in Quake2 (check if bugs)
 * "!" - different
 */
typedef enum
{
	HL_LUMP_ENTITIES,
	HL_LUMP_PLANES,		//*  dplane_t
	HL_LUMP_TEXTURES,
	HL_LUMP_VERTEXES,	//*  dvertex_t
	HL_LUMP_VISIBILITY,	//??
	HL_LUMP_NODES,		//?  dnode_t / hl_dnode_t (int/short children[2])
	HL_LUMP_TEXINFO,
	HL_LUMP_FACES,		//?  dface_t
	HL_LUMP_LIGHTING,	//*  byte[3]
	HL_LUMP_CLIPNODES,
	HL_LUMP_LEAFS,		//!!  dleaf_t / hl_dleaf_t
	HL_LUMP_MARKSURFACES,	//*  short
	HL_LUMP_EDGES,		//*  dedge_t
	HL_LUMP_SURFEDGES,	//*  int
	HL_LUMP_MODELS,		//?  dmodel_t / hl_dmodel_t (HL has unused "visleafs" and headnode[1..3])

	HL_LUMP_COUNT		// must be last
} hlbsplumps_t;


typedef struct
{
	int		version;
	lump_t	lumps[HL_LUMP_COUNT];
} dHlHdr_t;

#define	HL_MAX_MAP_HULLS	4

typedef struct
{
	CBox	bounds;
	CVec3	origin;
	int		headnode[HL_MAX_MAP_HULLS]; // used mostly headnode[0]; other - for entity clipping
	int		visleafs;				// not including the solid leaf 0 (UNUSED field!)
	int		firstface, numfaces;
} hl_dmodel_t;


#define	HL_NUM_AMBIENTS				4	// automatic ambient sounds

#define	HL_CONTENTS_EMPTY			-1
#define	HL_CONTENTS_SOLID			-2
#define	HL_CONTENTS_WATER			-3
#define	HL_CONTENTS_SLIME			-4
#define	HL_CONTENTS_LAVA			-5
#define	HL_CONTENTS_SKY				-6
#define	HL_CONTENTS_ORIGIN			-7	// removed at csg time
#define	HL_CONTENTS_CLIP			-8	// changed to contents_solid

#define	HL_CONTENTS_CURRENT_0		-9
#define	HL_CONTENTS_CURRENT_90		-10
#define	HL_CONTENTS_CURRENT_180		-11
#define	HL_CONTENTS_CURRENT_270		-12
#define	HL_CONTENTS_CURRENT_UP		-13
#define	HL_CONTENTS_CURRENT_DOWN	-14

#define HL_CONTENTS_TRANSLUCENT		-15


typedef struct
{
	int		nummiptex;
	int		dataofs[4];			// dataofs[nummiptex]
} hl_dmiptexlump_t;


typedef struct
{
	char	name[16];
	unsigned width, height;
	unsigned offsets[MIPLEVELS]; // four mip maps stored (==0 if external WAD file)
} hl_miptex_t;


typedef struct
{
	float	vecs[2][4];		// [s/t][xyz offset]
	int		miptex;
	int		flags;
} hl_texinfo_t;


// leaf 0 is the generic CONTENTS_SOLID leaf, used for all solid areas
// all other leafs need visibility info
typedef struct
{
	int		contents;		//!! MUST be converted
	int		visofs;			// -1 = no visibility info

	short	mins[3];		// for frustum culling
	short	maxs[3];

	unsigned short firstmarksurface;
	unsigned short nummarksurfaces;

	byte	ambient_level[HL_NUM_AMBIENTS];	//?? unused now
} hl_dleaf_t;


typedef struct
{
	int		planenum;
	short	children[2];	// negative numbers are -(leafs+1), not nodes
	short	mins[3];		// for sphere culling
	short	maxs[3];
	unsigned short firstface;
	unsigned short numfaces; // counting both sides
} hl_dnode_t;


typedef struct
{
	int		planenum;
	short	children[2];	// negative numbers are contents
} hl_dclipnode_t;
