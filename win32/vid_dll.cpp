#include "winquake.h"
#include "resource.h"

#include "../client/client.h"


#define SINGLE_RENDERER
#define DEFAULT_RENDERER	"gl"

cvar_t *win_noalttab;
cvar_t *win_priorityBoost;


static UINT MSH_MOUSEWHEEL = 0xDEAD0000; // initialized to something unused

// Console variables that we need to access from this module
static cvar_t	*vid_xpos;				// X coordinate of window position
static cvar_t	*vid_ypos;				// Y coordinate of window position

#ifndef SINGLE_RENDERER
static cvar_t	*vid_ref;
static HINSTANCE refLibrary;			// handle to renderer DLL
// Imports from main engine for renderer DLL
#include "../client/ref_impl.h"
#endif

// Structure containing functions exported from renderer DLL
refExport_t	re;

// Global variables used internally by this module
viddef_t viddef;						// global video state; used by other modules
static bool refActive = false;

static bool needRestart;

HWND	cl_hwnd;						// main window handle

static bool alttab_disabled;

extern unsigned sys_msg_time;

/*
** WIN32 helper functions
*/
static bool s_win95;

static void DisableAltTab (bool enable)
{
	if (alttab_disabled == enable) return;
	alttab_disabled = enable;

	if (s_win95)
	{
		BOOL old;
		SystemParametersInfo (SPI_SCREENSAVERRUNNING, enable, &old, 0);
	}
	else
	{
		if (enable)
		{
			RegisterHotKey (0, 0, 1/*MOD_ALT*/, VK_TAB);	// MOD_ALT is redefined, so use winuser.h const directly
//			RegisterHotKey (0, 1, 1/*MOD_ALT*/, VK_RETURN);	// disable Alt+Enter too (do not allow fullscreen noggle)
			// really, should disable <Win>, <Ctrl+Esc>, <Alt+Esc> key combinations too; see Q226359 in MSDN
		}
		else
		{
			UnregisterHotKey (0, 0);
//			UnregisterHotKey (0, 1);
		}
	}
}

static void SetHighPriority (bool enable)
{
	HANDLE hProcess = GetCurrentProcess ();
	SetPriorityClass (hProcess, enable ? HIGH_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS);
}


//==========================================================================


#define UNK		255

