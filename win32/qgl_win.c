#include "../ref_gl/gl_local.h"
#include "glw_win.h"

qgl_t			qgl;
static qgl_t	lib;


#include "../ref_gl/qgl_impl.h"


/*-------------- QGL_Shutdown --------------------*/

void QGL_Shutdown (void)
{
	int		i;

	if (glw_state.hinstOpenGL)
	{
		FreeLibrary (glw_state.hinstOpenGL);
		glw_state.hinstOpenGL = NULL;
	}

	glw_state.hinstOpenGL = NULL;

	for (i = 0; i < NUM_GLFUNCS; i++)
		qgl.funcs[i] = NULL;
}


/*----------------- QGL_Init ---------------------*/


qboolean QGL_Init (const char *dllname)
{
	int		i;
	dummyFunc_t func;

	if (!(glw_state.hinstOpenGL = LoadLibrary (dllname)))
	{
		Com_WPrintf ("QGL_Init: LoadLibrary (%s) failed\n", dllname);
		return false;
	}

	for (i = 0; i < NUM_GLFUNCS; i++)
	{
		func = qgl.funcs[i] = lib.funcs[i] = (dummyFunc_t) (GetProcAddress (glw_state.hinstOpenGL, qglNames[i]));
		if (!func)
		{
			// free library
			FreeLibrary (glw_state.hinstOpenGL);
			glw_state.hinstOpenGL = NULL;

			Com_WPrintf ("QGL_Init: inconsistent OpenGL library %s - function %s is not found\n", dllname, qglNames[i]);
			return false;
		}
	}
	return true;
}


void QGL_InitExtensions (void)
{
	int		i;
	extInfo_t *ext;
	dummyFunc_t func;

	gl_config.extensionMask = 0;

	Q_strncpyz (gl_config.extensions_string, qglGetString (GL_EXTENSIONS), sizeof(gl_config.extensions_string));
	for (i = 0, ext = extInfo; i < NUM_EXTENSIONS; i++, ext++)
	{
		qboolean enable;
		int		j;

		enable = false;
		if (strstr (gl_config.extensions_string, ext->name))
		{
			if (!ext->cvar || Cvar_VariableInt (ext->cvar))
				enable = true;
			else
				Com_Printf ("^6...ignoring %s\n", ext->name);
		}
		else
			Com_Printf ("...%s not found\n", ext->name);

		if (enable)
		{
			gl_config.extensionMask |= 1 << i;
			for (j = ext->first; j < ext->first + ext->count; j++)
			{
				func = qgl.funcs[j] = lib.funcs[j] = (dummyFunc_t) (qwglGetProcAddress (qglNames[j]));
				if (!func)
				{
					Com_WPrintf ("Inconsistent extension %s - function %s is not found\n", ext->name, qglNames[j]);
					enable = false;
					break;
				}
			}
		}

		// can get "enable == false" in previous block
		if (!enable)
		{
			gl_config.extensionMask &= ~(1 << i);
			for (j = ext->first; j < ext->first + ext->count; j++)
				qgl.funcs[j] = NULL;
		}
	}

	/*-------------- check requirements -------------*/
	for (i = 0, ext = extInfo; i < NUM_EXTENSIONS; i++, ext++)
	{
		int		tmp, tmp2;

		if (!(gl_config.extensionMask & (1 << i)))
			continue;

		tmp = ext->require;
		tmp2 = gl_config.extensionMask & tmp;
		if (tmp2 != tmp)
		{
			int		j;

			// display error
			for (j = 0; j < NUM_EXTENSIONS; j++)
			{
				if ((1 << j) & tmp2)
					Com_WPrintf ("%s required for %s\n", extInfo[j].name, ext->name);
			}
			// disable extension
			gl_config.extensionMask &= ~(1 << i);
		}
	}

	/*-------- choose preferred extensions ----------*/
	for (i = 0, ext = extInfo; i < NUM_EXTENSIONS; i++, ext++)
	{
		int		tmp;

		if (!(gl_config.extensionMask & (1 << i)))
			continue;

		tmp = gl_config.extensionMask & ext->deprecate;
		if (tmp)
		{
			int		j;

			// display error
			for (j = 0; j < NUM_EXTENSIONS; j++)
			{
				if ((1 << j) & tmp)
					Com_DPrintf ("...%s deprecated in favor of %s\n", ext->name, extInfo[j].name);
			}
			// disable extension
			gl_config.extensionMask &= ~(1 << i);
		}
		else
			Com_Printf ("...using %s\n", ext->name);
	}
}


void QGL_EnableLogging (qboolean enable)
{
	int		i;

	if (enable)
	{
		if (!glw_state.log_fp)
		{
			struct tm	*newtime;
			time_t		aclock;

			glw_state.log_fp = fopen (va("%s/gl.log", FS_Gamedir ()), "a+");

			time (&aclock);
			newtime = localtime (&aclock);
			fprintf (glw_state.log_fp, "%s\n", asctime (newtime));
		}

		for (i = 0; i < NUM_GLFUNCS; i++)
			qgl.funcs[i] = logFuncs.funcs[i];
		for ( ; i < NUM_GLFUNCS + NUM_EXTFUNCS; i++)
			if (lib.funcs[i])		// enable logging only when extension is active
				qgl.funcs[i] = logFuncs.funcs[i];
	}
	else
	{
		if (glw_state.log_fp)
		{
			fclose (glw_state.log_fp);
			glw_state.log_fp = NULL;
		}

		for (i = 0; i < NUM_GLFUNCS + NUM_EXTFUNCS; i++)
			qgl.funcs[i] = lib.funcs[i];
	}
}


void QGL_LogMessage (char *text)
{
	if (!glw_state.log_fp)
		return;

	fprintf (glw_state.log_fp, text);
}
