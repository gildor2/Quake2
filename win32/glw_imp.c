/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

/*
 * GLW_IMP.C
 *
 * This file contains ALL Win32 specific stuff having to do with the
 * OpenGL refresh.  When a port is being made the following functions
 * must be implemented by the port:
 *
 * GLimp_EndFrame
 * GLimp_Init
 * GLimp_Shutdown
 * GLimp_SwitchFullscreen
 * GLimp_HasGamma
 * GLimp_SetGamma
 *
 */
#include <assert.h>
#include <windows.h>
#include "../ref_gl/gl_local.h"
#include "glw_win.h"
#include "winquake.h"

static qboolean GLimp_SwitchFullscreen( int width, int height );
qboolean GLimp_InitGL (void);

glwstate_t glw_state;

extern cvar_t *r_fullscreen;
extern cvar_t *vid_ref;

static qboolean VerifyDriver (void)
{
	char buffer[1024];

	strcpy (buffer, glGetString (GL_RENDERER));
	strlwr (buffer);
	if (!strcmp (buffer, "gdi generic"))
		if (!glw_state.mcd_accelerated)
			return false;
	return true;
}


static qboolean GLimp_CreateWindow (int width, int height, qboolean fullscreen)
{
	glw_state.hWnd = (HWND) Vid_CreateWindow (width, height, fullscreen);
	if (!glw_state.hWnd)
		return false;

	// init all the gl stuff for the window
	if (!GLimp_InitGL ())
	{
		Com_WPrintf ("GLimp_CreateWindow: InitGL() failed\n");
		return false;
	}
	return true;
}


/*
 * GLimp_SetMode
 */