static const byte vkToKey[256] = {
//			08			19			2A			3B			4C			5D			6E			7F
/*00*/		0,			0,			0,			0,			0,	 		UNK,		UNK,		UNK,
/*08*/		K_BACKSPACE,K_TAB,		UNK,		UNK,		UNK,		K_ENTER,	UNK,		UNK,
/*10*/		K_SHIFT,	K_CTRL,		K_ALT,		K_PAUSE,	K_CAPSLOCK,	UNK,		UNK,		UNK,
/*18*/		UNK,		UNK,		UNK,		K_ESCAPE,	UNK,		UNK,		UNK,		UNK,
/*20*/		K_SPACE,	K_PGUP,		K_PGDN,		K_END,		K_HOME,		K_LEFTARROW,K_UPARROW,	K_RIGHTARROW,
/*28*/		K_DOWNARROW,0,			0,			0,			K_PRINTSCRN,K_INS,		K_DEL,		0,
/*30*/		'0',		'1',		'2',		'3',		'4',		'5',		'6',		'7',
/*38*/		'8',		'9',		UNK,		UNK,		UNK,		UNK,		UNK,		UNK,
/*40*/		UNK,		'a',		'b',		'c',		'd',		'e',		'f',		'g',
/*48*/		'h',		'i',		'j',		'k',		'l',		'm',		'n',		'o',
/*50*/		'p',		'q',		'r',		's',		't',		'u',		'v',		'w',
/*58*/		'x',		'y',		'z',		UNK,		UNK,		UNK,		UNK,		UNK,
/*60*/		K_KP_INS,	K_KP_END,	K_KP_DOWNARROW,K_KP_PGDN,K_KP_LEFTARROW,K_KP_5,	K_KP_RIGHTARROW,K_KP_HOME,
/*68*/		K_KP_UPARROW,K_KP_PGUP,	K_KP_STAR,	K_KP_PLUS,	0,			K_KP_MINUS,	K_KP_DEL,	K_KP_SLASH,
/*70*/		K_F1,		K_F2,		K_F3,		K_F4,		K_F5,		K_F6,		K_F7,		K_F8,
/*78*/		K_F9,		K_F10,		K_F11,		K_F12,		K_F13,		K_F14,		K_F15,		K_F16,
/*80*/		K_F17,		K_F18,		K_F19,		K_F20,		K_F21,		K_F22,		K_F23,		K_F24,
/*88*/		UNK,		UNK,		UNK,		UNK,		UNK,		UNK,		UNK,		UNK,
/*90*/		K_NUMLOCK,	K_SCRLOCK,	UNK,		UNK,		UNK,		UNK,		UNK,		UNK,
/*98*/		UNK,		UNK,		UNK,		UNK,		UNK,		UNK,		UNK,		UNK,
/*A0*/		K_SHIFT,	K_SHIFT,	K_CTRL,		K_CTRL,		K_ALT,		K_ALT,		UNK,		UNK,
/*A8*/		UNK,		UNK,		UNK,		UNK,		UNK,		UNK,		UNK,		UNK,
/*B0*/		UNK,		UNK,		UNK,		UNK,		UNK,		UNK,		UNK,		UNK,
/*B8*/		UNK,		UNK,		';',		'=',		',',		'-',		'.',		'/',
/*C0*/		'`',		0,			0,			0,			0,			0,			0,			0,
/*C8*/		0,			0,			0,			0,			0,			0,			0,			0,
/*D0*/		0,			0,			0,			0,			0,			0,			0,			0,
/*D8*/		UNK,		UNK,		UNK,		'[',		'\\',		']',		'\'',		UNK,
/*E0*/		UNK,		UNK,		UNK,		UNK,		UNK,		UNK,		UNK,		UNK,
/*E8*/		UNK,		UNK,		UNK,		UNK,		UNK,		UNK,		UNK,		UNK,
/*F0*/		UNK,		UNK,		UNK,		UNK,		UNK,		UNK,		UNK,		UNK,
/*F8*/		UNK,		UNK,		UNK,		UNK,		UNK,		UNK,		UNK,		0
};


static int MapKey (int vkCode, bool extended)
{
	int		key;

	if (!extended)
	{

		switch (vkCode)
		{
		case VK_PRIOR:	return K_KP_PGUP;
		case VK_NEXT:	return K_KP_PGDN;
		case VK_END:	return K_KP_END;
		case VK_HOME:	return K_KP_HOME;
		case VK_LEFT:	return K_KP_LEFTARROW;
		case VK_UP:		return K_KP_UPARROW;
		case VK_RIGHT:	return K_KP_RIGHTARROW;
		case VK_DOWN:	return K_KP_DOWNARROW;
		case VK_CLEAR:	return K_KP_5;
		case VK_DELETE:	return K_KP_DEL;
		case VK_INSERT: return K_KP_INS;
		}
	}
	else
	{
		switch (vkCode)
		{
		case VK_RETURN:	return K_KP_ENTER;
		case VK_CONTROL:return K_RCTRL;
		case VK_MENU:	return K_RALT;
		}
	}
	key = vkToKey[vkCode];
	if (key == UNK) key = vkCode + 256;
	return key;
}


static void AppActivate (bool active, bool minimized)
{
//	Com_Printf("act:%d min:%d\n", active, minimized);//!!
	MinimizedApp = minimized;
	if (ActiveApp != active)
	{
		ActiveApp = active;

		Key_ClearStates ();
		IN_Activate (active);
		CDAudio_Activate (active);
		S_Activate (active);
		if (refActive) RE_AppActivate (active);

		if (win_noalttab->integer)		DisableAltTab (active);
		if (win_priorityBoost->integer)	SetHighPriority (active);
	}
}

