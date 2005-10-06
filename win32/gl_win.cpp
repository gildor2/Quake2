#define WIN32_LEAN_AND_MEAN			// exclude rarely-used services from windown headers
#include <windows.h>				// need this include, because have wgl and GDI functions in gl.h

#include "../ref_gl/OpenGLDrv.h"
#include "gl_win.h"


extern bool FullscreenApp;	//!! cannot be accessed when !STATIC_BUILD


namespace OpenGLDrv {


static HWND gl_hWnd;
HDC			gl_hDC;


/*-----------------------------------------------------------------------------
	Gamma ramp support
-----------------------------------------------------------------------------*/

static bool gammaStored, gammaValid;
static WORD gammaRamp[3*256], newGamma[3*256];

extern cvar_t *r_ignorehwgamma;


bool GLimp_HasGamma ()
{
	if (r_ignorehwgamma->integer) return false;
	return gammaStored;
}


// Called from GLimp_Init()
static void ReadGamma ()
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


// Called from GLimp_Shutdown() and AppActivate()
static void RestoreGamma ()
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


// Called from GLimp_SetGamma() and AppActivate()
static void UpdateGamma ()
{
//	DebugPrintf("updata gamma\n");//!!
	if (!gammaValid) return;
	SetDeviceGammaRamp (gl_hDC, newGamma);
//	if (!SetDeviceGammaRamp (gl_hDC, newGamma))
//		DebugPrintf ("Cannot update gamma!\n");	//!!
}


//#define FIND_GAMMA			// define for replace GAMMA_ANGLE and GAMMA_OFFSET with 'a' and 'b' cvars
//#define FIND_GAMMA2

void GLimp_SetGamma (float gamma)
{
#ifdef FIND_GAMMA
	float	a, b;			// y = ax + b

	a = Cvar_Get("a","1")->value;
#	ifndef FIND_GAMMA2
	b = Cvar_Get("b","0.5")->value;
#	else
	b = Cvar_Get("b","-0.5")->value;
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

		if (GIsWin2K)
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

static void ErrFail (const char *what)
{
	int err = GetLastError ();
	appWPrintf ("...%s failed\nError %d  %s\n", what, err, appGetSystemErrorMessage (err));
}

static bool CreateGLcontext ()
{
	contextActive = false;
	if (!(contextHandle = wglCreateContext (gl_hDC)))
	{
		ErrFail ("CreateGLcontext()");
		return false;
	}
	return true;
}


static bool ActivateGLcontext ()
{
	if (contextActive) return true;
	// 1st - activate context, 2nd - enable rendering
	if (!wglMakeCurrent (gl_hDC, contextHandle))
	{
		ErrFail ("ActivateGLcontext()");
		return false;
	}
	contextActive = true;
	GL_EnableRendering (true);
	return true;
}


static bool DeactivateGLcontext ()
{
	if (!contextActive) return true;
	GL_EnableRendering (false);
	if (!wglMakeCurrent (gl_hDC, NULL))
	{
		ErrFail ("DeactivateGLcontext()");
		return false;
	}
	contextActive = false;
	return true;
}


static bool DestoryGLcontext ()
{
	if (!contextHandle) return true;
	if (!DeactivateGLcontext ()) return false;
	if (!wglDeleteContext (contextHandle))
	{
		ErrFail ("DestoryGLcontext()");
		return false;
	}
	contextHandle = NULL;
	return true;
}


/*-----------------------------------------------------------------------------
	Initialization/shutdown
-----------------------------------------------------------------------------*/

static bool GLimp_SetPixelFormat ()
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

	if ((pixelformat = wglChoosePixelFormat (gl_hDC, &pfdBase)) == 0)
	{
		ErrFail ("(wgl)ChoosePixelFormat()");
		return false;
	}
	if (wglSetPixelFormat (gl_hDC, pixelformat, &pfdBase) == FALSE)
	{
		ErrFail ("(wgl)SetPixelFormat()");
		return false;
	}
	wglDescribePixelFormat (gl_hDC, pixelformat, sizeof(pfd), &pfd);

	// startup the OpenGL subsystem by creating a context and making it current
	if (!CreateGLcontext ()) return false;
	if (!ActivateGLcontext ()) return false;

	if (!(pfd.dwFlags & PFD_GENERIC_ACCELERATED) && (pfd.dwFlags & PFD_GENERIC_FORMAT))
	{
		appWPrintf ("...no hardware acceleration detected\n");
		return false;
	}

	// print PFD info
	appPrintf ("Pixelformat %d: color:%d depth:%d\n", pixelformat, pfd.cColorBits, pfd.cDepthBits);
	return true;
}


static bool GLimp_InitGL ()
{
	// get a DC for the specified window
	if ((gl_hDC = GetDC (gl_hWnd)) == NULL)
	{
		ErrFail ("GetDC()");
		return false;
	}

	// set pixel format and create GL context
	if (!GLimp_SetPixelFormat ())
	{
		DestoryGLcontext ();
		ReleaseDC (gl_hWnd, gl_hDC);
		gl_hDC = NULL;
		return false;
	}

	return true;
}


bool GLimp_SetMode (unsigned *pwidth, unsigned *pheight, int mode, bool fullscreen)
{
	int		width, height, colorBits;

	if (!Vid_GetModeInfo (&width, &height, mode))
	{
		appWPrintf ("Invalid mode: %d\n", mode);
		return false;
	}

	appPrintf ("Mode %d: %dx%d (%s)\n", mode, width, height, fullscreen ? "fullscreen" : "windowed");

	// destroy the existing window
	if (gl_hWnd)
		GLimp_Shutdown (false);

	colorBits = gl_bitdepth->integer;
	gl_bitdepth->modified = false;

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
			appPrintf ("...using color depth of %d\n", colorBits);
		}
		else
		{
			HDC hdc = GetDC (NULL);
			int bitspixel = GetDeviceCaps (hdc, BITSPIXEL);
			ReleaseDC (0, hdc);
			appPrintf ("...using desktop color depth of %d\n", bitspixel);
		}

