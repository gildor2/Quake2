#include "WinPrivate.h"
#if MAX_DEBUG
#	include <float.h>
#endif
#include "../client/client.h"

#if __MINGW32__
// for VC included automatically
#include <pbt.h>
#endif

#include "resource.h"


#define DEFAULT_RENDERER	"gl"		// for vid_ref cvar

cvar_t *win_noalttab, *win_priorityBoost;
static cvar_t *vid_xpos, *vid_ypos;		// coordinates of window position

// win95 & WinNT3.51 mouse wheel support
static UINT MSH_MOUSEWHEEL = 0xDEAD0000; // initialized to something unused

#if !SINGLE_RENDERER

static cvar_t	*vid_ref;
static CDynamicLib refLibrary;			// handle to renderer DLL; rename ??
// Imports from main engine for renderer DLL
#include "../client/engine_exp.h"

// Structure containing functions exported from renderer DLL
refExport_t	re;

#endif // SINGLE_RENDERER

viddef_t viddef;						// global video state; used by other modules
static bool refActive = false;

HWND	cl_hwnd;						// main window handle

static void DisableAltTab(bool value)
{
	static bool disabled = false;
	if (disabled == value) return;
	disabled = value;

	if (!GIsWinNT)
	{
		BOOL old;
		SystemParametersInfo(SPI_SCREENSAVERRUNNING, value, &old, 0);
	}
	else
	{
		// this code does not works with WinNT 4.0 +
		if (value)
		{
			RegisterHotKey(0, 0, 1/*MOD_ALT*/, VK_TAB);	// MOD_ALT is redefined, so use winuser.h const directly
			// really, should disable <Win>, <Ctrl+Esc>, <Alt+Esc> key combinations too; see Q226359 in MSDN
		}
		else
			UnregisterHotKey(0, 0);
	}
}