/*
====================
MainWndProc

main window procedure
====================
*/
static LONG WINAPI MainWndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	guard(MainWndProc);

//	DebugPrintf("msg=%X w=%X l=%X\n",uMsg,wParam,lParam);//!!
	if (uMsg == MSH_MOUSEWHEEL) uMsg = WM_MOUSEWHEEL;

	switch (uMsg)
	{
	case WM_MOUSEWHEEL:
		// this chunk of code theoretically only works under NT4+ and Win98+
		// since this message doesn't exist under Win95
		if (wParam > 0)	// really, HIWORD(wParam) should be checked, but LOWORD() does not affects the sign of result
		{
			Key_Event (K_MWHEELUP, true, sys_msg_time);
			Key_Event (K_MWHEELUP, false, sys_msg_time);
		}
		else
		{
			Key_Event (K_MWHEELDOWN, true, sys_msg_time);
			Key_Event (K_MWHEELDOWN, false, sys_msg_time);
		}
		break;

	case WM_HOTKEY:
		return 0;

	case WM_SIZE:
		if (FullscreenApp)
		{
			static DEVMODE dm;
			static bool desktopMode = false;

//			Com_Printf("WM_SIZE: %d (act=%d min=%d)\n",wParam, ActiveApp, MinimizedApp);//!!
			if (wParam == SIZE_MINIMIZED && !desktopMode)
			{
				Com_DPrintf ("Setting desktop resolution\n");
				// save screen parameters
				HDC dc = GetDC (NULL);
				dm.dmSize = sizeof(dm);
				dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
				dm.dmPelsWidth = GetDeviceCaps (dc, HORZRES);
				dm.dmPelsHeight = GetDeviceCaps (dc, VERTRES);
				dm.dmBitsPerPel = GetDeviceCaps (dc, BITSPIXEL);
				// restore mode
				ChangeDisplaySettings (NULL, 0);
				desktopMode = true;
			}
			else if (wParam == SIZE_RESTORED && MinimizedApp && desktopMode)
			{
				Com_DPrintf ("Setting game resolution\n");
				ChangeDisplaySettings (&dm, CDS_FULLSCREEN);
				SetWindowPos (hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE);
				SetForegroundWindow (hWnd);
				desktopMode = false;
			}
		}
		break;

	case WM_ACTIVATE:
		{
			bool active = LOWORD(wParam) != WA_INACTIVE;
			bool minimized = HIWORD(wParam) != 0;
//			Com_Printf("WM_ACTIVATE: a=%d m=%d (act=%d min=%d)\n",active, minimized, ActiveApp, MinimizedApp);//!!
			AppActivate (active, minimized);
		}
		break;

	case WM_MOVE:
		if (!FullscreenApp)
		{
			int xPos = (short) LOWORD(lParam);    // horizontal position
			int yPos = (short) HIWORD(lParam);    // vertical position

			RECT r;
			r.left   = 0;
			r.top    = 0;
			r.right  = 1;
			r.bottom = 1;

			int style = GetWindowLong (hWnd, GWL_STYLE);
			AdjustWindowRect (&r, style, FALSE);

			Cvar_SetInteger ("vid_xpos", xPos + r.left);
			Cvar_SetInteger ("vid_ypos", yPos + r.top);
			vid_xpos->modified = false;
			vid_ypos->modified = false;
			if (ActiveApp) IN_Activate (true);
		}
		break;

	// this is complicated because Win32 seems to pack multiple mouse events into
	// one update sometimes, so we always check all states and look for events
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEMOVE:
		{
			int temp = 0;
			if (wParam & MK_LBUTTON) temp |= 1;
			if (wParam & MK_RBUTTON) temp |= 2;
			if (wParam & MK_MBUTTON) temp |= 4;
			IN_MouseEvent (temp);
		}
		break;

	case WM_SYSCOMMAND:
		switch (wParam)
		{
			case SC_SCREENSAVE:		// disable screensaver
			case SC_MONITORPOWER:	// disable monitor power managemment
			case SC_KEYMENU:		// disable activating window menu with keyboard
				return 0;
		}
		break;

	case WM_SYSKEYDOWN:
		if (wParam == VK_RETURN)	// Alt+Enter
		{
			Cvar_SetInteger ("r_fullscreen", !FullscreenApp);
			return 0;
		}
		// fall through
	case WM_KEYDOWN:
		{
			int k = MapKey (wParam, (lParam >> 24) & 1);
			if (k) Key_Event (k, true, sys_msg_time);
		}
		return 0;

	case WM_SYSKEYUP:
	case WM_KEYUP:
		{
			int k = MapKey (wParam, (lParam >> 24) & 1);
			if (k) Key_Event (k, false, sys_msg_time);
		}
		return 0;

	case WM_POWERBROADCAST:
		if (wParam == PBT_APMSUSPEND)	// will minimize window when in fullscreen mode
			RE_AppActivate (false);
		break;

	case WM_CLOSE:
		Com_Quit ();
		break;							// should not return here

	case MM_MCINOTIFY:
		{
			LONG CDAudio_MessageHandler (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);	//?? extern
			CDAudio_MessageHandler (hWnd, uMsg, wParam, lParam);
		}
		break;
	}

	return DefWindowProc (hWnd, uMsg, wParam, lParam);

	unguardf(("msg=%X", uMsg));
}


