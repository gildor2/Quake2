#ifndef __GL_LIGHT_INCLUDED__
#define __GL_LIGHT_INCLUDED__


// point light / spot light
typedef struct
{
	slightType_t type;
	bool	spot;
	byte	style;
	vec3_t	origin;
	vec3_t	color;
	float	intens;
	vec3_t	spotDir;
	float	spotDot;
	float	focus;
	float	fade;
	// fields for culling
	int		cluster;
	float	maxDist2;
} gl_slight_t;


typedef struct surfLight_s
{
	bool	sky;
	vec3_t	color;
	float	intens;
	surfacePlanar_t *pl;		// used for normal/axis/bounds
		//!! NOTE: if few surflights will be combined into a single one, should insert this fields here;
		//!! in this case can remove gl_model.h dependency and surfacePlanar_t decl. from gl_local.h
	struct surfLight_s *next;
	// fields for culling
	vec3_t	center;
	int		cluster;
	float	maxDist2;
} surfLight_t;


#define LIGHTGRID_STEP	32

typedef struct
{
	byte	c[6][3];
} lightCell_t;


void GL_ShowLights (void);

void GL_LightForEntity (refEntity_t *ent);
void GL_DiffuseLight (color_t *dst, float lightScale);

void GL_InitLightGrid (void);
void GL_PostLoadLights (void);


#endif
