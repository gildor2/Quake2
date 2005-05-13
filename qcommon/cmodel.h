#ifndef CMODEL_H
#define CMODEL_H

#include "qfiles.h"


struct cmodel_t
{
	vec3_t	mins, maxs;
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
	vec3_t	origin;
	float	size;
	float	radius;					// -1 for sunflare
	byte	color[4];
	byte	style;
	int		model;
	lightFlare_t *next;
};


// static map light

typedef enum {sl_linear, sl_inverse, sl_inverse2} slightType_t;

struct slight_t
{
	slightType_t type;
	byte	spot;					// bool
	vec3_t	origin;
	vec3_t	color;
	byte	style;
	float	intens;
	// arghrad fields
	float	focus;
	float	fade;					// for linear lights only: scale the distance
	// for spotlights
	float	spotDot;
	vec3_t	spotDir;
	slight_t *next;
};

// static map effects

struct splash_t
{
	vec3_t	origin;
	splash_t *next;
};


typedef enum {map_q2, map_kp, map_hl} mapType_t;
typedef enum {fog_no, fog_linear, fog_exp, fog_exp2} fogMode_t;


struct bspfile_t
{
	char		name[MAX_QPATH];	// mapname
	void		*file;				// buffer, returned by FS_LoadFile()
	mapType_t	type;
	unsigned	checksum;
	unsigned	length;
	CMemoryChain *extraChain;

	// entstring
	int			entDataSize;
	char		*entities;

	// geometry
	int			numPlanes;
	dplane_t	*planes;

	int			numVertexes;
	dvertex_t	*vertexes;

	// BSP
	int			visDataSize;
	dvis_t		*visibility;

	int			numNodes;
	dnode_t		*nodes;

	int			numLeafs;
	dleaf_t		*leafs;

	int			numLeaffaces;
	unsigned short *leaffaces;

	int			numLeafbrushes;
	unsigned short *leafbrushes;

	int			numEdges;
	dedge_t		*edges;

	int			numSurfedges;
	int			*surfedges;

	int			numBrushes;
	dbrush_t	*brushes;

	int			numBrushsides;
	dbrushside_t *brushsides;

	int			numAreas;
	darea_t		*areas;

	int			numAreaportals;
	dareaportal_t *areaportals;

	int			numModels;
	cmodel_t	*models;

	// faces
	int			numTexinfo;
	texinfo_t	*texinfo;

	int			numFaces;
	dface_t		*faces;

	// lighting
	int			lightDataSize;
	byte		*lighting;

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
	float		fogColor[3];
	union {
		struct {	// exp/exp2
			float	fogDens;
		};
		struct {	// linear
			float	fogStart;
			float	fogEnd;
		};
	};

	float	sunLight;		// 0 if no sun
	vec3_t	sunColor;
	vec3_t	sunVec;
	vec3_t	sunAmbient;		// ambient light from sky surfaces
	vec3_t	sunSurface;
	vec3_t	ambientLight;	// global ambient light
};

extern bspfile_t *map_bspfile;


//--bspfile_t *LoadBspFile (char *filename, bool clientload, unsigned *checksum);
char *ProcessEntstring (char *entString);


#endif // CMODEL_H
