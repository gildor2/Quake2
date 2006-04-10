#ifndef QFILES_H
#define QFILES_H

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


struct lump_t						// common for all quake-based bsp files (q1,q2,q3,hl)
{
	int		fileofs, filelen;
};


enum
{
	Q2LUMP_ENTITIES,				// char[]
	Q2LUMP_PLANES,					// dPlane_t[]
	Q2LUMP_VERTEXES,				// CVec3[]
	Q2LUMP_VISIBILITY,				// dBsp2Vis_t[] + byte[]
	Q2LUMP_NODES,					// dBsp2Node_t[]
	Q2LUMP_TEXINFO,					// dBsp2Texinfo_t[]
	Q2LUMP_FACES,					// dFace_t[]
	Q2LUMP_LIGHTING,				// byte[]
	Q2LUMP_LEAFS,					// dBsp2Leaf_t[]
	Q2LUMP_LEAFFACES,				// short[]
	Q2LUMP_LEAFBRUSHES,				// short[]
	Q2LUMP_EDGES,					// dEdge_t[]
	Q2LUMP_SURFEDGES,				// int[]
	Q2LUMP_MODELS,					// dBsp2Model_t[]
	Q2LUMP_BRUSHES,					// dBsp2Brush_t[]
	Q2LUMP_BRUSHSIDES,				// dBsp2Brushside_t[]
	Q2LUMP_POP,						// ?
	Q2LUMP_AREAS,					// darea_t[]
	Q2LUMP_AREAPORTALS,				// dareaportal_t[]

	Q2LUMP_COUNT					// should be last
};


struct dBsp2Hdr_t
{
	unsigned ident;					// BSP2_IDENT
	unsigned version;				// BSP2_VERSION
	lump_t	lumps[Q2LUMP_COUNT];
};

struct dBsp2Model_t
{
	CBox	bounds;
	CVec3	origin;					// unused! (for sounds or lights ?)
	int		headnode;
	int		firstface, numfaces;	// submodels just draw faces without walking the bsp tree
};


// planes (x&~1) and (x&~1)+1 are always opposites

// the same as cplane_t, but "byte type,signbits,pad[2]") -> "int type"
struct dPlane_t
{
	CVec3	normal;
	float	dist;
	int		type;					// useless - will be recomputed on map loading
};


#define MAX_TREE_DEPTH	512

struct dBsp2Node_t
{
	int		planenum;
	int		children[2];			// negative numbers are -(leafs+1), not nodes
	short	mins[3], maxs[3];		// for frustom culling
	unsigned short firstface;		// unused
	unsigned short numfaces;		// unused
};

struct dBsp2Leaf_t
{
	unsigned contents;				// OR of all brushes (not needed?)

	short	cluster;
	short	area;

	short	mins[3], maxs[3];		// for frustum culling

	unsigned short firstleafface;
	unsigned short numleaffaces;

	unsigned short firstleafbrush;
	unsigned short numleafbrushes;
};

struct dBsp2Texinfo_t
{
	struct {						// axis for s/t computation
		CVec3	vec;
		float	offset;
	} vecs[2];
	unsigned flags;					// miptex flags + overrides
	int		value;					// light emission, etc
	char	texture[32];			// texture name (textures/*.wal)
	int		nexttexinfo;			// for animations, -1 = end of chain
};


// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
struct dEdge_t
{
	unsigned short	v[2];			// vertex numbers
};

#define	MAXLIGHTMAPS	4
struct dFace_t
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
	int		contents;
};

struct dBsp2Brushside_t
{
	unsigned short planenum;		// facing out of the leaf
	short	texinfo;
};

// the visibility lump consists of a header with a count, then
// byte offsets for the PVS and PHS of each cluster, then the raw
// compressed bit vectors
enum
{
	DVIS_PVS,
	DVIS_PHS
};

struct dBsp2Vis_t
{
	int		numclusters;
	int		bitofs[8][2];			// bitofs[numclusters][2]
};

// each area has a list of portals that lead into other areas
// when portals are closed, other areas may not be visible or
// hearable even if the vis info says that it should be
struct dareaportal_t
{
	int		portalnum;
	int		otherarea;
};

