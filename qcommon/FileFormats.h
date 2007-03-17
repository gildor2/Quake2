#ifndef FILE_FORMATS_H
#define FILE_FORMATS_H

//?? may be, hide some strucs/consts (at least, for md2/md3) inside special namespaces
//?? (e.g. DMd2::Vert, DMd3::Hdr etc)

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
	byte	v[3];					// scaled byte to fit in frame mins/maxs
	byte	lightnormalindex;
};

struct dMd2Frame_t
{
	float	scale[3];				// multiply byte verts by this
	float	translate[3];			// then add this
	char	name[16];				// frame name from grabbing
	dMd2Vert_t verts[1];			// variable sized
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
	unsigned ident;					// MD2_IDENT
	unsigned version;				// MD2_VERSION

	int		skinWidth;				// unused
	int		skinHeight;				// unused
	int		frameSize;				// byte size of each frame

	int		numSkins;
	int		numXyz;
	int		numSt;					// greater than num_xyz for seams
	int		numTris;
	int		numGlcmds;				// dwords in strip/fan command list
	int		numFrames;

	int		ofsSkins;				// char [MD2_MAX_SKINNAME][numSkins]
	int		ofsSt;					// unused; offset -> dMd2St_t [numSt]
	int		ofsTris;				// unused; offset -> dTriangle_t [numTris]
	int		ofsFrames;				// offset -> dMd2Frame_t [numFrames]
	int		ofsGlcmds;				// offset -> glcmd stream
	int		ofsEnd;					// end of file
};


/*-----------------------------------------------------------------------------
	.MD3 model file format
-----------------------------------------------------------------------------*/

#define MD3_IDENT			BYTES4('I','D','P','3')
#define MD3_VERSION			15

#define MD3_MAX_LODS		4
#define	MD3_MAX_TRIANGLES	8192	// per surface
#define MD3_MAX_VERTS		4096	// per surface

#define MD3_MAX_SHADERS		256		// per surface
#define MD3_MAX_FRAMES		1024	// per model
#define	MD3_MAX_SURFACES	32		// per model
#define MD3_MAX_TAGS		16		// per frame


#define	MD3_XYZ_SCALE		(1.0f/64)	// vertex scales

struct dMd3Frame_t
{
	CBox	bounds;
	CVec3	localOrigin;			// NOTE: q3 model converter tool always sets this to (0,0,0)
	float	radius;					//		 and this is a maximal distance to all bounds[] vectors
	char	name[16];
};

struct dMd3Tag_t
{
	char	name[MAX_QPATH];		// tag name
	CCoords	tag;					// CVec3 origin + axis[3]
};

struct dMd3Shader_t
{
	char	name[MAX_QPATH];
	int		shaderIndex;			// original Q3: for in-game use
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
	short	xyz[3];					// scaled by MD3_XYZ_SCALE
	short	normal;					// encoded: byte[2] -- angles
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
	unsigned ident;					// md3 conversion tool set this to MD3_IDENT
	char	name[MAX_QPATH]; 		// can be "tag_..."
	int		flags;					// unused

	int		numFrames;				// same for all md3 surfaces; == model.numFrames
	int		numShaders;				// same for all md3 surfaces
	int		numVerts;
	int		numTriangles;

	// offsets from start of the current surface
	int		ofsTriangles;			// offset -> dMd3Triangle_t [numTriangles]
	int		ofsShaders;				// offset -> dMd3Shader_t [numShaders]
	int		ofsSt;					// offset -> dMd3St_t [numVerts]
	int		ofsXyzNormals;			// offset -> dMd3XyzNormal_t [numVerts*numFrames]

	int		ofsEnd;					// offset -> next surface
};

struct dMd3_t
{
	unsigned ident;					// MD3_IDENT
	unsigned version;				// MD3_VERSION

	char	name[MAX_QPATH];		// unused
	int		flags;					// unused

	int		numFrames;
	int		numTags;
	int		numSurfaces;

	int		numSkins;				// unused

	int		ofsFrames;				// offset -> dMd3Frame_t [numFrames]
	int		ofsTags;				// offset -> dMd3Tag_t [numTags*numFrames]
	int		ofsSurfaces;			// offset -> dMd3Surface_t [numSurfaces]

	int		ofsEnd;					// end of file
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
	unsigned ident;					// SP2_IDENT
	unsigned version;				// SP2_VERSION
	int		numframes;
	dSp2Frame_t frames[1];			// variable sized
};


/*-----------------------------------------------------------------------------
	.WAL texture file format
-----------------------------------------------------------------------------*/

struct dWal_t
{
	char	name[32];
	unsigned width, height;
	unsigned offsets[4];			// 4 mip maps stored; really, required only [0]
	char	animname[32];			// next frame in animation chain
	unsigned flags;
	unsigned contents;
	int		value;
};



