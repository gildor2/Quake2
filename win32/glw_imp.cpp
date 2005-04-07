/*!! TODO:
 *	- Linux: if gl_finish!=0 required (see gl_main.cpp::GL_Init() -- can force
 *		glFinish() on begin or end of frame (check this!))
 *  - UT BeginFrame / EndFrame (SwapBuffers) -- Lock / Unlock
 */

#define WIN32_LEAN_AND_MEAN			// exclude rarely-used services from windown headers
#include <windows.h>				// need this include, because have wgl and GDI functions in gl.h

#include "../ref_gl/gl_local.h"
#include "glw_win.h"

glwstate_t glw_state;

static bool minidriver;

extern bool FullscreenApp;	//!! cannot be accessed when !STATIC_BUILD


/*-----------------------------------------------------------------------------
	Gamma ramp support
-----------------------------------------------------------------------------*/

static bool gammaStored, gammaValid;
static WORD gammaRamp[3*256], newGamma[3*256];

extern cvar_t *r_ignorehwgamma;


bool GLimp_HasGamma (void)
{
	if (r_ignorehwgamma->integer) return false;
	return gammaStored;
}


// Called from GLimp_Init()
static void ReadGamma (void)
{
	if (r_ignorehwgamma->integer)
	{
		gammaStored = false;
		return;
	}
	HWND hwnd = GetDesktopWindow ();
	HDC hdc = GetDC (hwnd);
	gammaStored = GetDeviceGammaRamp (hdc, gammaRamp) != 0;
	ReleaseDC (hwnd, hdc);
}


// Called from GLimp_Shutdown() and GLimp_AppActivate()
static void RestoreGamma (void)
{
	if (!gammaStored) return;
	HWND hwnd = GetDesktopWindow ();
	HDC hdc = GetDC (hwnd);
//	DebugPrintf("restore gamma\n");//!!
	SetDeviceGammaRamp (hdc, gammaRamp);
//	if (!SetDeviceGammaRamp (hdc, gammaRamp))
//		DebugPrintf ("Cannot restore gamma!\n");//!!
	ReleaseDC (hwnd, hdc);
}


// Called from GLimp_SetGamma() and GLimp_AppActivate()
static void UpdateGamma (void)
{
//	DebugPrintf("updata gamma\n");//!!
	if (!gammaValid) return;
	SetDeviceGammaRamp (glw_state.hDC, newGamma);
//	if (!SetDeviceGammaRamp (glw_state.hDC, newGamma))
//		DebugPrintf ("Cannot update gamma!\n");	//!!
}


//#define FIND_GAMMA			// define for replace GAMMA_ANGLE and GAMMA_OFFSET with 'a' and 'b' cvars
//#define FIND_GAMMA2

void GLimp_SetGamma (float gamma)
{
#ifdef FIND_GAMMA
	float	a, b;			// y = ax + b

	a = Cvar_Get("a","1",0)->value;
#	ifndef FIND_GAMMA2
	b = Cvar_Get("b","0.5",0)->value;
#	else
	b = Cvar_Get("b","-0.5",0)->value;
#	endif
#endif

	if (!gammaStored) return;

	gamma = bound(gamma, 0.5, 3);
	float contr = bound(r_contrast->value, 0.1, 2);
	float bright = bound(r_brightness->value, 0.1, 2);
	float invGamma = 1.0f / gamma;
	float overbright = gl_config.overbright + 1;

	for (int i = 0; i < 256; i++)
	{
#if 0
		float tmp = (i / 255.0f * overbright - 0.5f) * contr + 0.5f;
		if (tmp < 0) tmp = 0;					// without this, can get semi-negative picture when r_gamma=0.5 (invGamma=2, sqr func)
		int v = appRound (65535.0f * (pow (tmp, invGamma) + bright - 1));
#else
		// taken from UT2003
		// note: ut_br = br-0.5, ut_contr = contr-0.5 (replace later: norm. bright=0.5, contr=0.5) !!
		float tmp = pow (i * overbright / 255.0f, invGamma) * contr * 65535;
		tmp = tmp + (bright - 1) * 32768 - (contr - 0.5) * 32768 + 16384;
		int v = appRound (tmp);
#endif

		if (_winmajor >= 5)						//?? may not work in DLL
		{
			// Win2K/XP performs checking of gamma ramp and may reject it
#ifndef FIND_GAMMA
			// clamp gamma curve with line 'y=x*GAMMA_ANGLE+GAMMA_OFFSET'
#define GAMMA_ANGLE		1
#define GAMMA_OFFSET	0.5
			int m = i * (GAMMA_ANGLE*256) + (int)(GAMMA_OFFSET*65536);
			if (v > m) v = m;
#define GAMMA_ANGLE2	1
#define GAMMA_OFFSET2	-0.5
			m = i * (GAMMA_ANGLE2*256) + (int)(GAMMA_OFFSET2*65536);
			if (v < m) v = m;
#else
#	ifndef FIND_GAMMA2
			m = appRound (i * a * 256 + b * 65535);
			if (v > m) v = m;
#	else
			m = appRound (i * a * 256 + b * 65535);
			if (v < m) v = m;
#	endif
#endif
		}
		v = bound(v, 0, 65535);
		newGamma[i] = newGamma[i+256] = newGamma[i+512] = v;
	}

	gammaValid = true;
	UpdateGamma ();
}