struct darea_t
{
	int		numareaportals;
	int		firstareaportal;
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

/* Marks:
 * "*" - same as Quake2
 * "?" - almost as in Quake2 (check if bugs)
 * "!" - different
 */
enum
{
	Q1LUMP_ENTITIES,		//* char[]
	Q1LUMP_PLANES,			//* dPlane_t[]
	Q1LUMP_TEXTURES,
	Q1LUMP_VERTEXES,		//* CVec3[]
	Q1LUMP_VISIBILITY,		//??
	Q1LUMP_NODES,			//?  dBsp2Node_t / hl_dnode_t (int/short children[2])
	Q1LUMP_TEXINFO,			// dBsp1Texinfo_t[]
	Q1LUMP_FACES,			//?  dFace_t
	Q1LUMP_LIGHTING,		//* byte[] (q1: monochrome, hl: rgb)
	Q1LUMP_CLIPNODES,
	Q1LUMP_LEAFS,			//!!  dBsp2Leaf_t / hl_dleaf_t
	Q1LUMP_MARKSURFACES,	//* short[] == Q2LUMP_LEAFFACES
	Q1LUMP_EDGES,			//* dEdge_t[]
	Q1LUMP_SURFEDGES,		//* int[]
	Q1LUMP_MODELS,			//?  dBsp2Model_t / hl_dmodel_t (HL has unused "visleafs" and headnode[1..3])

	Q1LUMP_COUNT			// should be last
};


struct dBsp1Hdr_t
{
	unsigned version;		// BSP1_VERSION or BSPHL_VERSION
	lump_t	lumps[Q1LUMP_COUNT];
};

#define	Q1_MAP_HULLS		4

struct dBsp1Model_t
{
	CBox	bounds;
	CVec3	origin;						//?? unused? (check)
	int		headnode[Q1_MAP_HULLS]; 	// used mostly headnode[0]; other - for entity clipping
	int		visleafs;					// not including the solid leaf 0 (UNUSED field!)
	int		firstface, numfaces;
};


#define	Q1_NUM_AMBIENTS		4			// automatic ambient sounds

// Q1/HL contents
#define	Q1_CONTENTS_EMPTY			-1
#define	Q1_CONTENTS_SOLID			-2
#define	Q1_CONTENTS_WATER			-3
#define	Q1_CONTENTS_SLIME			-4
#define	Q1_CONTENTS_LAVA			-5
#define	Q1_CONTENTS_SKY				-6
// added in Half-Life
#define	Q1_CONTENTS_ORIGIN			-7	// removed at csg time
#define	Q1_CONTENTS_CLIP			-8	// changed to contents_solid
#define	Q1_CONTENTS_CURRENT_0		-9
#define	Q1_CONTENTS_CURRENT_90		-10
#define	Q1_CONTENTS_CURRENT_180		-11
#define	Q1_CONTENTS_CURRENT_270		-12
#define	Q1_CONTENTS_CURRENT_UP		-13
#define	Q1_CONTENTS_CURRENT_DOWN	-14

#define Q1_CONTENTS_TRANSLUCENT		-15


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
	unsigned flags;					// 0 or TEX_SPECIAL==1 only (no significant meaning)
};


struct dBsp1Node_t
{
	int		planenum;
	short	children[2];			// negative numbers are -(leafs+1), not nodes
	short	mins[3], maxs[3];		// for frustum culling
	unsigned short firstface;		// unused
	unsigned short numfaces;		// unused
};


// leaf 0 is the generic CONTENTS_SOLID leaf, used for all solid areas
// all other leafs need visibility info
struct dBsp1Leaf_t
{
	int		contents;				// one of Q1_CONTENTS_XXXX
	int		visofs;					// -1 => no visibility info

	short	mins[3], maxs[3];		// for frustum culling

	unsigned short firstleafface;	// old name: "firstmarksurface"
	unsigned short numleaffaces;	// old name: "nummarksurfaces"

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

// NOTE: dbrush_t and dbrushside_t == dBsp2... with exception: texinfo -> shader
// dheader_t == dBsp2Hdr_t
// dnode_t == dBsp2Node_t w/o unused fields (firstface/numfaces)


#endif // QFILES_H