/*-----------------------------------------------------------------------------
	Quake2 .BSP file format
-----------------------------------------------------------------------------*/

#define BSP_IDENT			BYTES4('I','B','S','P')
#define BSP2_VERSION		38


struct lump_t						// common for all quake-based bsp files (q1,q2,q3,hl)
{
	int		fileofs, filelen;
};


struct dBsp2Hdr_t
{
	enum
	{
		LUMP_ENTITIES,				// char[]
		LUMP_PLANES,				// dBsp2Plane_t[]
		LUMP_VERTEXES,				// CVec3[]
		LUMP_VISIBILITY,			// dBsp2Vis_t[] + byte[]
		LUMP_NODES,					// dBsp2Node_t[]
		LUMP_TEXINFO,				// dBsp2Texinfo_t[]
		LUMP_FACES,					// dBspFace_t[]
		LUMP_LIGHTING,				// byte[]
		LUMP_LEAFS,					// dBsp2Leaf_t[]
		LUMP_LEAFFACES,				// short[]
		LUMP_LEAFBRUSHES,			// short[]
		LUMP_EDGES,					// dEdge_t[]
		LUMP_SURFEDGES,				// int[]
		LUMP_MODELS,				// dBsp2Model_t[]
		LUMP_BRUSHES,				// dBsp2Brush_t[]
		LUMP_BRUSHSIDES,			// dBsp2Brushside_t[]
		LUMP_POP,					// ?
		LUMP_AREAS,					// dArea_t[]
		LUMP_AREAPORTALS,			// dAreaPortal_t[]

		LUMP_COUNT					// should be last
	};

	unsigned ident;					// BSP_IDENT
	unsigned version;				// BSP2_VERSION
	lump_t	lumps[LUMP_COUNT];
};

struct dBsp2Model_t
{
	CBox	bounds;
	CVec3	origin;					// unused
	int		headnode;
	int		firstface, numfaces;	// submodels just draw faces without walking the bsp tree
};


// planes (x&~1) and (x&~1)+1 are always opposites

// the same as CPlane, but "byte type,signbits,pad[2]") -> "int type"
struct dBsp2Plane_t
{
	CVec3	normal;
	float	dist;
	int		type;					// useless - will be recomputed on map loading
};


#define MAX_TREE_DEPTH	512

struct dBsp2Node_t
{
	int		planeNum;
	int		children[2];			// negative numbers are -(leafs+1), not nodes
	short	mins[3], maxs[3];		// for frustom culling
	short	firstFace, numFaces;	// unused
};

struct dBsp2Leaf_t
{
	unsigned contents;				// OR of all brushes (not needed?)

	short	cluster;
	short	area;

	short	mins[3], maxs[3];		// for frustum culling

	unsigned short firstleafface, numleaffaces;
	unsigned short firstleafbrush, numleafbrushes;
};

struct dBsp2Texinfo_t
{
	struct {						// axis for s/t computation
		CVec3	vec;
		float	offset;
	} vecs[2];
	unsigned flags;					// miptex flags + overrides
	int		value;					// light emission
	char	texture[32];			// texture name (textures/*.wal)
	int		nexttexinfo;			// for animations, -1 = end of chain
};


// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
struct dEdge_t
{
	unsigned short v[2];			// vertex numbers
};

#define	MAXLIGHTMAPS	4
struct dBspFace_t
{
	unsigned short planenum;
	short	side;

	int		firstedge;				// we must support > 64k edges
	short	numedges;
	short	texinfo;

	// lighting info
	byte	styles[MAXLIGHTMAPS];
	int		lightofs;				// start of [numstyles*surfsize] samples
};

struct dBsp2Brush_t
{
	int		firstside;
	int		numsides;
	unsigned contents;
};

struct dBsp2Brushside_t
{
	unsigned short planenum;		// facing out of the leaf
	short	texinfo;				// may be -1 (no texinfo)
};

// the visibility lump consists of a header with a count, then
// byte offsets for the PVS and PHS of each cluster, then the raw
// compressed bit vectors
struct dBsp2Vis_t
{
	enum
	{
		PVS,
		PHS							// unused
	};

	int		numClusters;
	int		bitOfs[8][2];			// bitOfs[numClusters][2]
};

// each area has a list of portals that lead into other areas
// when portals are closed, other areas may not be visible or
// hearable even if the vis info says that it should be
struct dAreaPortal_t
{
	int		portalNum;
	int		otherArea;
};

struct dArea_t
{
	int		numAreaPortals;
	int		firstAreaPortal;
};


/*-----------------------------------------------------------------------------
	Kingpin extensions for Q2 BSP
-----------------------------------------------------------------------------*/
// materials