static void SetHighPriority(bool enable)
{
	SetPriorityClass(GetCurrentProcess(), enable ? HIGH_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS);
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


static int MapKey(int vkCode, bool extended)
{
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
	int key = vkToKey[vkCode];
	if (key == UNK) key = vkCode + 256;
	return key;
}


static void AppActivate(bool active, bool minimized)
{
//	appPrintf("act:%d min:%d\n", active, minimized);//!!
	MinimizedApp = minimized;
	if (ActiveApp != active)
	{
		ActiveApp = active;

		Key_ClearStates();
		//!! should be callbacks (or system.virtualMethod())
		IN_Activate(active);
		CDAudio_Activate(active);
		S_Activate(active);
		if (refActive) RE_AppActivate(active);

		if (win_noalttab->integer)		DisableAltTab(active);
		if (win_priorityBoost->integer)	SetHighPriority(active);
	}
}


/*-----------------------------------------------------------------------------
	Message processing
-----------------------------------------------------------------------------*/

static MSGHOOK_FUNC hooks[8];

void AddMsgHook(MSGHOOK_FUNC func)
{
	int idx = -1;
	for (int i = 0; i < ARRAY_COUNT(hooks); i++)
	{
		if (hooks[i] == func) return;	// already hooked
		if (!hooks[i] && idx < 0) idx = i;
	}
	if (idx < 0) appError("max msg hooks");
	hooks[idx] = func;
}

void RemoveMsgHook(MSGHOOK_FUNC func)
{
	for (int i = 0; i < ARRAY_COUNT(hooks); i++)
	{
		if (hooks[i] == func)
		{
			hooks[i] = NULL;
			return;
		}
	}
	Com_DPrintf("RemoveMsgHook: hook is not installed\n");
}


static LONG WINAPI MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	guard(MainWndProc);

	MSGLOG(("msg=%X w=%X l=%X\n", uMsg, wParam, lParam));
	if (uMsg == MSH_MOUSEWHEEL) uMsg = WM_MOUSEWHEEL;

	guard(callHook);
	for (int i = 0; i < ARRAY_COUNT(hooks); i++)
	{
		if (hooks[i])
		{
			if (hooks[i](uMsg, wParam, lParam)) return 0;
		}
	}
	unguard;

	switch (uMsg)
	{
	case WM_HOTKEY:
		return 0;

	case WM_SIZE:
		if (FullscreenApp)
		{
			static DEVMODE dm;
			static bool desktopMode = false;

			MSGLOG(("WM_SIZE: %dx%d wp=%d (act=%d min=%d)\n", LOWORD(lParam), HIWORD(lParam), wParam, ActiveApp, MinimizedApp));
			if (wParam == SIZE_MINIMIZED && !desktopMode)
			{
				Com_DPrintf("Setting desktop resolution\n");
				// save screen parameters
				HDC dc = GetDC(NULL);
				dm.dmSize       = sizeof(dm);
				dm.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
				//!! can get HORZRES/VERTRES/BITSPIXEL from our SetDisplayMode() - no GetDeviceCaps() needed!
				dm.dmPelsWidth  = GetDeviceCaps(dc, HORZRES);
				dm.dmPelsHeight = GetDeviceCaps(dc, VERTRES);
				dm.dmBitsPerPel = GetDeviceCaps(dc, BITSPIXEL);
				// restore mode
				MSGLOG(("CDS(NULL)\n"));
				ChangeDisplaySettings(NULL, 0);
				desktopMode = true;
			}
			else if (wParam == SIZE_RESTORED && MinimizedApp && desktopMode)
			{
				Com_DPrintf("Setting game resolution\n");
				MSGLOG(("CDS(%dx%d, FS)\n", dm.dmPelsWidth, dm.dmPelsHeight));
				ChangeDisplaySettings(&dm, CDS_FULLSCREEN);
				SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE);
				SetForegroundWindow(hWnd);
				desktopMode = false;
			}
		}
		break;

	case WM_ACTIVATE:
		{
			bool active    = LOWORD(wParam) != WA_INACTIVE;
			bool minimized = HIWORD(wParam) != 0;
			MSGLOG(("WM_ACTIVATE: a=%d m=%d (act=%d min=%d)\n",active, minimized, ActiveApp, MinimizedApp));
			AppActivate(active, minimized);
		}
		break;

	case WM_MOVE:
		MSGLOG(("WM_MOVE: x=%d, y=%d\n", LOWORD(lParam), HIWORD(lParam)));
		if (!FullscreenApp)
		{
			// using (short) type conversion to convert "unsigned short" to signed
			int xPos = (short) LOWORD(lParam);    // horizontal position
			int yPos = (short) HIWORD(lParam);    // vertical position
			// now xPos and yPos contains client area coordinates, so - convert to window coordinates
			RECT r;
			r.left   = 0;
			r.top    = 0;
			r.right  = 1;
			r.bottom = 1;
			AdjustWindowRect(&r, GetWindowLong(hWnd, GWL_STYLE), FALSE);
			// update cvars
			Cvar_SetInteger("vid_xpos", xPos + r.left);
			Cvar_SetInteger("vid_ypos", yPos + r.top);
			vid_xpos->modified = false;
			vid_ypos->modified = false;
			if (ActiveApp) IN_Activate(true);
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

	//?? use AddMgsHook to capture keyboard messages
	case WM_SYSKEYDOWN:
		if (wParam == VK_RETURN)	// Alt+Enter
		{
			Cvar_SetInteger("r_fullscreen", !FullscreenApp);
			return 0;
		}
		// fall through
	case WM_KEYDOWN:
		{
			int k = MapKey(wParam, (lParam >> 24) & 1);
			if (k) Key_Event(k, true);
		}
		return 0;

	case WM_SYSKEYUP:
	case WM_KEYUP:
		{
			int k = MapKey(wParam, (lParam >> 24) & 1);
			if (k) Key_Event(k, false);
		}
		return 0;

	case WM_POWERBROADCAST:
		if (wParam == PBT_APMSUSPEND)	// will minimize window when in fullscreen mode
			RE_AppActivate(false);
		//?? can also disconnect all remote (not local) clients from server (if one)
		break;

	case WM_CLOSE:
		GIsRequestingExit = true;
		break;							// should not return here

	case WM_ERASEBKGND:
		return 1;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);

	unguardf(("msg=%X", uMsg));
}


/*-----------------------------------------------------------------------------
	Window creation/destroying
-----------------------------------------------------------------------------*/

//#define ALLOW_WND_RESIZE	1

