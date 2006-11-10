#ifndef __GL_LIGHT_INCLUDED__
#define __GL_LIGHT_INCLUDED__

#include "cmodel.h"


namespace OpenGLDrv {


#define LIGHT_DEBUG		1

#if MAX_DEBUG
#undef	LIGHT_DEBUG
#define LIGHT_DEBUG		1
#endif

#if NO_DEBUG
#undef	LIGHT_DEBUG
#define LIGHT_DEBUG		0
#endif


/*-----------------------------------------------------------------------------
	Static map lights
-----------------------------------------------------------------------------*/

class CLight
{
public:
	CLight *next;
	// color info
	CVec3	color;
	float	intens;
	// fields for culling
	int		cluster;
	float	maxDist2;
	virtual void Init() = 0;
	virtual void Add(const CVec3 &org, const CAxis &axis) const = 0;
#if LIGHT_DEBUG
	virtual void Show() const = 0;
#endif
};

// point light / spot light
class CPointLight : public CLight
{
public:
	// most fields from slight_t
	bool	spot;
	byte	style;
	CVec3	origin;
	float	focus;
	float	fade;
	CVec3	spotDir;
	float	spotDot;
	// virtual methods
	virtual void Init();
	virtual void Add(const CVec3 &org, const CAxis &axis) const;
#if LIGHT_DEBUG
	virtual void Show() const;
#endif
	virtual float GetScale(float dist) const
	{
		return 1;
	}
	virtual const char *GetName() const
	{
		return "unknown";
	}
};


// classes for different light attenuation
class CLightLinear : public CPointLight
{
	virtual void Init();
	virtual float GetScale(float dist) const;
	virtual const char *GetName() const
	{
		return "linear";
	}
};

class CLightInv : public CPointLight
{
	virtual void Init();
	virtual float GetScale(float dist) const;
	virtual const char *GetName() const
	{
		return "inv";
	}
};

class CLightInv2 : public CPointLight
{
	virtual void Init();
	virtual float GetScale(float dist) const;
	virtual const char *GetName() const
	{
		return "inv2";
	}
};

class CLightNofade : public CPointLight
{
	virtual void Init();
	virtual float GetScale(float dist) const;
	virtual const char *GetName() const
	{
		return "nofade";
	}
};


// surface light (surface is a light emitter)
class CSurfLight : public CLight
{
public:
	bool	sky;
	CVec3	center;
	surfacePlanar_t *pl;		// used for normal/axis/bounds
		//!! NOTE: if few surflights will be combined into a single one, should insert this fields here;
		//!! in this case can remove gl_model.h dependency and surfacePlanar_t decl. from OpenGLDrv.h
	virtual void Init();
	virtual void Add(const CVec3 &org, const CAxis &axis) const;
#if LIGHT_DEBUG
	virtual void Show() const;
#endif
};


// sun light
class CSunLight : public CLight
{
public:
	CVec3	origin;				// for HL sunlights
	CVec3	dir;
	virtual void Init();
	virtual void Add(const CVec3 &org, const CAxis &axis) const;
#if LIGHT_DEBUG
	virtual void Show() const
	{}
#endif
};


/*-----------------------------------------------------------------------------
	World lighting info cache (lightgrid)
-----------------------------------------------------------------------------*/

#define LIGHTGRID_STEP	32

struct lightCell_t
{
	byte	c[6][3];
};


/*-----------------------------------------------------------------------------
	Functions
-----------------------------------------------------------------------------*/

void ShowLights();

void LightForEntity(refEntity_t *ent);
void DiffuseLight(color_t *dst, float lightScale);

void InitLightGrid();
void PostLoadLights();


} // namespace

#endif