#define SURF_WATER			0x00080000
#define SURF_CONCRETE		0x00100000
#define SURF_FABRIC			0x00200000
#define SURF_GRAVEL			0x00400000
#define SURF_METAL			0x00800000
#define SURF_METAL_L		0x01000000
#define SURF_SNOW			0x02000000
#define SURF_TILE			0x04000000
#define SURF_WOOD			0x08000000

#define SURF_KP_MATERIAL	0x0FF80000


/*-----------------------------------------------------------------------------
	Quake1/Half-Life .BSP file format
-----------------------------------------------------------------------------*/

#define BSP1_VERSION		29
#define BSPHL_VERSION		30

struct dBsp1Hdr_t
{
	enum
	{
		LUMP_ENTITIES,				// char[]
		LUMP_PLANES,				// dBsp2Plane_t[]
		LUMP_TEXTURES,
		LUMP_VERTEXES,				// CVec3[]
		LUMP_VISIBILITY,
		LUMP_NODES,					// dBsp1Node_t (q2->q1: int->short children[2])
		LUMP_TEXINFO,				// dBsp1Texinfo_t[]
		LUMP_FACES,					// dBspFace_t
		LUMP_LIGHTING,				// byte[] (q1: monochrome, hl: rgb == q2)
		LUMP_CLIPNODES,
		LUMP_LEAFS,					// dBsp1Leaf_t[]
		LUMP_MARKSURFACES,			// short[] (== Q2LUMP_LEAFFACES)
		LUMP_EDGES,					// dEdge_t[]
		LUMP_SURFEDGES,				// int[]
		LUMP_MODELS,				// dBsp1Model_t[] (almost as in q2)

		LUMP_COUNT					// should be last
	};

	unsigned version;				// BSP1_VERSION or BSPHL_VERSION
	lump_t	lumps[LUMP_COUNT];
};

#define	Q1_MAP_HULLS		4

struct dBsp1Model_t
{
	CBox	bounds;
	CVec3	origin;					// unused
	int		headnode;				// separated from headnode[Q1_MAP_HULLS] to be name-compatible with dBsp*Model_t
	int		headnode2[Q1_MAP_HULLS-1]; // secondary clipping hulls; not used here
	int		visleafs;				// not including the solid leaf 0 (UNUSED field!)
	int		firstface, numfaces;
};


#define	Q1_NUM_AMBIENTS		4		// automatic ambient sounds

// Q1/HL contents
#define	Q1_CONTENTS_EMPTY			-1
#define	Q1_CONTENTS_SOLID			-2
#define	Q1_CONTENTS_WATER			-3
#define	Q1_CONTENTS_SLIME			-4
#define	Q1_CONTENTS_LAVA			-5
#define	Q1_CONTENTS_SKY				-6
// added in Half-Life
#define	Q1_CONTENTS_ORIGIN			-7	// removed at csg time
#define	Q1_CONTENTS_CLIP			-8	// changed to CONTENTS_SOLID at csg time; presents in hull[idx>=1] only (absent in hull[0])
#define	Q1_CONTENTS_CURRENT_0		-9
#define	Q1_CONTENTS_CURRENT_90		-10
#define	Q1_CONTENTS_CURRENT_180		-11
#define	Q1_CONTENTS_CURRENT_270		-12
#define	Q1_CONTENTS_CURRENT_UP		-13
#define	Q1_CONTENTS_CURRENT_DOWN	-14

#define Q1_CONTENTS_TRANSLUCENT		-15
#define Q1_CONTENTS_LADDER			-16	// HL, set by func_ladder


struct dBsp1MiptexLump_t
{
	int		nummiptex;
	int		dataofs[1];				// dataofs[nummiptex]; offsets from start of this lump
};


struct dBsp1Miptex_t
{
	char	name[16];
	unsigned width, height;
	unsigned offsets[4];			// 4 mip maps stored (==0 if external WAD file in HL); note: q1 ignores offset
									// value, it uses data after structure end
};


struct dBsp1Texinfo_t
{
	struct {						// axis for s/t computation
		CVec3	vec;
		float	offset;
	} vecs[2];
	int		miptex;
	unsigned flags;					// 0 or TEX_SPECIAL==1 only (no significant meaning); unused
};


struct dBsp1Node_t
{
	int		planeNum;
	short	children[2];			// negative numbers are -(leafs+1), not nodes
	short	mins[3], maxs[3];		// for frustum culling
	short	firstFace, numFaces;	// unused
};


// leaf 0 is the generic CONTENTS_SOLID leaf, used for all solid areas
// all other leafs need visibility info
struct dBsp1Leaf_t
{
	int		contents;				// one of Q1_CONTENTS_XXXX
	int		visofs;					// -1 => no visibility info

	short	mins[3], maxs[3];		// for frustum culling

	unsigned short firstleafface;	// original name: "firstmarksurface"
	unsigned short numleaffaces;	// original name: "nummarksurfaces"

