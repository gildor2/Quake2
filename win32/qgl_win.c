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

	Z_Free (gl_config.extensionsString);
}


/*----------------- QGL_Init ---------------------*/


bool QGL_Init (const char *dllname)
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


static bool ExtensionSupported (const char *name)
{
	int		len;
	const char *s;

	len = strlen (name);
	s = gl_config.extensionsString;
	while (true)
	{
		while (s[0] == ' ') s++;				// skip spaces
		if (!s[0]) break;						// end of extension string

		if (!memcmp (s, name, len) && (s[len] == 0 || s[len] == ' ')) return true;
		while (s[0] != ' ' && s[0] != 0) s++;	// skip name
	}
	return false;
}


void QGL_InitExtensions (void)
{
	int		i, notFoundExt, disabledExt;
	extInfo_t *ext;
	dummyFunc_t func;
	const char *ext1, *ext2;

	gl_config.extensionMask = 0;
	notFoundExt = disabledExt = 0;
	ext1 = glGetString (GL_EXTENSIONS);

#if 1		//!! win32
	{
		const char * (APIENTRY *wglGetExtensionsStringARB) (HDC hdc);

		wglGetExtensionsStringARB = (void*)wglGetProcAddress ("wglGetExtensionsStringARB");	//?? better type conversion
		if (wglGetExtensionsStringARB)
			ext2 = wglGetExtensionsStringARB (glw_state.hDC);
	}
#else
	ext2 = NULL;
#endif

	i = strlen (ext1) + 1;
	if (ext2) i += strlen (ext2) + 1;
	gl_config.extensionsString = Z_Malloc (i);
	if (ext2)
		Com_sprintf (gl_config.extensionsString, i, "%s %s", ext1, ext2);
	else
		strcpy (gl_config.extensionsString, ext1);

	for (i = 0, ext = extInfo; i < NUM_EXTENSIONS; i++, ext++)
	{
		bool	enable;
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
				func = qgl.funcs[j] = lib.funcs[j] = (dummyFunc_t) (wglGetProcAddress (qglNames[j]));
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


void QGL_LogMessage (const char *text)
{
	if (!glw_state.log_fp)
		return;

	fprintf (glw_state.log_fp, text);
}
