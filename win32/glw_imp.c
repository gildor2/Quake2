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

static qboolean VerifyDriver( void )
{
	char buffer[1024];

	strcpy( buffer, qglGetString( GL_RENDERER ) );
	strlwr( buffer );
	if ( strcmp( buffer, "gdi generic" ) == 0 )
		if ( !glw_state.mcd_accelerated )
			return false;
	return true;
}

/*
 * Vid_CreateWindow
 */
#define	WINDOW_CLASS_NAME	"Quake 2"

qboolean Vid_CreateWindow( int width, int height, qboolean fullscreen )
{
	WNDCLASS	wc;
	RECT		r;
	int			stylebits;
	int			x, y, w, h;
	int			exstyle;

	/* Register the frame class */
	wc.style	= 0;
	wc.lpfnWndProc	= (WNDPROC)glw_state.wndproc;
	wc.cbClsExtra	= 0;
	wc.cbWndExtra	= 0;
	wc.hInstance	= glw_state.hInstance;
	wc.hIcon	= 0;
	wc.hCursor	= 0; //LoadCursor (NULL,IDC_ARROW);
	wc.hbrBackground	= (void *)COLOR_GRAYTEXT;
	wc.lpszMenuName 	= 0;
	wc.lpszClassName	= WINDOW_CLASS_NAME;

	if (!RegisterClass (&wc) )
		Com_Error (ERR_FATAL, "Couldn't register window class");

	if (fullscreen)
	{
		exstyle = WS_EX_TOPMOST;
		stylebits = WS_POPUP|WS_VISIBLE;
	}
	else
	{
		exstyle = 0;
		stylebits = WINDOW_STYLE;
	}

	r.left = 0;
	r.top = 0;
	r.right  = width;
	r.bottom = height;

	AdjustWindowRect (&r, stylebits, FALSE);

	w = r.right - r.left;
	h = r.bottom - r.top;

	if (fullscreen)
	{
		x = 0;
		y = 0;
	}
	else
	{
		x = Cvar_VariableInt ("vid_xpos");
		y = Cvar_VariableInt ("vid_ypos");
	}

	glw_state.hWnd = CreateWindowEx (
		 exstyle,
		 WINDOW_CLASS_NAME,
		 "Quake 2",
		 stylebits,
		 x, y, w, h,
		 NULL,
		 NULL,
		 glw_state.hInstance,
		 NULL);

	if (!glw_state.hWnd)
		Com_Error (ERR_FATAL, "Couldn't create window");

	ShowWindow( glw_state.hWnd, SW_SHOW );
	UpdateWindow( glw_state.hWnd );

	// init all the gl stuff for the window
	if (!GLimp_InitGL ())
	{
		Com_WPrintf("Vid_CreateWindow() - GLimp_InitGL failed\n");
		return false;
	}

	SetForegroundWindow( glw_state.hWnd );
	SetFocus( glw_state.hWnd );

	// let the sound and input subsystems know about the new window
	Vid_NewWindow (width, height);

	return true;
}


/*
 * GLimp_SetMode
 */
int GLimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen )
{
	int		width, height, colorBits;
	const char *win_fs[] = { "W", "FS" };

	Com_Printf ("Initializing OpenGL display\n");

	Com_Printf ("...setting mode %d:", mode);

	if ( !Vid_GetModeInfo( &width, &height, mode ) )
	{
		Com_WPrintf (" invalid mode\n");
		return rserr_invalid_mode;
	}

	Com_Printf (" %d %d %s\n", width, height, win_fs[fullscreen != false]);

	// destroy the existing window
	if (glw_state.hWnd)
	{
		gl_state.fullscreen = false; // disable ChangeDisplaySettings(0,0) in Shutdown()
		GLimp_Shutdown ();
	}

	colorBits = gl_bitdepth->integer;

	// do a CDS if needed
	if (fullscreen)
	{
		DEVMODE dm;

		Com_Printf ("...attempting fullscreen\n");

		memset (&dm, 0, sizeof(dm));

		dm.dmSize = sizeof(dm);
		dm.dmPelsWidth  = width;
		dm.dmPelsHeight = height;
		dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT;

		if (colorBits)
		{
			dm.dmBitsPerPel = colorBits;
			dm.dmFields |= DM_BITSPERPEL;
			Com_Printf ("...using gl_bitdepth of %d\n", colorBits);
		}
		else
		{
			HDC hdc = GetDC (NULL);
			int bitspixel = GetDeviceCaps (hdc, BITSPIXEL);
			ReleaseDC (0, hdc);
			Com_Printf ("...using desktop display depth of %d\n", bitspixel);
		}

		gl_state.colorBits = colorBits;
		Com_Printf ("...calling CDS: ");
		if (ChangeDisplaySettings (&dm, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL)
		{
			*pwidth = width;
			*pheight = height;
			gl_state.fullscreen = true;
			Com_Printf ("ok\n");

			if ( !Vid_CreateWindow (width, height, true) )
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
			dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;

			if (colorBits)
			{
				dm.dmBitsPerPel = colorBits;
				dm.dmFields |= DM_BITSPERPEL;
			}

			/*
			** our first CDS failed, so maybe we're running on some weird dual monitor
			** system
			*/
			if ( ChangeDisplaySettings( &dm, CDS_FULLSCREEN ) != DISP_CHANGE_SUCCESSFUL )
			{
				Com_WPrintf (" failed\n");

				Com_Printf ("...setting windowed mode\n");

				ChangeDisplaySettings( 0, 0 );

				*pwidth = width;
				*pheight = height;
				gl_state.fullscreen = false;
				gl_state.colorBits = 0;
				if ( !Vid_CreateWindow (width, height, false) )
					return rserr_invalid_mode;
				return rserr_invalid_fullscreen;
			}
			else
			{
				Com_Printf (" ok\n");
				if ( !Vid_CreateWindow (width, height, true) )
					return rserr_invalid_mode;

				gl_state.fullscreen = true;
				return rserr_ok;
			}
		}
	}
	else	// not fullscreen
	{
		Com_Printf ("...setting windowed mode\n");

		ChangeDisplaySettings( 0, 0 );

		*pwidth = width;
		*pheight = height;
		gl_state.fullscreen = false;
		if ( !Vid_CreateWindow (width, height, false) )
			return rserr_invalid_mode;
	}

	return rserr_ok;
}

/*
=============
GLimp_Gamma
=============
*/

int gamma_stored = 0;
int gamma_valid = 0;
unsigned short gamma_ramp[3*256], new_gamma[3*256];

extern cvar_t *r_ignorehwgamma;


qboolean GLimp_HasGamma (void)
{
	if (r_ignorehwgamma->integer) return false;

	return gamma_stored;
}


// Called from GLimp_Init()
void ReadGamma (void)
{
	HWND hwnd;
	HDC hdc;

	if (r_ignorehwgamma->integer)
	{
		gamma_stored = 0;
		return;
	}
	hwnd = GetDesktopWindow ();
	hdc = GetDC (hwnd);
	gamma_stored = GetDeviceGammaRamp (hdc, gamma_ramp);
	ReleaseDC (hwnd, hdc);
}


// Called from GLimp_Shutdown() and GLimp_AppActivate()
void RestoreGamma (void)
{
	HWND hwnd;
	HDC hdc;

	if (!gamma_stored) return;
	hwnd = GetDesktopWindow ();
	hdc = GetDC (hwnd);
	SetDeviceGammaRamp (hdc, gamma_ramp);
//	if (!SetDeviceGammaRamp (hdc, gamma_ramp))
//		Com_WPrintf ("Cannot restore gamma!\n");
	ReleaseDC (hwnd, hdc);
}


// Called from GLimp_SetGamma() and GLimp_AppActivate()
void UpdateGamma (void)
{
#if 0
	HWND hwnd;
	HDC hdc;

	if (!gamma_valid) return;
	hwnd = GetDesktopWindow ();
//	hwnd = glw_state.hWnd;
	hdc = GetDC (hwnd);
	if (!SetDeviceGammaRamp (hdc, new_gamma))
		Com_WPrintf ("Cannot update gamma!\n");
	ReleaseDC (hwnd, hdc);
#else
	if (!gamma_valid) return;
	SetDeviceGammaRamp (glw_state.hDC, new_gamma);
//	if (!SetDeviceGammaRamp (glw_state.hDC, new_gamma))
//		Com_WPrintf ("Cannot update gamma!\n");
#endif
}


void GLimp_SetGamma (float gamma, float intens)
{
	int		i, v;
	float	invGamma, overbright;

	if (!gamma_stored) return;

	if (gamma < 0.5f) gamma = 0.1f;
	else if (gamma > 3.0f) gamma = 3.0f;
	if (intens < 0.1f) intens = 0.1f;

	invGamma = 1.0 / gamma;
	overbright = (float) (1 << gl_config.overbrightBits);
	for (i = 0; i < 256*3; i++)
	{
		float	tmp;

#if 1	// overbright->pow
		tmp = gamma_ramp[i] * overbright + 128.0f;
		if (invGamma == 1.0)
			v = tmp;
		else
			v = 65535.0f * pow (tmp / 65535.5f, invGamma);
#else	// pow->overbright
		tmp = gamma_ramp[i] * intens + 128.0f;
		if (invGamma == 1.0)
			v = tmp * overbright;
		else
			v = 65535.0f * pow (tmp / 65535.5f, invGamma) * overbright;
#endif

		if (v > 65535) v = 65535;
		else if (v < 0) v = 0;

		new_gamma[i] = v;
	}
	gamma_valid = 1;
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
void GLimp_Shutdown( void )
{
	RestoreGamma ();

	if (qwglMakeCurrent && !qwglMakeCurrent (NULL, NULL))
		Com_WPrintf ("ref_gl::R_Shutdown() - wglMakeCurrent failed\n");
	if (glw_state.hGLRC)
	{
		if (qwglDeleteContext && !qwglDeleteContext (glw_state.hGLRC))
			Com_WPrintf ("ref_gl::R_Shutdown() - wglDeleteContext failed\n");
		glw_state.hGLRC = NULL;
	}

	if (glw_state.hDC)
	{
		if (!ReleaseDC (glw_state.hWnd, glw_state.hDC))
			Com_WPrintf ("ref_gl::R_Shutdown() - ReleaseDC failed\n");
		glw_state.hDC = NULL;
	}

	if (glw_state.hWnd)
	{
		ShowWindow (glw_state.hWnd, SW_HIDE);		// as Q3 does...
		DestroyWindow (glw_state.hWnd);
		glw_state.hWnd = NULL;
	}

	if (glw_state.log_fp)
	{
		fclose (glw_state.log_fp);
		glw_state.log_fp = 0;
	}

	UnregisterClass (WINDOW_CLASS_NAME, glw_state.hInstance);

	if (gl_state.fullscreen)
	{
		if (!strlen (vid_ref->string))
			ChangeDisplaySettings (0, 0);
		gl_state.fullscreen = false;
	}
}


/*
 * GLimp_Init
 *
 * This routine is responsible for initializing the OS specific portions
 * of OpenGL.  Under Win32 this means dealing with the pixelformats and
 * doing the wgl interface stuff.
 */
int GLimp_Init( void *hinstance, void *wndproc )
{
#define OSR2_BUILD_NUMBER 1111

	OSVERSIONINFO	vinfo;

	vinfo.dwOSVersionInfoSize = sizeof(vinfo);

	glw_state.allowdisplaydepthchange = false;

	if ( GetVersionEx( &vinfo) )
	{
		if ( vinfo.dwMajorVersion > 4 )
		{
			glw_state.allowdisplaydepthchange = true;
		}
		else if ( vinfo.dwMajorVersion == 4 )
		{
			if ( vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT )
			{
				glw_state.allowdisplaydepthchange = true;
			}
			else if ( vinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
			{
				if ( LOWORD( vinfo.dwBuildNumber ) >= OSR2_BUILD_NUMBER )
				{
					glw_state.allowdisplaydepthchange = true;
				}
			}
		}
	}
	else
	{
		Com_WPrintf ("GLimp_Init() - GetVersionEx failed\n");
		return false;
	}

	glw_state.hInstance = hinstance;
	glw_state.wndproc = wndproc;

	ReadGamma ();

	return true;
}

qboolean GLimp_InitGL (void)
{
	PIXELFORMATDESCRIPTOR pfd =
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
/*??	cvar_t *stereo;

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
	/*------- figure out if we're running on a minidriver or not ---------*/
	if (strstr (gl_driver->string, "opengl32"))
		glw_state.minidriver = false;
	else
		glw_state.minidriver = true;

	// Get a DC for the specified window
	if (glw_state.hDC != NULL)
		Com_WPrintf ("GLimp_Init() - non-NULL DC exists\n");

	if ((glw_state.hDC = GetDC (glw_state.hWnd)) == NULL)
	{
		Com_WPrintf ("GLimp_Init() - GetDC failed\n");
		return false;
	}

	if (glw_state.minidriver)
	{
		if ((pixelformat = qwglChoosePixelFormat (glw_state.hDC, &pfd)) == 0)
		{
			Com_WPrintf ("GLimp_Init() - qwglChoosePixelFormat failed\n");
			return false;
		}
		if (qwglSetPixelFormat (glw_state.hDC, pixelformat, &pfd) == FALSE)
		{
			Com_WPrintf ("GLimp_Init() - qwglSetPixelFormat failed\n");
			return false;
		}
		qwglDescribePixelFormat (glw_state.hDC, pixelformat, sizeof(pfd), &pfd);
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
		DescribePixelFormat (glw_state.hDC, pixelformat, sizeof(pfd), &pfd);

		if (!(pfd.dwFlags & PFD_GENERIC_ACCELERATED))
		{
			extern cvar_t *gl_allow_software;

			if (gl_allow_software->integer)
				glw_state.mcd_accelerated = true;
			else
				glw_state.mcd_accelerated = false;
		}
		else
		{
			glw_state.mcd_accelerated = true;
		}
	}

/*??	// report if stereo is desired but unavailable
	if (!(pfd.dwFlags & PFD_STEREO) && stereo->integer)
	{
		Com_WPrintf ("...failed to select stereo pixel format\n");
		Cvar_SetInteger ("cl_stereo", 0);
		gl_state.stereo_enabled = false;
	}
*/
	// startup the OpenGL subsystem by creating a context and making it current
	if ((glw_state.hGLRC = qwglCreateContext (glw_state.hDC)) == 0)
	{
		Com_WPrintf ("GLimp_Init() - qwglCreateContext failed\n");

		goto fail;
	}

	if (!qwglMakeCurrent (glw_state.hDC, glw_state.hGLRC))
	{
		Com_WPrintf ("GLimp_Init() - qwglMakeCurrent failed\n");

		goto fail;
	}

	if (!VerifyDriver ())
	{
		Com_WPrintf ("GLimp_Init() - no hardware acceleration detected\n");
		goto fail;
	}

	// print out PFD specifics
	Com_Printf ("GL PFD: color(%d-bits) Z(%d-bit)\n", pfd.cColorBits, pfd.cDepthBits);
	// gamma info
	Com_Printf ("GAMMA: %s\n", gamma_stored?"hardware":"software");

	return true;

fail:
	if (glw_state.hGLRC)
	{
		qwglDeleteContext (glw_state.hGLRC);
		glw_state.hGLRC = NULL;
	}

	if (glw_state.hDC)
	{
		ReleaseDC (glw_state.hWnd, glw_state.hDC);
		glw_state.hDC = NULL;
	}
	return false;
}


/*
 * GLimp_BeginFrame
 */
void GLimp_BeginFrame( float camera_separation )
{
	if (gl_bitdepth->modified)
	{
		if (gl_bitdepth->integer && !glw_state.allowdisplaydepthchange)
		{
			Cvar_SetInteger ("gl_bitdepth", 0);
			Com_WPrintf ("gl_bitdepth requires Win95 OSR2.x or WinNT 4.x\n" );
		}
		gl_bitdepth->modified = false;
	}

/*??
	if (camera_separation < 0 && gl_state.stereo_enabled)
	{
		qglDrawBuffer (GL_BACK_LEFT);
	}
	else if (camera_separation > 0 && gl_state.stereo_enabled)
	{
		qglDrawBuffer (GL_BACK_RIGHT);
	}
	else
	{
		qglDrawBuffer (GL_BACK);
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
	// swapinterval stuff
	if (gl_swapinterval->modified)
	{
		gl_swapinterval->modified = false;
		if (/*?? !gl_state.stereoEnabled &&*/ qwglSwapIntervalEXT)
			qwglSwapIntervalEXT (gl_swapinterval->integer);
	}

	if (stricmp (gl_drawbuffer->string, "GL_FRONT"))
	{	// draw buffer = GL_BACK
		if (!glw_state.minidriver)
			SwapBuffers (glw_state.hDC);
		else
		{
			// use wglSwapBuffers() for miniGL and Voodoo
			if (!qwglSwapBuffers (glw_state.hDC))
				Com_Error (ERR_FATAL, "GLimp_EndFrame() - SwapBuffers() failed!\n");
		}
	}
/*	if (gl_logFile->modified)
	{
		GLimp_EnableLogging (gl_logFile->integer);
		gl_logFile->modified = false;
	}
	else if (gl_logFile->integer == 2)
	{
		GLimp_EnableLogging (false);
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