	byte	ambient_level[Q1_NUM_AMBIENTS];
};


struct dBsp1Clipnode_t				// unused
{
	int		planenum;
	short	children[2];			// negative numbers are contents
};


/*-----------------------------------------------------------------------------
	Quake3 .BSP file format
-----------------------------------------------------------------------------*/

#define BSP3_VERSION		46

struct dBsp3Hdr_t
{
	enum
	{
		LUMP_ENTITIES,				// char[]
		LUMP_SHADERS,				// dBsp3Shader_t[]; used instead of TEXINFO
		LUMP_PLANES,				// dBsp3Plane_t[]
		LUMP_NODES,					// dBsp3Node_t[]
		LUMP_LEAFS,					// dBsp3Leaf_t[]
		LUMP_LEAFFACES,				// int[]
		LUMP_LEAFBRUSHES,			// int[]
		LUMP_MODELS,				// dBsp3Model_t[]
		LUMP_BRUSHES,				// dBsp3Brush_t[]
		LUMP_BRUSHSIDES,			// dBsp3Brushside_t[]
		LUMP_VERTEXES,				// dBsp3Vert_t[]
		LUMP_INDEXES,				// int[]
		LUMP_FOGS,					// dBsp3Fog_t
		LUMP_FACES,					// dBsp3Face_t[]
		LUMP_LIGHTMAPS,				// byte[]
		LUMP_LIGHTGRID,				// unused
		LUMP_VISIBILITY,			// dBsp3Vis_t[] + byte[]

		LUMP_COUNT					// should be last
	};

	unsigned ident;					// BSP_IDENT
	unsigned version;				// BSP3_VERSION
	lump_t	lumps[LUMP_COUNT];
};

// comparision with dBsp2Model_t: without usuded "origin" + use brushes instead of headnode
struct dBsp3Model_t
{
	CBox	bounds;
	int		firstface, numfaces;	// submodels just draw faces without walking the bsp tree
	int		firstBrush, numBrushes;
};

// the same as CPlane, but removed "byte type,signbits,pad[2]"
struct dBsp3Plane_t
{
	CVec3	normal;
	float	dist;
};

// comparision with dBsp2Node_t: mins/maxs short[]->int[], and removed unused firstface/numfaces
struct dBsp3Node_t
{
	int		planeNum;
	int		children[2];			// negative numbers are -(leafs+1), not nodes
	int		mins[3], maxs[3];		// for frustom culling
};

struct dBsp3Leaf_t
{
	int		cluster;
	int		area;

	int		mins[3], maxs[3];		// for frustum culling

	int 	firstleafface, numleaffaces;
	int 	firstleafbrush, numleafbrushes;
};

// texinfo analog
struct dBsp3Shader_t
{
	char	name[MAX_QPATH];
	unsigned surfaceFlags;
	unsigned contentFlags;
};

#define Q3_LIGHTMAP_SIZE	128

struct dBsp3Face_t
{
	enum
	{
		BAD, PLANAR, PATCH, TRISURF, FLARE
	};

	int		shaderNum;
	int		fogNum;
	int		surfaceType;			// value from enum above
	int		firstVert, numVerts;
	int		firstIndex, numIndexes;
	int		lightmapNum;
	int		lightmapParms[4];		// X, Y, width, height -- unused
	CVec3	flareOrigin;			// for FLARE
	CVec3	lightmapVecs[3];		// [2] - normal; for FLARE [0] - color; for PATCH [0,1] - bounds
	int		patchWidth, patchHeight;
};

// comparision with dBsp2Brush_t: int contents -> shaderNum
struct dBsp3Brush_t
{
	int		firstside;
	int		numsides;
	int		shaderNum;				// for contents info
};

struct dBsp3Brushside_t
{
	int		planenum;				// facing out of the leaf
	int		shaderNum;				//?? which shaderNum used: from brushside or from brush ?
};

struct dBsp3Vert_t
{
	CVec3	v;
	float	st[2];
	float	lm[2];
	CVec3	normal;
	color_t	c;
};

struct dBsp3Fog_t
{
	char	name[MAX_QPATH];		// shader name
	int		brushNum;
	int		visibleSide;			// -1 == none
};

// the visibility lump consists of a header with a count, then the raw compressed bit vectors
struct dBsp3Vis_t
{
	int		numClusters;
	int		rowSize;
};

#define Q3_CONTENTS_SOLID			0x000001
#define Q3_CONTENTS_LAVA			0x000002
#define	Q3_CONTENTS_SLIME			0x000010
#define	Q3_CONTENTS_WATER			0x000020
#define	Q3_CONTENTS_FOG				0x000040
#define	Q3_CONTENTS_PLAYERCLIP		0x010000
#define	Q3_CONTENTS_MONSTERCLIP		0x020000


#endif // FILE_FORMATS_H
