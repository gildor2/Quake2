#ifndef __GL_FRONTEND_INCLUDED__
#define __GL_FRONTEND_INCLUDED__


#include "gl_shader.h"


//?? move to gl_state/gl_config ??
//?? remove ?
extern GLenum	gl_fogMode;
extern float	gl_fogColor[4];
extern float	gl_fogDensity;
extern float	gl_fogStart, gl_fogEnd;


//?? rename (gl_entity_t ? entity_t) and separate beam
typedef struct refEntity_s
{
	int		flags;
	model_t	*model;
	float	dist2;						// Z-coordinate of model
	bool	visible;					// valid for current frame

	vec3_t	center, size2;				// bounding box: center and (mins-maxs)/2
	float	radius;
	union {
		struct {
		/*-------- entity with model --------*/
			// position info
			vec3_t	origin;
			vec3_t	axis[3];
			bool	worldMatrix;
			bool	mirror;
			byte	frustumMask;		//?? remove
			vec3_t	modelvieworg;		// vieworg in model coordinate system
			float	modelMatrix[4][4];	// modelview matrix
			// info for frame lerping
			int		frame, oldFrame;
			float	backLerp;
			// shading
			//?? skin_t *customSkin;	// multiple shaders (1 per surface)
			int		skinNum;			// number of default (inline) skin
		};
		/*------------- bbox ----------------*/
		struct {
			vec3_t	boxAxis[3];
			vec3_t	boxSize;
		};
	};

	shader_t	*customShader;			// one shader for all surfaces
	color_t		shaderColor;			// for "rgbGen/alphaGen entity"
	// draw sequence
	struct refEntity_s *drawNext;
} refEntity_t;

extern refEntity_t	gl_entities[];
extern int			gl_numEntities;


typedef struct
{
	vec3_t	origin;
	float	intensity;
	vec3_t	modelOrg;					// temporaty: origin in model coordinates
	color_t	c;
} refDlight_t;

extern refDlight_t	gl_dlights[];
extern int			gl_numDlights;


void	GL_DrawPortal (void);
void	GL_AddEntity (entity_t *ent);
void	GL_AddDlight (dlight_t *dl);

typedef struct surfaceInfo_s
{
	int		sort;
	surfaceCommon_t			*surf;
	struct surfaceInfo_s	*sortNext;
} surfaceInfo_t;


#define NUM_FRUSTUM_PLANES	5
#define MAX_FRUSTUM_MASK	((1<<NUM_FRUSTUM_PLANES)-1)

typedef struct viewPortal_s
{
	int		flags;
	float	time;				// time in seconds (for shader effects etc)
	lightstyle_t *lightStyles;	// light styles for Q2/HL dynamic lightmaps
	refDlight_t *dlights;
	int		numDlights;
	// view params
	vec3_t	vieworg;
	vec3_t	viewaxis[3];
	// modelview params (unused now, required for portals (??))
/* following fields may be useful when Q3 portals will be implemented (or useless ??)
	vec3_t	modelorg;			// {0 0 0} for world model (for non-portal view)
	vec3_t	modelaxis[3];		// {1 0 0}, {0 1 0}, {0 0 1} for world model
	vec3_t	modelvieworg;		// coords of vieworg in modelaxis coord system (same as vieworg for world model) */
	float	modelMatrix[4][4];
	// projection params
	int		x, y, w, h;			// viewport
	float	fov_x, fov_y, t_fov_x, t_fov_y, fov_scale;
	cplane_t frustum[NUM_FRUSTUM_PLANES];	// used for frustum culling
	float	projectionMatrix[4][4];
	vec3_t	mins, maxs;			// bounding box of all visible leafs
	float	zFar;				// maximim distance from vieworg to mins/maxs vertexes
	// surfaces
	surfaceInfo_t *surfaces;
	int		numSurfaces;
	// entities
	int		firstEntity, numEntities;
	// particles
	particle_t *particles;
	beam_t	*beams;
} viewPortal_t;


extern viewPortal_t	vp;


#endif