void *Vid_CreateWindow(int width, int height, bool fullscreen)
{
	guard(Vid_CreateWindow);

	MSGLOG(("CreateWindow(%d, %d, fs=%d)\n", width, height, fullscreen));
	FullscreenApp = fullscreen;

#if !ALLOW_WND_RESIZE
	HWND oldWnd = cl_hwnd;
#endif

	DWORD stylebits, exstyle;
	int x, y, w, h;
	if (fullscreen)
	{
		stylebits = WS_POPUP|WS_SYSMENU;
		exstyle   = WS_EX_TOPMOST;
		x = 0;
		y = 0;
		w = width;
		h = height;
	}
	else
	{
		stylebits = WS_CAPTION|WS_SYSMENU;
		exstyle   = 0;
		// compute window pos/size
		RECT r;
		r.left   = 0;
		r.top    = 0;
		r.right  = width;
		r.bottom = height;
		AdjustWindowRect(&r, stylebits, FALSE);
		x = vid_xpos->integer;
		y = vid_ypos->integer;
		w = r.right - r.left;
		h = r.bottom - r.top;
	}

	// if size=0 -- invisible (fake) window
//	if (width || height)		// if enable this, window will be created without taskbar button!
//		stylebits |= WS_VISIBLE;

#if ALLOW_WND_RESIZE
	if (cl_hwnd)
	{
		// window already exists - simply reposition it
		SetWindowLong(cl_hwnd, GWL_STYLE, stylebits);
		// NOTE: SetWindowLong(wnd, GWL_EXSTYLE, WS_EX_TOPMOST) works bad, SetWindowPos() have a better effect
		ShowWindow(cl_hwnd, SW_SHOW);
		SetWindowPos(cl_hwnd, fullscreen ? HWND_TOPMOST : HWND_NOTOPMOST, x, y, w, h, 0);
	}
	else
#endif
	{
		// create a window
		const char *app = appPackage();
#if !ALLOW_WND_RESIZE
		if (!oldWnd)			// when oldWnd exists, window class already registered
#endif
		{
			// Register the window class
			WNDCLASS wc;
			memset(&wc, 0, sizeof(WNDCLASS));
			wc.lpfnWndProc   = MainWndProc;
			wc.hInstance     = hInstance;
			wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
			wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
//			wc.hbrBackground = (HBRUSH) COLOR_GRAYTEXT;
			wc.lpszClassName = app;
			if (!RegisterClass(&wc)) appError("Cannot register window class");
		}

		cl_hwnd = CreateWindowEx(exstyle, app, app, stylebits, x, y, w, h, NULL, NULL, hInstance, NULL);
		if (!cl_hwnd) appError("Cannot create window");

		if (width || height) ShowWindow(cl_hwnd, SW_SHOW);
		// let the sound and input subsystems know about the new window
		cl.forceViewFrame = true;
		in_needRestart = true;
	}

	viddef.width  = width;
	viddef.height = height;

	UpdateWindow(cl_hwnd);
	SetForegroundWindow(cl_hwnd);
	SetFocus(cl_hwnd);		//?? capture keyboard; is it really necessary ?

#if !ALLOW_WND_RESIZE
	if (oldWnd) DestroyWindow(oldWnd);
#endif

	return cl_hwnd;

	unguard;
}


void Vid_DestroyWindow(bool force)
{
//force = true; //???
	if (!force)	//?? add cvar "win_singleWindow"
	{
//		ShowWindow(cl_hwnd, SW_HIDE);
		return;
	}

	Com_DPrintf("...destroying window\n");
	if (cl_hwnd)
	{
//		ShowWindow(cl_hwnd, SW_HIDE);	-- this will force to CDS(0,0) when vid_restart, because Activate(0)->Minimize()->WM_SIZE
		DestroyWindow(cl_hwnd);
		cl_hwnd = 0;
	}

	UnregisterClass(appPackage(), hInstance);
}


/*-------------------------------------------*/

static bool needRestart;

void Vid_Restart()
{
	needRestart = true;
	M_ForceMenuOff();		// required: menu graphics may be required to reload
}


static const struct {
	short width, height;
} vid_modes[] = {
	{320,	240},
	{400,	300},
	{512,	384},
	{640,	480},
	{800,	600},
	{960,	720},
	{1024,	768},
	{1152,	864},
	{1280,	960},
	{1280,  1024},
	{1600,	1200},
	{1680,  1050},
	{1920,  1200},
	{2560,  1600}
};


bool Vid_GetModeInfo(int *width, int *height, int mode)
{
	if (mode < 0 || mode >= ARRAY_COUNT(vid_modes))
		return false;

	*width  = vid_modes[mode].width;
	*height = vid_modes[mode].height;

	return true;
}


static void Vid_UpdateWindowPosAndSize(int x, int y)
{
	RECT r;
	r.left   = 0;
	r.top    = 0;
	r.right  = viddef.width;
	r.bottom = viddef.height;
	AdjustWindowRect(&r, GetWindowLong(cl_hwnd, GWL_STYLE), FALSE);

	int w = r.right - r.left;
	int h = r.bottom - r.top;
	MoveWindow(cl_hwnd, vid_xpos->integer, vid_ypos->integer, w, h, TRUE);
}


//!! can move FreeRenderer(), LoadRenderer() and major part of Vid_Tick() to client (platform-independent)
//!! + move needRestart and Vid_Restart() func; will require to load renderer
//?? Really, renderer system should be redesigned (split backend--frontend, frontend will load/init backend ...
//?? so, this code will be removed at all.

static void FreeRenderer()
{
	refActive = false;
#if !SINGLE_RENDERER
	refLibrary.Free();		// if statically linked, handle = NULL ...
	memset(&re, 0, sizeof(re));
#endif
}


#if STATIC_BUILD && !SINGLE_RENDERER
// externs
namespace OpenGLDrv {
	extern const refExport_t re;
}
#endif