/*
 * Vid_CreateWindow
 */

void *Vid_CreateWindow (int width, int height, bool fullscreen)
{
	WNDCLASS wc;
	RECT	r;
	int		stylebits;
	int		x, y, w, h;
	int		exstyle;

	guard(Vid_CreateWindow);

	if (fullscreen)
	{
		exstyle = WS_EX_TOPMOST;
		stylebits = WS_POPUP|WS_SYSMENU;
	}
	else
	{
		exstyle = 0;
		stylebits = WS_SYSMENU|WS_CAPTION;
	}

	// if size=0 -- invisible (fake) window
//	if (width || height)		// if enable this, window will be created without taskbar button!
//		stylebits |= WS_VISIBLE;

	r.left = 0;
	r.top = 0;
	r.right  = width;
	r.bottom = height;

	AdjustWindowRect (&r, stylebits, FALSE);

	w = r.right - r.left;
	h = r.bottom - r.top;

	FullscreenApp = fullscreen;
	if (fullscreen)
	{
		x = 0;
		y = 0;
	}
	else
	{
		x = vid_xpos->integer;
		y = vid_ypos->integer;
	}

	if (cl_hwnd)
	{
		SetWindowLong (cl_hwnd, GWL_STYLE, stylebits);
//		SetWindowLong (cl_hwnd, GWL_EXSTYLE, exstyle); -- require to change WS_EX_TOPMOST only, do it with SetWindowPos() (better effect)
		ShowWindow (cl_hwnd, SW_SHOW);
		SetWindowPos (cl_hwnd, fullscreen ? HWND_TOPMOST : HWND_NOTOPMOST, x, y, w, h, 0);
	}
	else
	{
		// Register the frame class
		wc.style			= 0;
		wc.lpfnWndProc		= MainWndProc;
		wc.cbClsExtra		= 0;
		wc.cbWndExtra		= 0;
		wc.hInstance		= global_hInstance;
		wc.hIcon			= LoadIcon (global_hInstance, MAKEINTRESOURCE(IDI_ICON1));
		wc.hCursor			= LoadCursor (NULL,IDC_ARROW);
		wc.hbrBackground	= (HBRUSH) COLOR_GRAYTEXT;
		wc.lpszMenuName 	= NULL;
		wc.lpszClassName	= APPNAME;
		if (!RegisterClass (&wc)) Com_FatalError ("Couldn't register window class");

		cl_hwnd = CreateWindowEx (exstyle, APPNAME, APPNAME, stylebits, x, y, w, h, NULL, NULL, global_hInstance, NULL);
		if (!cl_hwnd) Com_FatalError ("Couldn't create window");

		if (width || height) ShowWindow (cl_hwnd, SW_SHOW);
		// let the sound and input subsystems know about the new window
		cl.force_refdef = true;		// can't use a paused refdef
		in_needRestart = true;
	}

	viddef.width  = width;
	viddef.height = height;

	UpdateWindow (cl_hwnd);
	SetForegroundWindow (cl_hwnd);
	SetFocus (cl_hwnd);

	return cl_hwnd;

	unguard;
}