/*-----------------------------------------------------------------------------
	Win32 OpenGL context
-----------------------------------------------------------------------------*/


static HGLRC	contextHandle;
static bool		contextActive;


static bool CreateGLcontext (void)
{
	contextActive = false;
	if (!(contextHandle = wglCreateContext (glw_state.hDC)))
	{
		Com_WPrintf ("...CreateGLcontext() failed\n");
		return false;
	}
	return true;
}


static bool ActivateGLcontext (void)
{
	if (contextActive) return true;
	// 1st - activate context, 2nd - enable rendering
	if (!wglMakeCurrent (glw_state.hDC, contextHandle))
	{
		Com_WPrintf ("...ActivateGLcontext() failed\n");
		return false;
	}
	contextActive = true;
	GL_EnableRendering (true);
	return true;
}


static bool DeactivateGLcontext (void)
{
	if (!contextActive) return true;
	GL_EnableRendering (false);
	if (!wglMakeCurrent (glw_state.hDC, NULL))
	{
		Com_WPrintf ("...DeactivateGLcontext() failed\n");
		return false;
	}
	contextActive = false;
	return true;
}


static bool DestoryGLcontext (void)
{
	if (!contextHandle) return true;
	if (!DeactivateGLcontext ()) return false;
	if (!wglDeleteContext (contextHandle))
	{
		Com_WPrintf ("...DestoryGLcontext() failed...\n");
		return false;
	}
	contextHandle = NULL;
	return true;
}


/*-----------------------------------------------------------------------------
	Initialization/shutdown
-----------------------------------------------------------------------------*/

