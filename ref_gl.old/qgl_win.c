#include "gl_local.h"
#include "../win32/glw_win.h"

qgl_t			qgl;
static qgl_t	lib;

static FILE *logFile;

#include "qgl_impl.h"


/*-------------- QGL_Shutdown --------------------*/

void QGL_Shutdown (void)
{
	int		i;

	Com_DPrintf ("QGL_Shutdown()\n");
	if (glw_state.hinstOpenGL)
	{
		FreeLibrary (glw_state.hinstOpenGL);
		glw_state.hinstOpenGL = NULL;
	}

	glw_state.hinstOpenGL = NULL;

	for (i = 0; i < NUM_GLFUNCS; i++)
		qgl.funcs[i] = NULL;

//	if (gl_config.extensionsString)
//	{
		Z_Free (gl_config.extensionsString);
//		gl_config.extensionsString = NULL;
//	}
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


static qboolean ExtensionSupported (const char *name)
{
	char	*match, *next, *origin;
	int		len;

	len = strlen (name);
	for (origin = gl_config.extensionsString; *origin; origin = next)
	{
		match = strstr (origin, name);
		if (!match) return false;

		next = match + len;
		if ((match == origin || *(match - 1) == ' ') && (*next == ' ' || *next == 0))
			return true;
	}
	return false;
}


void QGL_InitExtensions (void)
{
	int		i, notFoundExt, disabledExt;
	extInfo_t *ext;
	dummyFunc_t func;

	gl_config.extensionMask = 0;
	notFoundExt = disabledExt = 0;
	gl_config.extensionsString = CopyString (qglGetString (GL_EXTENSIONS));

	for (i = 0, ext = extInfo; i < NUM_EXTENSIONS; i++, ext++)
	{
		qboolean enable;
		int		j;

		enable = false;
		if (ExtensionSupported (ext->name))
		{
			if (!ext->cvar || Cvar_VariableInt (ext->cvar))
				enable = true;
			else
				disabledExt |= 1 << i;
		}
		else
			notFoundExt |= 1 << i;

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
	if (gl_config.extensionMask)
	{
		Com_Printf ("...used extensions:\n");
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
						Com_DPrintf ("   %s deprecated in favor of %s\n", ext->name, extInfo[j].name);
				}
				// disable extension
				gl_config.extensionMask &= ~(1 << i);
			}
			else
				Com_Printf ("   %s\n", ext->name);
		}
	}
	else
		Com_WPrintf ("...no extensions was found\n");

	/*---------- notify disabled extensions ---------*/
	if (disabledExt)
	{
		Com_Printf ("^6...disabled extensions:\n");
		for (i = 0, ext = extInfo; i < NUM_EXTENSIONS; i++, ext++)
			if (disabledExt & (1 << i))
				Com_Printf ("^6   %s\n", ext->name);
	}

	/*----------- notify extension absence ----------*/
	if (notFoundExt)
	{
		Com_Printf ("...undetected extensions:\n");
		for (i = 0, ext = extInfo; i < NUM_EXTENSIONS; i++, ext++)
			if (notFoundExt & (1 << i))
			{
				int		tmp;

				tmp = gl_config.extensionMask & ext->deprecate;
				if (tmp)
				{
					int		j;

					// display error
					for (j = 0; j < NUM_EXTENSIONS; j++)
					{
						if ((1 << j) & tmp)
						{
							Com_DPrintf ("   %s is covered by %s\n", ext->name, extInfo[j].name);
							break;		// ignore other exteensions
						}
					}
				}
				else
					Com_Printf ("   %s\n", ext->name);
			}
		Com_Printf ("\n");
	}
}


void QGL_EnableLogging (qboolean enable)
{
	int		i;

	if (enable)
	{
		if (!logFile)
		{
			struct tm	*newtime;
			time_t		aclock;

			logFile = fopen (va("%s/gl.log", FS_Gamedir ()), "a+");

			time (&aclock);
			newtime = localtime (&aclock);
			fprintf (logFile, "\n------------------------\n%s------------------------\n", asctime (newtime));
		}

		for (i = 0; i < NUM_GLFUNCS; i++)
			qgl.funcs[i] = logFuncs.funcs[i];
		for ( ; i < NUM_GLFUNCS + NUM_EXTFUNCS; i++)
			if (lib.funcs[i])		// enable logging only when extension is active
				qgl.funcs[i] = logFuncs.funcs[i];
	}
	else
	{
		if (logFile)
		{
			fclose (logFile);
			logFile = NULL;
		}

		for (i = 0; i < NUM_GLFUNCS + NUM_EXTFUNCS; i++)
			qgl.funcs[i] = lib.funcs[i];
	}
}


void QGL_LogMessage (const char *text)
{
	if (!logFile) return;
	fprintf (logFile, text);
}
