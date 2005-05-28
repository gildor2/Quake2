#ifndef __GL_LIGHT_INCLUDED__
#define __GL_LIGHT_INCLUDED__

#include "cmodel.h"


namespace OpenGLDrv {


/*-----------------------------------------------------------------------------
	Static map lights
-----------------------------------------------------------------------------*/

// point light / spot light
struct gl_slight_t
{
	slightType_t type;
	bool	spot;
	byte	style;
	CVec3	origin;
	CVec3	color;
	float	intens;
	CVec3	spotDir;
	float	spotDot;
	float	focus;
	float	fade;
	// fields for culling
	int		cluster;
	float	maxDist2;
};

// surface light (surface is a light emitter)
struct surfLight_t
{
	bool	sky;
	CVec3	color;
	float	intens;
	surfacePlanar_t *pl;		// used for normal/axis/bounds
		//!! NOTE: if few surflights will be combined into a single one, should insert this fields here;
		//!! in this case can remove gl_model.h dependency and surfacePlanar_t decl. from OpenGLDrv.h
	surfLight_t *next;
	// fields for culling
	CVec3	center;
	int		cluster;
	float	maxDist2;
};


/*-----------------------------------------------------------------------------
	World lighting info cache (lightgrid)
-----------------------------------------------------------------------------*/

#define LIGHTGRID_STEP	32

typedef struct
{
	byte	c[6][3];
} lightCell_t;


/*-----------------------------------------------------------------------------
	Functions
-----------------------------------------------------------------------------*/

void ShowLights (void);

void LightForEntity (refEntity_t *ent);
void DiffuseLight (color_t *dst, float lightScale);

void InitLightGrid (void);
void PostLoadLights (void);


} // namespace

#endif
