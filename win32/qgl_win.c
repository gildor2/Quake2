#include "../ref_gl/gl_local.h"
#include "glw_win.h"

//!! make this file multi-platform (and move from /win32 to renderer directory + rename file + header)

qgl_t			qgl;
static qgl_t	lib;

static FILE *logFile;

#include "../ref_gl/qgl_impl.h"


/*-------------- QGL_Shutdown --------------------*/

void QGL_Shutdown (void)
{
	if (logFile)
	{
		fclose (logFile);
		logFile = NULL;
	}

	if (glw_state.hinstOpenGL)
	{
		FreeLibrary (glw_state.hinstOpenGL);	//!! win32
		glw_state.hinstOpenGL = NULL;
	}

	glw_state.hinstOpenGL = NULL;

	memset (&qgl, 0, sizeof(qgl));
}


/*----------------- QGL_Init ---------------------*/


bool QGL_Init (const char *libName)
{
	int		i;
	dummyFunc_t func;

	if (!(glw_state.hinstOpenGL = LoadLibrary (libName)))	//!! win32
	{
		Com_WPrintf ("QGL_Init: LoadLibrary (%s) failed\n", libName);
		return false;
	}

	for (i = 0; i < NUM_GLFUNCS; i++)
	{
		func = qgl.funcs[i] = lib.funcs[i] = (dummyFunc_t) (GetProcAddress (glw_state.hinstOpenGL, qglNames[i]));	//!! win32
		if (!func)
		{
			// free library
			FreeLibrary (glw_state.hinstOpenGL);	//!! win32
			glw_state.hinstOpenGL = NULL;

			Com_WPrintf ("QGL_Init: inconsistent OpenGL library %s: function %s is not found\n", libName, qglNames[i]);
			return false;
		}
	}
	return true;
}


/*--------------- Extensions ---------------------*/

static bool ExtensionNameSupported (const char *name, const char *extString)
{
	int		len;
	const char *s;

	if (!extString) return false;

	len = strlen (name);
	s = extString;
	while (true)
	{
		while (s[0] == ' ') s++;				// skip spaces
		if (!s[0]) break;						// end of extension string

		if (!memcmp (s, name, len) && (s[len] == 0 || s[len] == ' ')) return true;
		while (s[0] != ' ' && s[0] != 0) s++;	// skip name
	}
	return false;
}


static bool ExtensionSupported (extInfo_t *ext, const char *extStr1, const char *extStr2)
{
	const char *s;

	s = ext->name = ext->names;		// 1st alias
	if (ExtensionNameSupported (s, extStr1) || ExtensionNameSupported (s, extStr2))
		return true;
	while (true)
	{
		s = strchr (s, '\0') + 1;
		if (!s[0]) return false;	// no another aliases
		Com_DPrintf ("%s not found - try alias %s\n", ext->names, s);
		if (ExtensionNameSupported (s, extStr1) || ExtensionNameSupported (s, extStr2))
		{
			ext->name = s;
			return true;
		}
	}
}


