/*=============================================================================

	Map and model stuff

=============================================================================*/

#ifndef CMODEL_H
#define CMODEL_H

#include "FileFormats.h"


// CBspModel.flags
#define CMODEL_ALPHA		0x0001		// all model surfaces will have SURF_ALPHA flag (kingpin)
#define CMODEL_MOVABLE		0x0002		// non-static inline model
#define CMODEL_SHOW			0x0004		// will not be sent to game, should draw automatically (func_illusionary in q1/hl)
#define CMODEL_WALL			0x0008		// will not be sent to game, static collision model
#define CMODEL_NODRAW		0x0010		// do not draw model even when have visible surfaces
#define CMODEL_CONTENTS		0x0020		// "CBspModel.contents" contains real model contents
#define CMODEL_SCROLL		0x0040		// allow SURF_FLOWING on model surfaces (hl)

#define CMODEL_LOCAL		(CMODEL_SHOW|CMODEL_WALL)	// flags of models, not sent to game (processed by engine)

struct CBspModel
{
	CBox		bounds;
	float		radius;
	int			headnode;
	int			firstFace, numFaces;
	unsigned	flags;					// set of CMODEL_XXX flags
	unsigned	contents;				// contents of all model brushes; used only with CMODEL_CONTENTS
	color_t		color;					// set by Half-Life entities
	CVec3		origin;
};


struct cbrush_t;						//?? forward; change to CBrush?
struct CBspNode;						// forward

struct CBspBaseNode
{
	bool		isLeaf:1;
	int			num:31;					// node or leaf number
	CBspNode	*parent;				// parent node or NULL for root
	CBox		bounds;					// for culling
};

// empty declarations (may be overrided with renderer)
struct CBspNodeExtra;
struct CBspLeafExtra;

// NOTE: we placed field "ex" first in both leaf and node strucs, this allows us
//   to use some common fields in CBspNodeExtra and CBspLeafExtra strucs without
//   checking, whether struc is leaf or node
struct CBspNode : public CBspBaseNode
{
	CBspNodeExtra *ex;					// first
	CBspNode	*children[2];			// really, CBspBaseNode, but using CBspNode allows us to reduce text size
	CPlane		*plane;
};

struct CBspLeaf : public CBspBaseNode
{
	CBspLeafExtra *ex;					// first
	unsigned	contents;
	int			cluster;
	int			area;
	unsigned	firstFace, numFaces;
	unsigned	numBrushes;
	cbrush_t	**brushes;
};


struct lightFlare_t
{
	CVec3		origin;
	float		size;
	float		radius;					// -1 for sunflare
	color_t		color;
	byte		style;
	int			model;
	lightFlare_t *next;
};


// static map light

enum slightType_t
{
	// corresponds to "falloff" of arghrad
	sl_linear, sl_inverse, sl_inverse2, sl_nofade
};

struct slight_t
{
	slightType_t type;
	slight_t	*next;
	bool		spot;
	bool		sun;
	byte		style;
	CVec3		origin;
	CVec3		color;
	float		intens;
	// arghrad fields
	float		focus;
	float		fade;					// for linear lights only: scale the distance
	// for spotlights
	float		spotDot;
	CVec3		spotDir;				// used for sunlight too
};

// static map effects

struct splash_t
{
	CVec3		origin;
	splash_t	*next;
};


enum mapType_t
{
	map_q2, map_kp, map_q1, map_hl, map_q3
};

enum fogMode_t
{
	fog_no, fog_linear, fog_exp, fog_exp2
};


struct bspfile_t
{
	TString<64>	Name;					// mapname
	byte		*file;					// buffer, returned by FS_LoadFile()
	bool		clientLoaded;			// for synchronization of server/client/renderer map loading
	mapType_t	type;
	unsigned	checksum;
	unsigned	length;
	CMemoryChain *extraChain;

	const char	*entStr;

	int			numPlanes;
	union {
		dBsp2Plane_t *planes2;
		dBsp3Plane_t *planes3;
	};
	CPlane		*planes;				// generated

	int			numVertexes;
	union {
		CVec3       *vertexes2;
		dBsp3Vert_t *vertexes3;
	};

	int			numIndexes;
	unsigned	*indexes3;

	int			numModels;
	CBspModel	*models;

	int			lightDataSize;
	byte		*lighting;

	int			numFaces;
	union {
		dBspFace_t  *faces2;
		dBsp3Face_t *faces3;
	};

	//?? access to this as methods; remove CM_ClusterPVS(); same for renderer
	int			visRowSize;
	const byte	*visInfo;				// byte[cluster][visRowSize]; NULL when no visinfo
	int			numClusters;			// 0 when no visinfo

	int			numNodes;
	union {
		dBsp1Node_t	*nodes1;
		dBsp2Node_t	*nodes2;
		dBsp3Node_t *nodes3;
	};
	CBspNode	*nodes;					// [numNodes]; generated

	int			numLeafs;
	union {
		dBsp1Leaf_t *leafs1;
		dBsp2Leaf_t	*leafs2;
		dBsp3Leaf_t *leafs3;
	};
	CBspLeaf	*leafs;					// [numLeafs]; generated

	int			numLeaffaces;
	union {
		unsigned short *leaffaces2;
		unsigned int   *leaffaces3;
	};

	int			numLeafbrushes;
	union {
		unsigned short *leafbrushes2;
		unsigned int   *leafbrushes3;
	};

	int			numEdges;
	dEdge_t		*edges;

	int			numSurfedges;
	int			*surfedges;

	int			numBrushes;
	union {
		dBsp2Brush_t *brushes2;
		dBsp3Brush_t *brushes3;
	};

	int			numBrushSides;
	union {
		dBsp2Brushside_t *brushsides2;
		dBsp3Brushside_t *brushsides3;
	};

	int			numAreas;
	dArea_t		*areas;

	int			numAreaportals;
	dAreaPortal_t *areaportals;

	int			numTexinfo;
	union {
		dBsp1Texinfo_t *texinfo1;		// note: texinfo2 will be generated from texinfo1 for Q1/HL map
		dBsp2Texinfo_t *texinfo2;
		dBsp3Shader_t  *texinfo3;
	};
	csurface_t	*surfaces;				// generated

	dBsp1MiptexLump_t *miptex1;
	TString<128> Wads;

	//--------- extended data -----------------------------

	int			numFlares;
	lightFlare_t *flares;
	slight_t	*slights;

	// static effects
	splash_t	*splashes;				// target_splash entities

	// fog
	//?? remove
	fogMode_t	fogMode;
	CVec3		fogColor;
	union {
		struct {	// exp/exp2
			float	fogDens;
		};
		struct {	// linear
			float	fogStart;
			float	fogEnd;
		};
	};

	int			sunCount;				// 0 -- no sun
	CVec3		sunAmbient;				// ambient light from sky surfaces
	CVec3		sunSurface;
	CVec3		ambientLight;			// global ambient light
};


extern bspfile_t bspfile;


//--bspfile_t *LoadBspFile (char *filename, bool clientload, unsigned *checksum);

#if !NO_DEBUG
CBrush *CM_BuildBrush (int brushNum, CMemoryChain *mem);
#endif

const char *ProcessEntstring (const char *entString);

const CBspLeaf *CM_FindLeaf (const CVec3 &p, int headnode = 0);
// call with topnode set to the headnode, returns with topnode
// set to the first node that splits the box
int CM_BoxLeafs (const CBox &bounds, CBspLeaf **list, int listsize, int *topnode = NULL, int headnode = 0);


#endif // CMODEL_H
