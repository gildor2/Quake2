/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

//
// qfiles.h: quake file formats
// This file must be identical in the quake and utils directories
//

/*
========================================================================

The .pak files are just a linear collapse of a directory tree

========================================================================
*/

#define IDPAKHEADER		('P'+('A'<<8)+('C'<<16)+('K'<<24))

typedef struct
{
	char	name[56];
	int		filepos, filelen;
} dPackFile_t;

typedef struct
{
	int		ident;		// == IDPAKHEADER
	int		dirofs;
	int		dirlen;
} dPackHeader_t;

//#define	MAX_FILES_IN_PACK	4096	// unlimited now


/*
========================================================================

.MD2 triangle model file format

========================================================================
*/

#define MD2_IDENT			('I'+('D'<<8)+('P'<<16)+('2'<<24))
#define MD2_VERSION			8

/*
#define	MAX_TRIANGLES		4096
#define MAX_VERTS			2048
#define MAX_FRAMES			512
*/
#define MD2_MAX_SKINS		32
#define	MD2_MAX_SKINNAME	64

typedef struct
{
	short	s;
	short	t;
} dStVert_t;

typedef struct
{
	short	indexXyz[3];
	short	indexSt[3];
} dTriangle_t;

typedef struct
{
	byte	v[3];			// scaled byte to fit in frame mins/maxs
	byte	lightnormalindex;
} dTriVertx_t;

typedef struct
{
	float	scale[3];		// multiply byte verts by this
	float	translate[3];	// then add this
	char	name[16];		// frame name from grabbing
	dTriVertx_t verts[1];	// variable sized
} dAliasFrame_t;


/* The glcmd format:
 *   - a positive integer starts a tristrip command, followed by that many
 *     vertex structures.
 *   - a negative integer starts a trifan command, followed by -x vertexes
 *   - a zero indicates the end of the command list.
 * A vertex consists of a floating point s, a floating point t,
 *   and an integer vertex index.
 */

typedef struct
{
	int		ident;
	int		version;

	int		skinWidth;
	int		skinHeight;
	int		frameSize;		// byte size of each frame

	int		numSkins;
	int		numXyz;
	int		numSt;			// greater than num_xyz for seams
	int		numTris;
	int		numGlcmds;		// dwords in strip/fan command list
	int		numFrames;

	int		ofsSkins;		// each skin is a MD2_MAX_SKINNAME string
	int		ofsSt;			// byte offset from start for stverts
	int		ofsTris;		// offset for dtriangles
	int		ofsFrames;		// offset for first frame
	int		ofsGlcmds;
	int		ofsEnd;			// end of file
} dmdl_t;

/*
========================================================================

.MD3 file format

========================================================================
*/

#define MD3_IDENT			(('3'<<24)+('P'<<16)+('D'<<8)+'I')
#define MD3_VERSION			15

#define	MD3_XYZ_SCALE		(1.0f/64)	// vertex scales

#define	MD3_MAX_TRIANGLES	8192		// per surface
#define MD3_MAX_VERTS		4096		// per surface
//!! unfinished!


/*
========================================================================

.SP2 sprite file format

========================================================================
*/

#define SP2_IDENT			('I'+('D'<<8)+('S'<<16)+('2'<<24))
#define SP2_VERSION			2

typedef struct
{
	int		width, height;
	int		origin_x, origin_y;		// raster coordinates inside pic
	char	name[MD2_MAX_SKINNAME];	// name of pcx file
} dsprframe_t;

typedef struct {
	int		ident;
	int		version;
	int		numframes;
	dsprframe_t frames[1];			// variable sized
} dsprite_t;

/*
==============================================================================

  .WAL texture file format

==============================================================================
*/


#define	MIPLEVELS	4
typedef struct miptex_s
{
	char	name[32];
	unsigned width, height;
	unsigned offsets[MIPLEVELS];	// four mip maps stored
	char	animname[32];			// next frame in animation chain
	int		flags;
	int		contents;
	int		value;
} miptex_t;



/*
==============================================================================

  .BSP file format

==============================================================================
*/

#define BSP2_IDENT			('I'+('B'<<8)+('S'<<16)+('P'<<24))
#define BSP2_VERSION		38


// upper design bounds
// leaffaces, leafbrushes, planes, and verts are still bounded by
// 16 bit short limits
#define	MAX_MAP_MODELS		1024
#define	MAX_MAP_BRUSHES		8192
#define	MAX_MAP_ENTITIES	2048
#define	MAX_MAP_ENTSTRING	0x40000
#define	MAX_MAP_TEXINFO		8192

