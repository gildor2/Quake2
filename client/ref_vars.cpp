#include "qcommon.h"

cvar_t	*vid_ref;
cvar_t	*r_fullscreen;

cvar_t	*r_gamma;
cvar_t	*r_brightness;
cvar_t	*r_contrast;
cvar_t	*r_saturation;

cvar_t	*r_fullbright;
cvar_t	*r_lightmap;


void InitRendererVars (void)
{
	static bool initialized = false;
CVAR_BEGIN(vars)
	CVAR_VAR(vid_ref, gl, CVAR_ARCHIVE),
	CVAR_VAR(r_fullscreen, 1, CVAR_ARCHIVE),

	CVAR_VAR(r_gamma, 1, CVAR_ARCHIVE),
	CVAR_VAR(r_brightness, 1, CVAR_ARCHIVE),
	CVAR_VAR(r_contrast, 1, CVAR_ARCHIVE),
	CVAR_VAR(r_saturation, 1, CVAR_ARCHIVE|CVAR_NOUPDATE),

	CVAR_VAR(r_fullbright, 0, CVAR_CHEAT),
	CVAR_VAR(r_lightmap, 0, CVAR_CHEAT),
CVAR_END

	if (initialized) return;
	initialized = true;
	Cvar_GetVars (ARRAY_ARG(vars));
}