		if (ChangeDisplaySettings (&dm, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
		{
			appWPrintf ("...fullscreen unavailable in this mode\n");
			appPrintf ("...setting windowed mode\n");
			fullscreen = false;

			ChangeDisplaySettings (NULL, 0);
		}
	}
	else	// not fullscreen
	{
		appPrintf ("...setting windowed mode\n");
		ChangeDisplaySettings (NULL, 0);
	}

	*pwidth = width;
	*pheight = height;
	gl_config.fullscreen = fullscreen;

	gl_hWnd = (HWND) Vid_CreateWindow (width, height, fullscreen);
	if (!gl_hWnd) return false;
	if (!GLimp_InitGL ()) return false;	//?? may try to DestroyWindow(force) + CreateWindow() again

	// init gamma
	ReadGamma ();
	appPrintf ("Gamma: %s\n", gammaStored ? "hardware" : "software");

	return true;
}


void GLimp_Shutdown (bool complete)
{
	RestoreGamma ();

	appPrintf ("Performing GL shutdown\n");
	DestoryGLcontext ();
	if (gl_hDC)
	{
		if (!ReleaseDC (gl_hWnd, gl_hDC))
			ErrFail ("ReleaseDC()");
		gl_hDC = NULL;
	}

	Vid_DestroyWindow (true); //?? (gl_bitdepth->modified);
	gl_hWnd = NULL;

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
	Activation/deactivation of application
-----------------------------------------------------------------------------*/

void AppActivate (bool active)
{
	if (active)
	{
		SetForegroundWindow (gl_hWnd);
		ShowWindow (gl_hWnd, SW_RESTORE);
		if (FullscreenApp)
		{
			ActivateGLcontext ();
			SetWindowPos (gl_hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
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
			ShowWindow (gl_hWnd, SW_MINIMIZE);
			DeactivateGLcontext ();
		}
		RestoreGamma ();
	}
}


} // namespace