void QGL_InitExtensions (void)
{
	int		i;
	unsigned notFoundExt;
	extInfo_t *ext;
	dummyFunc_t func;
	const char *ext1, *ext2;

	gl_config.extensionMask = 0;
	notFoundExt = 0;
	gl_config.disabledExt = gl_config.ignoredExt = 0;
	gl_config.extensions = ext1 = glGetString (GL_EXTENSIONS);

	ext2 = NULL;
#if 1		//!! win32
	{
		const char * (APIENTRY *wglGetExtensionsStringARB) (HDC hdc);

		wglGetExtensionsStringARB = (void*)wglGetProcAddress ("wglGetExtensionsStringARB");	//?? better type conversion
		if (wglGetExtensionsStringARB)
			ext2 = wglGetExtensionsStringARB (glw_state.hDC);
	}
#endif
	gl_config.extensions2 = ext2;

	for (i = 0, ext = extInfo; i < NUM_EXTENSIONS; i++, ext++)
	{
		bool	enable;
		int		j;

		enable = false;
		if (ExtensionSupported (ext, ext1, ext2))
		{
			if (!ext->cvar || Cvar_VariableInt (ext->cvar))
				enable = true;
			else
				gl_config.disabledExt |= 1 << i;
		}
		else
			notFoundExt |= 1 << i;

		if (enable)
		{
			gl_config.extensionMask |= 1 << i;
			for (j = ext->first; j < ext->first + ext->count; j++)
			{
				func = qgl.funcs[j] = lib.funcs[j] = (dummyFunc_t) (wglGetProcAddress (qglNames[j]));	//!! win32
				if (!func)
				{
					Com_WPrintf ("Inconsistent extension %s: function %s is not found\n", ext->name, qglNames[j]);
					enable = false;
					break;
				}
			}
		}

		// (theoretically) can get "enable == false" in previous block
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
		if (!(gl_config.extensionMask & (1 << i)))
			continue;

		if ((gl_config.extensionMask & ext->require) != ext->require)
		{
			int		j;

			if (gl_config.disabledExt & ext->require)	// require disabled extension
				gl_config.disabledExt |= 1 << i;		// mark this extension as disabled too
			else
			{
				unsigned tmp = (gl_config.extensionMask ^ ext->require) & ext->require;
				// display error
				for (j = 0; j < NUM_EXTENSIONS; j++)
				{
					if ((1 << j) & tmp)
						Com_WPrintf ("%s required for %s\n", extInfo[j].names, ext->name);
				}
//				// disable extension
//				gl_config.ignoredExt |= 1 << i;
			}
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
						Com_DPrintf ("   %s ignored in favor of %s\n", ext->name, extInfo[j].name);
				}
				// disable extension
				gl_config.extensionMask &= ~(1 << i);
				gl_config.ignoredExt |= 1 << i;
			}
			else
				Com_Printf ("   %s\n", ext->name);
		}
	}
	else
		Com_WPrintf ("...no extensions was found\n");

	/*---------- notify disabled extensions ---------*/
	if (gl_config.disabledExt)
	{
		Com_Printf ("^6...disabled extensions:\n");
		for (i = 0, ext = extInfo; i < NUM_EXTENSIONS; i++, ext++)
			if (gl_config.disabledExt & (1 << i))
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


void QGL_PrintExtensionsString (const char *label, const char *str)
{
	char	name[256];
	int		i, j;
	unsigned m;
	extInfo_t *ext;
	const char *color;

	// display label
	Com_Printf ("^1%s extensions: ", label);
	// display extension names
	i = 0;
	while (true)
	{
		char c = *str++;
		if (c == ' ' || c == 0)
		{
			if (i)
			{
				name[i] = 0;
				// name[] now contains current extension name
				color = NULL;
				for (j = 0, m = 1, ext = extInfo; j < NUM_EXTENSIONS; j++, ext++, m <<= 1)
				{
					const char *s = ext->names;
					while (s[0])	// while aliases present
					{
						if (!strcmp (s, name)) break;
						s = strchr (s, '\0') + 1;
					}
					if (!s[0]) continue;
					// here: current name is one of aliases of extInfo[j]
					if (gl_config.disabledExt & m)
						color = "^6";			// disabled by cvar
					else if (gl_config.ignoredExt & m)
						color = "^4";			// ignored in a favor of different extension
					else if (gl_config.extensionMask & m)
						color = "^2";			// used
					else
						color = "^1";			// switched off by another reason
					Com_Printf ("%s%s^7 ", color, name);
					break;
				}
				if (!color) Com_Printf ("%s ", name);		// unsupported extension
				i = 0;
			}
		}
		else
		{
			if (i >= sizeof(name)-1)
			{
				Com_WPrintf ("Bad extension string\n");
				return;
			}
			name[i++] = c;
		}
		if (!c) break;
	}
	// EOLN
	Com_Printf ("\n");
}


/*---------------- Logging -----------------------*/

void QGL_EnableLogging (qboolean enable)
{
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

		qgl = logFuncs;
	}
	else
	{
		if (logFile)
		{
			fclose (logFile);
			logFile = NULL;
		}

		qgl = lib;
	}
}


void QGL_LogMessage (const char *text)
{
	if (!logFile) return;
	fprintf (logFile, text);
}