#define	MAX_MAP_AREAS		256
#define	MAX_MAP_AREAPORTALS	1024
#define	MAX_MAP_PLANES		65536
#define	MAX_MAP_NODES		65536
#define	MAX_MAP_BRUSHSIDES	65536
#define	MAX_MAP_LEAFS		65536
#define	MAX_MAP_VERTS		65536
#define	MAX_MAP_FACES		65536
#define	MAX_MAP_LEAFFACES	65536
#define	MAX_MAP_LEAFBRUSHES 65536
#define	MAX_MAP_PORTALS		65536
#define	MAX_MAP_EDGES		128000
#define	MAX_MAP_SURFEDGES	256000
#define	MAX_MAP_LIGHTING	0x200000
#define	MAX_MAP_VISIBILITY	0x100000

// key / value pair sizes

#define	MAX_KEY		32
#define	MAX_VALUE	1024

//=============================================================================

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
} dheader_t;

typedef struct
{
	float	mins[3], maxs[3];
	float	origin[3];				// unused! (for sounds or lights ?)
	int		headnode;
	int		firstface, numfaces;	// submodels just draw faces
									// without walking the bsp tree
} dmodel_t;


typedef struct
{
	float	point[3];
} dvertex_t;


// planes (x&~1) and (x&~1)+1 are always opposites

typedef struct
{
	float	normal[3];
	float	dist;
	int		type;					// PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
} dplane_t;


// contents flags are seperate bits
// a given brush can contribute multiple content bits
// multiple brushes can be in a single leaf

// lower bits are stronger, and will eat weaker brushes completely

#define	CONTENTS_SOLID			0x00000001	// an eye is never valid in a solid
#define	CONTENTS_WINDOW			0x00000002	// translucent, but not watery
#define	CONTENTS_AUX			0x00000004
#define	CONTENTS_LAVA			0x00000008
#define	CONTENTS_SLIME			0x00000010
#define	CONTENTS_WATER			0x00000020
#define	CONTENTS_MIST			0x00000040
#define CONTENTS_ALPHA			0x00000080	// from Kingping - can shoot through this

// remaining contents are non-visible, and don't eat brushes

#define	CONTENTS_AREAPORTAL		0x00008000

#define	CONTENTS_PLAYERCLIP		0x00010000
#define	CONTENTS_MONSTERCLIP	0x00020000

// currents can be added to any other contents, and may be mixed
#define	CONTENTS_CURRENT_0		0x00040000
#define	CONTENTS_CURRENT_90		0x00080000
#define	CONTENTS_CURRENT_180	0x00100000
#define	CONTENTS_CURRENT_270	0x00200000
#define	CONTENTS_CURRENT_UP		0x00400000
#define	CONTENTS_CURRENT_DOWN	0x00800000

#define	CONTENTS_ORIGIN			0x01000000	// removed before bsping an entity

#define	CONTENTS_MONSTER		0x02000000	// should never be on a brush, only in game
#define	CONTENTS_DEADMONSTER	0x04000000
#define	CONTENTS_DETAIL			0x08000000	// brushes to be added after vis leafs
#define	CONTENTS_TRANSLUCENT	0x10000000	// auto set if any surface has trans
#define	CONTENTS_LADDER			0x20000000


#define	SURF_LIGHT		0x0001		// value will hold the light strength

#define	SURF_SLICK		0x0002		// effects game physics

#define	SURF_SKY		0x0004		// don't draw, but add to skybox
#define	SURF_WARP		0x0008		// turbulent water warp
#define	SURF_TRANS33	0x0010
#define	SURF_TRANS66	0x0020
#define	SURF_FLOWING	0x0040		// scroll towards angle
#define	SURF_NODRAW		0x0080		// don't bother referencing the texture

// added since 4.00
// Kingpin (used for non-KP maps too)
#define SURF_ALPHA		0x1000
#define	SURF_SPECULAR	0x4000		// have a bug in KP's q_shared.h: SPECULAR and DIFFUSE consts are 0x400 and 0x800
#define	SURF_DIFFUSE	0x8000

#define SURF_AUTOFLARE	0x2000		// just free flag (should use extra struc for dtexinfo_t !!)


// content masks
#define	MASK_ALL				(-1)
#define	MASK_SOLID				(CONTENTS_SOLID|CONTENTS_WINDOW)
#define	MASK_PLAYERSOLID		(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define	MASK_DEADSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW)
#define	MASK_MONSTERSOLID		(CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define	MASK_WATER				(CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define	MASK_OPAQUE				(CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define	MASK_SHOT				(CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEADMONSTER)
#define MASK_CURRENT			(CONTENTS_CURRENT_0|CONTENTS_CURRENT_90|CONTENTS_CURRENT_180|CONTENTS_CURRENT_270|	\
		CONTENTS_CURRENT_UP|CONTENTS_CURRENT_DOWN)


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


typedef struct texinfo_s
{
	float	vecs[2][4];			// [s/t][xyz offset]
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
} hl_dheader_t;

#define	HL_MAX_MAP_HULLS	4

typedef struct
{
	float	mins[3], maxs[3];
	float	origin[3];
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