int GLimp_SetMode (int *pwidth, int *pheight, int mode, qboolean fullscreen)
{
	int		width, height, colorBits;

	Com_Printf ("Initializing OpenGL display\n");

	Com_Printf ("...setting mode %d:", mode);

	if (!Vid_GetModeInfo (&width, &height, mode))
	{
		Com_WPrintf (" invalid mode\n");
		return rserr_invalid_mode;
	}

	Com_Printf (" %d %d %s\n", width, height, fullscreen ? "FS" : "W");

	// destroy the existing window
	if (glw_state.hWnd)
	{
		gl_config.fullscreen = false; // disable ChangeDisplaySettings(0,0) in Shutdown()
		GLimp_Shutdown ();
	}

	colorBits = gl_bitdepth->integer;
	gl_bitdepth->modified = false;

	// do a CDS if needed
	if (fullscreen)
	{
		DEVMODE dm;

		Com_Printf ("...attempting fullscreen\n");

		memset (&dm, 0, sizeof(dm));

		dm.dmSize = sizeof(dm);
		dm.dmPelsWidth  = width;
		dm.dmPelsHeight = height;
		dm.dmFields     = DM_PELSWIDTH|DM_PELSHEIGHT;

		if (colorBits)
		{
			dm.dmBitsPerPel = colorBits;
			dm.dmFields |= DM_BITSPERPEL;
			Com_Printf ("...using bitdepth of %d\n", colorBits);
		}
		else
		{
			HDC hdc = GetDC (NULL);
			int bitspixel = GetDeviceCaps (hdc, BITSPIXEL);
			ReleaseDC (0, hdc);
			Com_Printf ("...using desktop display depth of %d\n", bitspixel);
		}

		gl_config.colorBits = colorBits;
		Com_Printf ("...calling CDS: ");
		if (ChangeDisplaySettings (&dm, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL)
		{
			*pwidth = width;
			*pheight = height;
			gl_config.fullscreen = true;
			Com_Printf ("ok\n");

			if (!GLimp_CreateWindow (width, height, true))
				return rserr_invalid_mode;

			return rserr_ok;
		}
		else
		{
			*pwidth = width;
			*pheight = height;

			Com_WPrintf ("failed\n");
			Com_Printf ("...calling CDS assuming dual monitors:");

			dm.dmPelsWidth = width * 2;
			dm.dmPelsHeight = height;
			dm.dmFields = DM_PELSWIDTH|DM_PELSHEIGHT;

			if (colorBits)
			{
				dm.dmBitsPerPel = colorBits;
				dm.dmFields |= DM_BITSPERPEL;
			}

			/*
			** our first CDS failed, so maybe we're running on some weird dual monitor
			** system
			*/
			if (ChangeDisplaySettings (&dm, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
			{
				Com_WPrintf (" failed\n");

				Com_Printf ("...setting windowed mode\n");

				ChangeDisplaySettings (0, 0);

				*pwidth = width;
				*pheight = height;
				gl_config.fullscreen = false;
				gl_config.colorBits = 0;
				if (!GLimp_CreateWindow (width, height, false))
					return rserr_invalid_mode;
				return rserr_invalid_fullscreen;
			}
			else
			{
				Com_Printf (" ok\n");
				if (!GLimp_CreateWindow (width, height, true))
					return rserr_invalid_mode;

				gl_config.fullscreen = true;
				return rserr_ok;
			}
		}
	}
	else	// not fullscreen
	{
		Com_Printf ("...setting windowed mode\n");

		ChangeDisplaySettings (0, 0);

		*pwidth = width;
		*pheight = height;
		gl_config.fullscreen = false;
		if (!GLimp_CreateWindow (width, height, false))
			return rserr_invalid_mode;
	}

	return rserr_ok;
}

/*
=============
GLimp_Gamma
=============
*/

static qboolean gammaStored, gammaValid;
static unsigned short gammaRamp[3*256], newGamma[3*256];

extern cvar_t *r_ignorehwgamma;


qboolean GLimp_HasGamma (void)
{
	if (r_ignorehwgamma->integer) return false;

	return gammaStored;
}


// Called from GLimp_Init()
static void ReadGamma (void)
{
	HWND	hwnd;
	HDC		hdc;

	if (r_ignorehwgamma->integer)
	{
		gammaStored = false;
		return;
	}
	hwnd = GetDesktopWindow ();
	hdc = GetDC (hwnd);
	gammaStored = GetDeviceGammaRamp (hdc, gammaRamp);
	ReleaseDC (hwnd, hdc);
}


// Called from GLimp_Shutdown() and GLimp_AppActivate()
static void RestoreGamma (void)
{
	HWND	hwnd;
	HDC		hdc;

	if (!gammaStored) return;
	hwnd = GetDesktopWindow ();
	hdc = GetDC (hwnd);
//	DebugPrintf("restore gamma\n");//!!
	SetDeviceGammaRamp (hdc, gammaRamp);
//	if (!SetDeviceGammaRamp (hdc, gammaRamp))
//		DebugPrintf ("Cannot restore gamma!\n");//!!
	ReleaseDC (hwnd, hdc);
}


// Called from GLimp_SetGamma() and GLimp_AppActivate()
static void UpdateGamma (void)
{
#if 0
	HWND	hwnd;
	HDC		hdc;

	if (!gammaValid) return;
	hwnd = GetDesktopWindow ();
//	hwnd = glw_state.hWnd;
	hdc = GetDC (hwnd);
	if (!SetDeviceGammaRamp (hdc, newGamma))
		Com_WPrintf ("Cannot update gamma!\n");
	ReleaseDC (hwnd, hdc);
#else
//	DebugPrintf("updata gamma\n");//!!
	if (!gammaValid) return;
	SetDeviceGammaRamp (glw_state.hDC, newGamma);
//	if (!SetDeviceGammaRamp (glw_state.hDC, newGamma))
//		DebugPrintf ("Cannot update gamma!\n");	//!!
#endif
}


void GLimp_SetGamma (float gamma, float intens)
{
	int		i, v;
	float	invGamma, overbright;
	float	contr, bright;

	if (!gammaStored) return;

	gamma = bound(gamma, 0.5, 3);
	if (intens < 0.1f)	//?? remove
		intens = 0.1f;
	contr = bound(r_contrast->value, 0.3, 1.8);
	bright = bound(r_brightness->value, 0.3, 1.8);

//	DebugPrintf("set gamma %g, %g\n", gamma, intens);//!!

	invGamma = 1.0f / gamma;
	overbright = (float) (1 << gl_config.overbright);
	for (i = 0; i < 256; i++)
	{
		float	tmp;

		tmp = (((i << 8) * overbright - 32768) * contr + 32768) * bright;
		if (invGamma == 1.0)
			v = Q_round (tmp);
		else
			v = Q_round (65535.0f * pow (tmp / 65535.5f, invGamma));

		if (_winmajor >= 5)						//?? may not work in DLL
		{
			int		m;

			// Win2K/XP performs checking of gamma ramp and may reject it (clamp with (0,MAX_GAMMA)-(255,FFFF) line)
#define MAX_GAMMA		0x8000					// don't works with higher values
#define GAMMA_STEP		((0x10000 - MAX_GAMMA) / 256)
			m = i * GAMMA_STEP + MAX_GAMMA;
			v = bound(v, 0, m);
		}
		else
			v = bound(v, 0, 65535);

		newGamma[i] = newGamma[i+256] = newGamma[i+512] = v;
	}

	gammaValid = true;
	UpdateGamma ();
}


/*
 * GLimp_Shutdown
 *
 * This routine does all OS specific shutdown procedures for the OpenGL
 * subsystem.  Under OpenGL this means NULLing out the current DC and
 * HGLRC, deleting the rendering context, and releasing the DC acquired
 * for the window.  The state structure is also nulled out.
 *
 */
void GLimp_Shutdown (void)
{
	RestoreGamma ();

	Com_Printf ("...performing shutdown\n");
	if (!wglMakeCurrent (NULL, NULL))
		Com_WPrintf ("...wglMakeCurrent failed\n");
	if (glw_state.hGLRC)
	{
		if (!wglDeleteContext (glw_state.hGLRC))
			Com_WPrintf ("...wglDeleteContext failed\n");
		glw_state.hGLRC = NULL;
	}

	if (glw_state.hDC)
	{
		if (!ReleaseDC (glw_state.hWnd, glw_state.hDC))
			Com_WPrintf ("...ReleaseDC failed\n");
		glw_state.hDC = NULL;
	}

	if (glw_state.log_fp)
	{
		fclose (glw_state.log_fp);
		glw_state.log_fp = NULL;
	}

	Vid_DestroyWindow (gl_bitdepth->modified);
	glw_state.hWnd = NULL;

	if (gl_config.fullscreen)
	{
		if (!strlen (vid_ref->string))
			ChangeDisplaySettings (0, 0);
		gl_config.fullscreen = false;
	}
}


/*
 * GLimp_Init
 *
 * This routine is responsible for initializing the OS specific portions
 * of OpenGL.  Under Win32 this means dealing with the pixelformats and
 * doing the wgl interface stuff.
 */
int GLimp_Init (void)
{
#define OSR2_BUILD_NUMBER 1111

	OSVERSIONINFO	vinfo;

	vinfo.dwOSVersionInfoSize = sizeof(vinfo);

	glw_state.allowdisplaydepthchange = false;

	if (GetVersionEx (&vinfo))
	{
		if (vinfo.dwMajorVersion > 4)
			glw_state.allowdisplaydepthchange = true;
		else if (vinfo.dwMajorVersion == 4)
		{
			if (vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
				glw_state.allowdisplaydepthchange = true;
			else if (vinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
			{
				if (LOWORD(vinfo.dwBuildNumber) >= OSR2_BUILD_NUMBER)
					glw_state.allowdisplaydepthchange = true;
			}
		}
	}
	else
	{
		Com_WPrintf ("GLimp_Init(): GetVersionEx failed\n");
		return false;
	}

	ReadGamma ();

	return true;
}

static qboolean GLimp_SetPixelFormat (void)
{
	PIXELFORMATDESCRIPTOR pfd =			// non-static var
	{
		sizeof(PIXELFORMATDESCRIPTOR),	// size of this pfd
		1,								// version number
		PFD_DRAW_TO_WINDOW |			// support window
		PFD_SUPPORT_OPENGL |			// support OpenGL
		PFD_DOUBLEBUFFER,				// double buffered
		PFD_TYPE_RGBA,					// RGBA type
		24,								// 24-bit color depth
		0, 0, 0, 0, 0, 0,				// color bits ignored
		0,								// no alpha buffer
		0,								// shift bit ignored
		0,								// no accumulation buffer
		0, 0, 0, 0, 					// accum bits ignored
		32,								// 32-bit z-buffer
		0,								// no stencil buffer
		0,								// no auxiliary buffer
		PFD_MAIN_PLANE,					// main layer
		0,								// reserved
		0, 0, 0							// layer masks ignored
	};
	int  pixelformat;
/* ??	cvar_t *stereo;

	stereo = Cvar_Get( "cl_stereo", "0", 0 );

	// set PFD_STEREO if necessary
	if ( stereo->integer )
	{
		Com_Printf ("...attempting to use stereo\n");
		pfd.dwFlags |= PFD_STEREO;
		gl_state.stereo_enabled = true;
	}
	else
	{
		gl_state.stereo_enabled = false;
	}
*/

	if (glw_state.minidriver)
	{
		if ((pixelformat = wglChoosePixelFormat (glw_state.hDC, &pfd)) == 0)
		{
			Com_WPrintf ("GLimp_Init() - qwglChoosePixelFormat failed\n");
			return false;
		}
		if (wglSetPixelFormat (glw_state.hDC, pixelformat, &pfd) == FALSE)
		{
			Com_WPrintf ("GLimp_Init() - qwglSetPixelFormat failed\n");
			return false;
		}
	}
	else
	{
		if ((pixelformat = ChoosePixelFormat (glw_state.hDC, &pfd)) == 0)
		{
			Com_WPrintf ("GLimp_Init() - ChoosePixelFormat failed\n");
			return false;
		}
		if (SetPixelFormat (glw_state.hDC, pixelformat, &pfd) == FALSE)
		{
			Com_WPrintf ("GLimp_Init() - SetPixelFormat failed\n");
			return false;
		}

		if (!(pfd.dwFlags & PFD_GENERIC_ACCELERATED))
		{
			extern cvar_t *gl_allow_software;

			if (gl_allow_software->integer)
				glw_state.mcd_accelerated = true;
			else
				glw_state.mcd_accelerated = false;
		}
		else
			glw_state.mcd_accelerated = true;
	}

/* ??	// report if stereo is desired but unavailable
	if (!(pfd.dwFlags & PFD_STEREO) && stereo->integer)
	{
		Com_WPrintf ("...failed to select stereo pixel format\n");
		Cvar_SetInteger ("cl_stereo", 0);
		gl_state.stereo_enabled = false;
	}
*/
	// startup the OpenGL subsystem by creating a context and making it current
	if ((glw_state.hGLRC = wglCreateContext (glw_state.hDC)) == 0)
	{
		Com_WPrintf ("...qwglCreateContext failed\n");
		return false;
	}

	if (!wglMakeCurrent (glw_state.hDC, glw_state.hGLRC))
	{
		Com_WPrintf ("...qwglMakeCurrent failed\n");
		return false;
	}

	if (!VerifyDriver ())
	{
		Com_WPrintf ("...no hardware acceleration detected\n");
		return false;
	}

	// print out PFD specifics
	DescribePixelFormat (glw_state.hDC, pixelformat, sizeof(pfd), &pfd);
	Com_Printf ("GL PFD: color(%d-bits) Z(%d-bit)\n", pfd.cColorBits, pfd.cDepthBits);
	return true;
}


static qboolean GLimp_InitGL (void)
{
	// figure out if we're running on a minidriver or not
	if (strstr (gl_driver->string, "opengl32"))
		glw_state.minidriver = false;
	else
		glw_state.minidriver = true;

	// Get a DC for the specified window
	if (glw_state.hDC != NULL)
		Com_WPrintf ("...non-NULL DC exists\n");
	if ((glw_state.hDC = GetDC (glw_state.hWnd)) == NULL)
	{
		Com_WPrintf ("...GetDC() failed\n");
		return false;
	}

	if (!GLimp_SetPixelFormat ())
	{
		if (glw_state.hGLRC)
		{
			wglDeleteContext (glw_state.hGLRC);
			glw_state.hGLRC = NULL;
		}
		if (glw_state.hDC)
		{
			ReleaseDC (glw_state.hWnd, glw_state.hDC);
			glw_state.hDC = NULL;
		}
		return false;
	}

	// gamma info
	Com_Printf ("GAMMA: %s\n", gammaStored ? "hardware" : "software");	//!! move this

	return true;
}


/*
 * GLimp_BeginFrame
 */
void GLimp_BeginFrame (float camera_separation)
{
	LOG_STRING("GLimp_BeginFrame()\n");
	if (gl_bitdepth->modified)
	{
		if (gl_bitdepth->integer && !glw_state.allowdisplaydepthchange)
		{
			Cvar_SetInteger ("gl_bitdepth", 0);
			Com_WPrintf ("gl_bitdepth requires Win95 OSR2.x or WinNT 4.x\n" );
		}
		gl_bitdepth->modified = false;
	}

/* ??
	if (camera_separation < 0 && gl_state.stereo_enabled)
	{
		glDrawBuffer (GL_BACK_LEFT);
	}
	else if (camera_separation > 0 && gl_state.stereo_enabled)
	{
		glDrawBuffer (GL_BACK_RIGHT);
	}
	else
	{
		glDrawBuffer (GL_BACK);
	}*/
}


/*
 * GLimp_EndFrame
 *
 * Responsible for doing a swapbuffers and possibly for other stuff
 * as yet to be determined.  Probably better not to make this a GLimp
 * function and instead do a call to GLimp_SwapBuffers.
 */
void GLimp_EndFrame (void)
{
	LOG_STRING("GLimp_EndFrame()\n");
	// swapinterval stuff
	if (gl_swapinterval->modified)
	{
		gl_swapinterval->modified = false;
		if (/*?? !gl_state.stereoEnabled &&*/ wglSwapIntervalEXT)
			wglSwapIntervalEXT (gl_swapinterval->integer);
	}

	if (stricmp (gl_drawbuffer->string, "GL_FRONT"))
	{	// draw buffer = GL_BACK
		if (!glw_state.minidriver)
		{
			LOG_STRING("SwapBuffers()\n");
			SwapBuffers (glw_state.hDC);
		}
		else
		{
			// use wglSwapBuffers() for miniGL and Voodoo
			if (!wglSwapBuffers (glw_state.hDC))
				Com_Error (ERR_FATAL, "GLimp_EndFrame() - SwapBuffers() failed!\n");
		}
	}
/*	if (gl_logFile->modified)
	{
		QGL_EnableLogging (gl_logFile->integer);
		gl_logFile->modified = false;
	}
	else if (gl_logFile->integer == 2)
	{
		QGL_EnableLogging (false);
		Cvar_SetInteger ("gl_logFile", 0);
	}
*/
}


/*
 * GLimp_AppActivate
 */
void GLimp_AppActivate (qboolean active)
{
	if (active)
	{
		SetForegroundWindow (glw_state.hWnd);
		ShowWindow (glw_state.hWnd, SW_RESTORE);
		if (r_fullscreen->integer) SetWindowPos (glw_state.hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
		UpdateGamma ();
	}
	else
	{
		if (r_fullscreen->integer)
			ShowWindow (glw_state.hWnd, SW_MINIMIZE);
		RestoreGamma ();
	}
}