/*
 * Vid_DestroyWindow
 */

void Vid_DestroyWindow (bool force)
{
//force = true; //???
	if (!force)	//?? add cvar "win_singleWindow"
	{
//		ShowWindow (cl_hwnd, SW_HIDE);
		return;
	}

	Com_DPrintf ("...destroying window\n");
	if (cl_hwnd)
	{
//		ShowWindow (cl_hwnd, SW_HIDE);	-- this will force to CDS(0,0) when vid_restart, because Activate(0)->Minimize()->WM_SIZE
		DestroyWindow (cl_hwnd);
		cl_hwnd = 0;
	}

	UnregisterClass (APPNAME, global_hInstance);
}


void Vid_Restart (void)
{
	needRestart = true;
}


/*------------- Vid_GetModeInfo -------------*/

typedef struct
{
	short	width, height;
} vidMode_t;

static const vidMode_t vid_modes[] =
{
	{320,	240},
	{400,	300},
	{512,	384},
	{640,	480},
	{800,	600},
	{960,	720},
	{1024,	768},
	{1152,	864},
	{1280,	960},
	{1600,	1200},
	{2048,	1536}
};


bool Vid_GetModeInfo (int *width, int *height, int mode)
{
	if (mode < 0 || mode >= ARRAY_COUNT(vid_modes))
		return false;

	*width  = vid_modes[mode].width;
	*height = vid_modes[mode].height;

	return true;
}


static void Vid_UpdateWindowPosAndSize (int x, int y)
{
	RECT	r;
	r.left   = 0;
	r.top    = 0;
	r.right  = viddef.width;
	r.bottom = viddef.height;

	int style = GetWindowLong (cl_hwnd, GWL_STYLE);
	AdjustWindowRect (&r, style, FALSE);

	int w = r.right - r.left;
	int h = r.bottom - r.top;

	MoveWindow (cl_hwnd, vid_xpos->integer, vid_ypos->integer, w, h, TRUE);
}



/*-------- Dummy functions for console-only mode ---------*/

static void	D_RenderFrame (refdef_t *fd) {}
static void	D_BeginRegistration (const char *map) {}
static model_t *D_RegisterModel (const char *name) { return NULL; }
static image_t *D_RegisterSkin (const char *name) { return NULL; }
static void D_ReloadImage (const char *name) {}
static image_t *D_FindPic (const char *name) { return NULL; }
static void D_SetSky (const char *name, float rotate, vec3_t axis) {}
static void	D_EndRegistration (void) {}

static void	D_Draw_GetPicSize (int *w, int *h, const char *pic)
{
	if (w) *w = 0;
	if (h) *h = 0;
}

static void D_Screenshot (int flags, const char *name)
{
	Com_WPrintf ("Screenshots are unsupported by this renderer\n");
}

static void	D_Draw_Pic (int x, int y, const char *name, int color) {}
static void	D_Draw_StretchPic (int x, int y, int w, int h, const char *pic) {}
static void	D_Draw_Char (int x, int y, int c, int color) {}
static void	D_Draw_TileClear (int x, int y, int w, int h, const char *name) {}
static void	D_Draw_Fill (int x, int y, int w, int h, int c) {}
static void D_Draw_Fill2 (int x, int y, int w, int h, unsigned rgba) {}
static void	D_DrawTextPos (int x, int y, const char *text, unsigned rgba) {}
static void	D_DrawTextSide (const char *text, unsigned rgba) {}
static void	D_Draw_StretchRaw8 (int x, int y, int w, int h, int cols, int rows, byte *data, unsigned *palette) {}
static float D_GetClientLight (void) { return 0; }		// normal value is 150