#if !SINGLE_RENDERER
static bool LoadRenderer(const char *name)
#else
static bool LoadRenderer()
#endif
{
	guard(LoadRenderer);

#if MAX_DEBUG
	// when FPU exceptions are enabled, game may crash in DirectSoundCreate()
	// note: exceptions will be re-enabled in next MainLoop iteration
	_controlfp(_controlfp(0, 0) | _EM_ZERODIVIDE|_EM_INVALID, _MCW_EM);
#endif

	if (refActive)
	{
		RE_Shutdown();
		FreeRenderer();
	}

#if !SINGLE_RENDERER

	appPrintf("Loading %s\n", name);

#if STATIC_BUILD
	if (!strcmp(name, "gl"))
		re = OpenGLDrv::re;
	else
#endif
	{
		TString<256> DllName;
		DllName.sprintf("ref_%s" DLLEXT, name);
		if (!(refLibrary.Load(DllName)))
		{
			appWPrintf("Load(\"%s\") failed\n", *DllName);
			return false;
		}
		CreateDynRenderer_t pCreateRenderer;
		if (!refLibrary.GetProc("CreateRenderer", &pCreateRenderer))
		{
			appWPrintf("GetProc() failed on %s\n", *DllName);
			FreeRenderer();
			return false;
		}

		re.struc_size = sizeof(refExport_t);
		if (!pCreateRenderer(&ri, &re))
		{
			appWPrintf("%s has incompatible renderer\n", *DllName);
			FreeRenderer();
			return false;
		}
	}
#endif // SINGLE_RENDERER

	if (!RE_Init())
	{
		RE_Shutdown();
		FreeRenderer();
		return false;
	}

	appPrintf("------------------------------------\n");
	refActive = true;

	return true;

	unguard;
}


void Vid_Tick()
{
	if (win_noalttab->modified)
	{
		DisableAltTab(win_noalttab->integer != 0);
		win_noalttab->modified = false;
	}

	if (win_priorityBoost->modified)
	{
		SetHighPriority(win_priorityBoost->integer != 0);
		win_priorityBoost->modified = false;
	}

#if !SINGLE_RENDERER
	if (vid_ref->modified)
	{
		needRestart = true;
		vid_ref->modified = false;
	}
#endif

	if (needRestart)
	{
		// renderer has changed
		cl.forceViewFrame = true;
		S_StopAllSounds_f();
		cl.rendererReady = false;

#if !SINGLE_RENDERER
		static char lastRenderer[MAX_QPATH];

		bool loaded = LoadRenderer(vid_ref->string);
		if (!loaded && lastRenderer[0])
		{
			if (LoadRenderer(lastRenderer))
			{
				Cvar_Set("vid_ref", lastRenderer);
				vid_ref->modified = false;
				loaded = true;
			}
		}
		if (!loaded) appError("Cannot load renderer");
		strcpy(lastRenderer, vid_ref->string);
#else
		if (!LoadRenderer()) appError("Cannot load renderer");
#endif
		needRestart = false;
	}

	// update our window position
	if (vid_xpos->modified || vid_ypos->modified)
	{
		if (!FullscreenApp)
			Vid_UpdateWindowPosAndSize(vid_xpos->integer, vid_ypos->integer);

		vid_xpos->modified = false;
		vid_ypos->modified = false;
	}
}


void Vid_Init()
{
CVAR_BEGIN(vars)
#if !SINGLE_RENDERER
	CVAR_FULL(&vid_ref, "vid_ref", DEFAULT_RENDERER, CVAR_ARCHIVE),
#endif
	CVAR_VAR(vid_xpos, 0, CVAR_ARCHIVE),
	CVAR_VAR(vid_ypos, 0, CVAR_ARCHIVE),
	CVAR_VAR(win_noalttab, 0, CVAR_ARCHIVE),
	CVAR_VAR(win_priorityBoost, 0, CVAR_ARCHIVE)
CVAR_END

	guard(Vid_Init);

	Cvar_GetVars(ARRAY_ARG(vars));
	InitRendererVars();

	// add some console commands that we want to handle
	RegisterCommand("vid_restart", Vid_Restart);

	if (!GIsWin2K)			// for WinNT3.51 and Win95 only
		MSH_MOUSEWHEEL = RegisterWindowMessage("MSWHEEL_ROLLMSG");

	// create invisible (fake) window to capture Win32 focus
	Vid_CreateWindow(0, 0, false);

	// load renderer
	needRestart = true;		// should init renderer on startup
	Vid_Tick();

	unguard;
}


void Vid_Shutdown()
{
	if (refActive)
	{
		// perform shutdown
		RE_Shutdown(true);
		Vid_DestroyWindow(true);
		FreeRenderer();
	}
}
