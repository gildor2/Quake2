#ifndef __GL_FRONTEND_INCLUDED__
#define __GL_FRONTEND_INCLUDED__


#include "gl_shader.h"


namespace OpenGLDrv {


struct refDlight_t
{
	CVec3	origin;
	float	intensity;
	CVec3	modelOrg;					// temporaty: origin in model coordinates
	color_t	c;
};

extern refDlight_t	gl_dlights[];
extern int			gl_numDlights;


/*-----------------------------------------------------------------------------
	Portal
-----------------------------------------------------------------------------*/

struct surfaceInfo_t
{
	unsigned sort;
	surfaceBase_t *surf;
	surfaceInfo_t *sortNext;
};


#define NUM_FRUSTUM_PLANES	5
#define MAX_FRUSTUM_MASK	((1<<NUM_FRUSTUM_PLANES)-1)

struct viewPortal_t
{
	unsigned flags;
	double	time;				// time in seconds (for shader effects etc)
	lightstyle_t *lightStyles;	// light styles for Q2/HL dynamic lightmaps
	refDlight_t *dlights;
	int		numDlights;
	// view params
	CCoords	view;
	// modelview params (unused now, required for portals (??))
	float	modelMatrix[4][4];
	// projection params
	int		x, y, w, h;			// viewport
	float	fov_x, fov_y, t_fov_x, t_fov_y, fov_scale;
	cplane_t frustum[NUM_FRUSTUM_PLANES];	// used for frustum culling
	float	projectionMatrix[4][4];
	CBox	bounds;				// bounding box of all visible leafs
	float	zFar;				// maximim distance (Z-coordinate) from view.origin to mins/maxs vertexes
	// surfaces
	surfaceInfo_t *surfaces;
	int		numSurfaces;
	// entities
	int		firstEntity, numEntities;
	// particles
	particle_t *particles;
	beam_t	*beams;
};


extern viewPortal_t	vp;


/*-----------------------------------------------------------------------------
	Dynamic world data
-----------------------------------------------------------------------------*/

struct surfDlight_t
{
	float	pos[2];
	float	radius;
	refDlight_t *dlight;
	CVec3	*axis;				// CVec3[2]
};


/*-----------------------------------------------------------------------------
	Frontend interface
-----------------------------------------------------------------------------*/

//?? move to gl_state/gl_config ??
//?? remove ?
extern GLenum	gl_fogMode;
extern float	gl_fogColor[4];
extern float	gl_fogDensity;
extern float	gl_fogStart, gl_fogEnd;

// map areas
extern byte		areaMask[MAX_MAP_AREAS/8];
extern bool		forceVisMap;


//?? rename (gl_entity_t ? entity_t) -- no "ref" ("refresh")
struct refEntity_t
{
	unsigned flags;
	model_t	*model;
	float	dist2;						// Z-coordinate of model
	bool	visible;					// valid for current frame

	CVec3	center, size2;				// bounding box: center and (mins-maxs)/2
	float	radius;
	union {								//?? union is useless
		struct {
		/*-------- entity with model --------*/
			// position info
			CCoords	coord;
			bool	worldMatrix;
			bool	mirror;
			byte	frustumMask;		//?? remove
			CVec3	modelvieworg;		// vp.view.origin in model coordinate system
			float	modelMatrix[4][4];	// modelview matrix
			float	drawScale;			// 1 == original size
			// info for frame lerping
			int		frame, oldFrame;
			float	backLerp;
			// shading
			int		skinNum;			// number of default (inline) skin
			CModelSkin *skin;			// if not NULL, used for model shading
		};
	};

	shader_t	*customShader;			// one shader for all surfaces
	color_t		shaderColor;			// for "rgbGen/alphaGen entity"
	double		time;					// time for shader effects
	// draw sequence
	refEntity_t *drawNext;
};

extern refEntity_t	gl_entities[];
extern int			gl_numEntities;


void	DrawPortal ();
void	AddEntity (entity_t *ent);
void	AddDlight (dlight_t *dl);


} // namespace

#endif
