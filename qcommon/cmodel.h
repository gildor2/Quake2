#ifndef CMODEL_H
#define CMODEL_H

#include "qfiles.h"


// cmodel_t.flags
#define CMODEL_ALPHA	1
#define CMODEL_MOVABLE	2

struct cmodel_t
{
	CBox	bounds;
	float	radius;
	int		headnode;
	int		firstface, numfaces;
	unsigned flags;
};


/*-----------------------------------------------------------------------------
	Map and model stuff
-----------------------------------------------------------------------------*/

struct lightFlare_t
{
	CVec3	origin;
	float	size;
	float	radius;					// -1 for sunflare
	color_t	color;
	byte	style;
	int		model;
	lightFlare_t *next;
};


// static map light

typedef enum {sl_linear, sl_inverse, sl_inverse2} slightType_t;

struct slight_t
{
	slightType_t type;
	bool	spot;					// bool
	byte	style;
	CVec3	origin;
	CVec3	color;
	float	intens;
	// arghrad fields
	float	focus;
	float	fade;					// for linear lights only: scale the distance
	// for spotlights
	float	spotDot;
	CVec3	spotDir;
	slight_t *next;
};

// static map effects

struct splash_t
{
	CVec3	origin;
	splash_t *next;
};


typedef enum {map_q2, map_kp, map_q1, map_hl} mapType_t;
typedef enum {fog_no, fog_linear, fog_exp, fog_exp2} fogMode_t;


struct bspfile_t
{
	char		name[MAX_QPATH];		// mapname
	byte		*file;					// buffer, returned by FS_LoadFile()
	mapType_t	type;
	unsigned	checksum;
	unsigned	length;
	CMemoryChain *extraChain;

	int			entStrSize;
	const char	*entStr;

	int			numPlanes;
	dPlane_t	*planes;

	int			numVertexes;
	CVec3		*vertexes;

	int			numModels;
	cmodel_t	*models;

	int			lightDataSize;
	byte		*lighting;

	int			numFaces;
	dFace_t		*faces;

	int			visDataSize;
	dBsp2Vis_t	*vis;

	int			numNodes;
	union {
		dBsp2Node_t	*nodes2;
		dBsp1Node_t	*nodes1;
	};

	int			numLeafs;
	union {
		dBsp2Leaf_t	*leafs2;
		dBsp1Leaf_t *leafs1;
	};

	int			numLeaffaces;
	unsigned short *leaffaces;

	int			numLeafbrushes;
	unsigned short *leafbrushes;

	int			numEdges;
	dEdge_t		*edges;

	int			numSurfedges;
	int			*surfedges;

	int			numBrushes;
	dBsp2Brush_t *brushes;

	int			numBrushsides;
	dBsp2Brushside_t *brushsides;

	int			numAreas;
	darea_t		*areas;

	int			numAreaportals;
	dareaportal_t *areaportals;

	int			numTexinfo;
	union {
		dBsp2Texinfo_t *texinfo2;
		dBsp1Texinfo_t *texinfo1;
	};

	dBsp1MiptexLump_t *miptex1;

	//--------- extended data -----------------------------

	int			numFlares;
	lightFlare_t *flares;

	int			numSlights;
	slight_t	*slights;

	// static effects
	int			numSplashes;			// target_splash entities
	splash_t	*splashes;

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

	float		sunLight;				// 0 if no sun
	CVec3		sunColor;
	CVec3		sunVec;
	CVec3		sunAmbient;				// ambient light from sky surfaces
	CVec3		sunSurface;
	CVec3		ambientLight;			// global ambient light
};

extern bspfile_t *map_bspfile;


//--bspfile_t *LoadBspFile (char *filename, bool clientload, unsigned *checksum);


#endif // CMODEL_H