static bool GLimp_SetPixelFormat (void)
{
	static const PIXELFORMATDESCRIPTOR pfdBase =
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
	PIXELFORMATDESCRIPTOR pfd;
	int		pixelformat;
/* ??	cvar_t *stereo;

	stereo = Cvar_Get( "cl_stereo", "0", 0 );

	// set PFD_STEREO if necessary
	if ( stereo->integer )
	{
		Com_Printf ("...attempting to use stereo\n");
		pfdBase.dwFlags |= PFD_STEREO;
		gl_state.stereo_enabled = true;
	}
	else
	{
		gl_state.stereo_enabled = false;
	}
*/

	if (minidriver)
	{
		if ((pixelformat = wglChoosePixelFormat (glw_state.hDC, &pfdBase)) == 0)
		{
			Com_WPrintf ("wglChoosePixelFormat() failed\n");
			return false;
		}
		if (wglSetPixelFormat (glw_state.hDC, pixelformat, &pfdBase) == FALSE)
		{
			Com_WPrintf ("wglSetPixelFormat() failed\n");
			return false;
		}
		wglDescribePixelFormat (glw_state.hDC, pixelformat, sizeof(pfd), &pfd);
	}
	else
	{
		if ((pixelformat = ChoosePixelFormat (glw_state.hDC, &pfdBase)) == 0)
		{
			Com_WPrintf ("ChoosePixelFormat() failed\n");
			return false;
		}
		if (SetPixelFormat (glw_state.hDC, pixelformat, &pfdBase) == FALSE)
		{
			Com_WPrintf ("SetPixelFormat() failed\n");
			return false;
		}

		DescribePixelFormat (glw_state.hDC, pixelformat, sizeof(pfd), &pfd);
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
	if (!CreateGLcontext ()) return false;
	if (!ActivateGLcontext ()) return false;

	if (!(pfd.dwFlags & PFD_GENERIC_ACCELERATED) && (pfd.dwFlags & PFD_GENERIC_FORMAT))
	{
		Com_WPrintf ("...no hardware acceleration detected\n");
		return false;
	}

	// print PFD info
	Com_Printf ("Pixelformat %d: color:%d depth:%d\n", pixelformat, pfd.cColorBits, pfd.cDepthBits);
	return true;
}


static bool GLimp_InitGL (void)
{
	// figure out if we're running on a minidriver or not
	if (gl_driver->IsDefault())				// compare with opengl32.dll
		minidriver = false;
	else
	{
		Com_Printf ("...minidriver detected\n");
		minidriver = true;
	}

	// Get a DC for the specified window
	if ((glw_state.hDC = GetDC (glw_state.hWnd)) == NULL)
	{
		Com_WPrintf ("...GetDC() failed\n");
		return false;
	}

	if (!GLimp_SetPixelFormat ())
	{
		DestoryGLcontext ();
		ReleaseDC (glw_state.hWnd, glw_state.hDC);
		glw_state.hDC = NULL;
		return false;
	}

	return true;
}


bool GLimp_SetMode (unsigned *pwidth, unsigned *pheight, int mode, bool fullscreen)
{
	int		width, height, colorBits;

	if (!Vid_GetModeInfo (&width, &height, mode))
	{
		Com_WPrintf ("Invalid mode: %d\n", mode);
		return false;
	}

	Com_Printf ("Mode %d: %dx%d (%s)\n", mode, width, height, fullscreen ? "fullscreen" : "windowed");

	// destroy the existing window
	if (glw_state.hWnd)
		GLimp_Shutdown (false);

	colorBits = gl_bitdepth->integer;
	gl_bitdepth->modified = false;

	// check Win95
	if (gl_bitdepth->integer)
	{
		OSVERSIONINFO	vinfo;
		vinfo.dwOSVersionInfoSize = sizeof(vinfo);
		GetVersionEx (&vinfo);
		if (vinfo.dwMajorVersion == 4 &&
			vinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS &&		// Win9x
			LOWORD(vinfo.dwBuildNumber) < 1111)						// OSR2_BUILD_NUMBER
		{
			Com_WPrintf ("Changing color bitdepth under Win95 requires OSR2\n" );
			colorBits = 0;
		}
	}

	// do a CDS if needed
	if (fullscreen)
	{
		DEVMODE dm;
		memset (&dm, 0, sizeof(dm));
		dm.dmSize = sizeof(dm);
		dm.dmPelsWidth  = width;
		dm.dmPelsHeight = height;
		dm.dmFields     = DM_PELSWIDTH|DM_PELSHEIGHT;

		if (colorBits)
		{
			dm.dmBitsPerPel = colorBits;
			dm.dmFields |= DM_BITSPERPEL;
			Com_Printf ("...using color depth of %d\n", colorBits);
		}
		else
		{
			HDC hdc = GetDC (NULL);
			int bitspixel = GetDeviceCaps (hdc, BITSPIXEL);
			ReleaseDC (0, hdc);
			Com_Printf ("...using desktop color depth of %d\n", bitspixel);
		}

		if (ChangeDisplaySettings (&dm, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
		{
			Com_WPrintf ("...fullscreen unavailable in this mode\n");
			Com_Printf ("...setting windowed mode\n");
			fullscreen = false;

			ChangeDisplaySettings (NULL, 0);
		}
	}
	else	// not fullscreen
	{
		Com_Printf ("...setting windowed mode\n");
		ChangeDisplaySettings (NULL, 0);
	}

	*pwidth = width;
	*pheight = height;
	gl_config.fullscreen = fullscreen;

	glw_state.hWnd = (HWND) Vid_CreateWindow (width, height, fullscreen);
	if (!glw_state.hWnd) return false;
	if (!GLimp_InitGL ()) return false;	//?? may try to DestroyWindow(force) + CreateWindow() again

	// init gamma
	ReadGamma ();
	Com_Printf ("Gamma: %s\n", gammaStored ? "hardware" : "software");

	return true;
}


void GLimp_Shutdown (bool complete)
{
	RestoreGamma ();

	Com_Printf ("...performing shutdown\n");
	DestoryGLcontext ();
	if (glw_state.hDC)
	{
		if (!ReleaseDC (glw_state.hWnd, glw_state.hDC))
			Com_WPrintf ("...ReleaseDC() failed\n");
		glw_state.hDC = NULL;
	}

	Vid_DestroyWindow (true); //?? (gl_bitdepth->modified);
	glw_state.hWnd = NULL;

	if (gl_config.fullscreen)
	{
		if (complete)
		{
			Com_DPrintf ("...restore display mode\n");
			ChangeDisplaySettings (NULL, 0);
		}
		gl_config.fullscreen = false;
	}
}


/*-----------------------------------------------------------------------------
	Miscellaneous
-----------------------------------------------------------------------------*/

void GLimp_SwapBuffers (void)
{
	LOG_STRING("// GLimp_SwapBuffers()\n");
	// swapinterval stuff
	int ret;
	if (gl_swapinterval->modified)
	{
		gl_swapinterval->modified = false;
		if (/*?? !gl_state.stereoEnabled &&*/ GL_SUPPORT(QWGL_EXT_SWAP_CONTROL))
			wglSwapIntervalEXT (gl_swapinterval->integer);
	}

	if (!minidriver)
	{
		LOG_STRING("SwapBuffers()\n");
		ret = SwapBuffers (glw_state.hDC);
	}
	else
	{
		// use wglSwapBuffers() for miniGL and Voodoo
		ret = wglSwapBuffers (glw_state.hDC);
	}
	if (!ret) Com_FatalError ("GLimp_SwapBuffers(): SwapBuffers() failed!\n");
}


void GLimp_AppActivate (bool active)
{
	if (active)
	{
		SetForegroundWindow (glw_state.hWnd);
		ShowWindow (glw_state.hWnd, SW_RESTORE);
		if (FullscreenApp)
		{
			ActivateGLcontext ();
			SetWindowPos (glw_state.hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
		}
		// update gamma
#if 0
		UpdateGamma ();				// immediately -- bugs with ATI
#else
		r_gamma->modified = true;	// later
#endif
	}
	else
	{
		if (FullscreenApp)
		{
			ShowWindow (glw_state.hWnd, SW_MINIMIZE);
			DeactivateGLcontext ();
		}
		RestoreGamma ();
	}
}