static void FreeRenderer (void)
{
	refActive = false;

#ifndef SINGLE_RENDERER
	if (refLibrary)		// if false - sattically linked
	{
		if (!FreeLibrary (refLibrary))
			Com_FatalError ("Reflib FreeLibrary() failed");
		refLibrary = NULL;
	}
#endif
	memset (&re, 0, sizeof(re));
}


#ifdef STATIC_BUILD
// externs
bool STATIC_RENDERER(GL) (refExport_t *);
#endif

#ifndef SINGLE_RENDERER
static bool LoadRenderer (const char *name)
#else
static bool LoadRenderer (void)
#endif
{
	guard(LoadRenderer);

	if (refActive)
	{
		RE_Shutdown ();
		FreeRenderer ();
	}

#ifndef SINGLE_RENDERER

	Com_Printf ("Loading %s\n", name);

#ifdef STATIC_BUILD
	refLibrary = NULL;

	if (!strcmp (name, "gl"))
		STATIC_RENDERER(GL) (&re);
	else
#endif
	{
		char	dllName[MAX_OSPATH];
		appSprintf (ARRAY_ARG(dllName), "ref_%s.dll", name);
		if (!(refLibrary = LoadLibrary (dllName)))
		{
			Com_WPrintf ("LoadLibrary(\"%s\") failed\n", dllName);
			return false;
		}
		CreateDynRenderer_t pCreateRenderer;
		if (!(pCreateRenderer = (CreateDynRenderer_t) GetProcAddress (refLibrary, "CreateRenderer")))
		{
			Com_WPrintf ("GetProcAddress() failed on %s\n", dllName);
			FreeRenderer ();
			return false;
		}

		re.struc_size = sizeof(refExport_t);
		if (!pCreateRenderer (&ri, &re))
		{
			Com_WPrintf ("%s has incompatible renderer\n", dllName);
			FreeRenderer ();
			return false;
		}
	}

#else // SINGLE_RENDERER
	STATIC_RENDERER(GL) (&re);
#endif

	if (*re.flags & REF_CONSOLE_ONLY)
	{
		re.RenderFrame =	D_RenderFrame;
		re.Screenshot =		D_Screenshot;
		re.BeginRegistration = D_BeginRegistration;
		re.RegisterModel =	D_RegisterModel;
		re.RegisterSkin =	D_RegisterSkin;
		re.RegisterPic =	D_FindPic;
		re.SetSky =			D_SetSky;
		re.EndRegistration = D_EndRegistration;

		re.ReloadImage =    D_ReloadImage;
		re.DrawGetPicSize =	D_Draw_GetPicSize;
		re.DrawPic =		D_Draw_Pic;
		re.DrawStretchPic =	D_Draw_StretchPic;
		re.DrawDetailedPic = D_Draw_StretchPic;
		re.DrawChar =		D_Draw_Char;
		re.DrawTileClear =	D_Draw_TileClear;
		re.DrawFill =		D_Draw_Fill;
		re.DrawFill2 =		D_Draw_Fill2;
		re.DrawStretchRaw8 = D_Draw_StretchRaw8;

		re.DrawTextPos =	D_DrawTextPos;
		re.DrawTextLeft =	D_DrawTextSide;
		re.DrawTextRight =	D_DrawTextSide;

		re.GetClientLight = D_GetClientLight;
	}

	if (!RE_Init ())
	{
		RE_Shutdown ();
		FreeRenderer ();
		return false;
	}

	Com_Printf ("------------------------------------\n");
	refActive = true;

	return true;

	unguard;
}

/*
============
Vid_CheckChanges

This function gets called once just before drawing each frame, and it's sole purpose in life
is to check to see if any of the video mode parameters have changed, and if they have to
update the rendering DLL and/or video mode to match.
============
*/

void Vid_CheckChanges (void)
{
	if (win_noalttab->modified)
	{
		DisableAltTab (win_noalttab->integer != 0);
		win_noalttab->modified = false;
	}

	if (win_priorityBoost->modified)
	{
		SetHighPriority (win_priorityBoost->integer != 0);
		win_priorityBoost->modified = false;
	}

#ifndef SINGLE_RENDERER
	if (vid_ref->modified)
	{
		needRestart = true;
		vid_ref->modified = false;
	}
#endif

	if (needRestart)
	{
		// refresh has changed
		cl.force_refdef = true;		// can't use a paused refdef
		S_StopAllSounds_f ();
		cl.refresh_prepped = false;

#ifndef SINGLE_RENDERER
		static char lastRenderer[MAX_QPATH];

		bool loaded = LoadRenderer (vid_ref->string);
		if (!loaded && lastRenderer[0])
		{
			if (LoadRenderer (lastRenderer))
			{
				Cvar_Set ("vid_ref", lastRenderer);
				vid_ref->modified = false;
				loaded = true;
			}
		}
		if (!loaded) Com_FatalError ("Couldn't load renderer");
		strcpy (lastRenderer, vid_ref->string);
#else
		if (!LoadRenderer ()) Com_FatalError ("Couldn't load renderer");
#endif
		needRestart = false;
	}

	// update our window position
	if (vid_xpos->modified || vid_ypos->modified)
	{
		if (!FullscreenApp)
			Vid_UpdateWindowPosAndSize (vid_xpos->integer, vid_ypos->integer);

		vid_xpos->modified = false;
		vid_ypos->modified = false;
	}
}

/*
============
Vid_Init
============
*/
void Vid_Init (void)
{
CVAR_BEGIN(vars)
#ifndef SINGLE_RENDERER
	CVAR_FULL(&vid_ref, "vid_ref", DEFAULT_RENDERER, CVAR_ARCHIVE),
#endif
	CVAR_VAR(vid_xpos, 0, CVAR_ARCHIVE),
	CVAR_VAR(vid_ypos, 0, CVAR_ARCHIVE),
	CVAR_VAR(win_noalttab, 0, CVAR_ARCHIVE),
	CVAR_VAR(win_priorityBoost, 0, CVAR_ARCHIVE)
CVAR_END

	guard(Vid_Init);

	needRestart = true;		// should init renderer on startup
	Cvar_GetVars (ARRAY_ARG(vars));
	InitRendererVars ();

	// Add some console commands that we want to handle
	RegisterCommand ("vid_restart", Vid_Restart);

	OSVERSIONINFO vinfo;
	vinfo.dwOSVersionInfoSize = sizeof(vinfo);
	GetVersionEx (&vinfo);
	if (vinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
	{
		s_win95 = true;
		MSH_MOUSEWHEEL = RegisterWindowMessage ("MSWHEEL_ROLLMSG");
	}

#if 0
	// this is a gross hack but necessary to clamp the mode for 3Dfx
	{
		cvar_t *gl_driver = Cvar_Get ("gl_driver", "opengl32", 0);
		cvar_t *gl_mode = Cvar_Get ("gl_mode", "3", 0);

		if (stricmp (gl_driver->string, "3dfxgl") == 0)
		{
			Cvar_SetInteger ("gl_mode", 3);
			viddef.width  = 640;
			viddef.height = 480;
		}
	}
#endif
	// Disable the 3Dfx splash screen
	putenv ("FX_GLIDE_NO_SPLASH=0");

	// Create invisible (fake) window to capture Win32 focus
	Vid_CreateWindow (0, 0, false);

	// Start the graphics mode and load refresh DLL
	Vid_CheckChanges ();

	unguard;
}

/*
============
Vid_Shutdown
============
*/
void Vid_Shutdown (void)
{
	if (refActive)
	{
		// perform shutdown
		RE_Shutdown (true);
		Vid_DestroyWindow (true);
		FreeRenderer ();
	}
}
