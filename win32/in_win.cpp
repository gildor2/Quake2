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

//#define DIRECTINPUT_VERSION  0x0700
#define DIRECTINPUT_VERSION  0x0800

//#define INITGUID
#include "winquake.h"
#include "dinput.h"

/*-----------------------------------------------------------------------------
	Some DirectInput GUIDs
	Placed here to reduce executable size (if create using #define INITGUID
	before #include "dinput.h", ALL DirectInput GUIDs will be created)
-----------------------------------------------------------------------------*/

#if DIRECTINPUT_VERSION < 0x0800
#pragma comment (lib, "dinput.lib")
#else
#pragma comment (lib, "dinput8.lib")
#endif

#ifndef INITGUID

#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
		EXTERN_C const GUID name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

#if DIRECTINPUT_VERSION < 0x0800
DEFINE_GUID(IID_IDirectInputA,	0x89521360,0xAA8A,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
#else
DEFINE_GUID(IID_IDirectInput8A,	0xBF798030,0x483A,0x4DA2,0xAA,0x99,0x5D,0x64,0xED,0x36,0x97,0x00);
#endif

DEFINE_GUID(GUID_SysMouse,	0x6F1D2B60,0xD5A0,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
//DEFINE_GUID(GUID_SysKeyboard,0x6F1D2B61,0xD5A0,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
//DEFINE_GUID(GUID_Joystick,	0x6F1D2B70,0xD5A0,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);

DEFINE_GUID(GUID_XAxis, 0xA36D02E0,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_YAxis, 0xA36D02E1,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);
DEFINE_GUID(GUID_ZAxis, 0xA36D02E2,0xC9F3,0x11CF,0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00);

#endif

#include "../client/client.h"


/*!! Add DirectInput keyboard support, remove win32 mouse/keyboard support; joystick - DirectInput only;
 *   After this, can change "in_mouse==0|1|2" to "nomouse" cvar + can remove mouse/keyboard code from MainWndProc()
 *   NOTE: this will require at least DirectX 7/8 ... May be, keep Win32 mouse/keyboard (just in case - its code not too large)?
 *   AFTER: remove GPL/id copyright
 */

bool in_needRestart;


/*-----------------------------------------------------------------------------
	Mouse looking stuff
-----------------------------------------------------------------------------*/

//?? This is a rudiment from old non-mouse games; remove +mlooking at all?! (make always ON)
//?? movement rudiments:
//??	+mlook: button version of freelook
//??	+klook: look with keyboard (!freelook, button version)
//??	+strafe: when 1, moving mouse left<->right will strafe (in ANY mode): sidemove != 0
//??	lookstrafe: when mlook=1, moving mouse left<->right will strafe
//??	lookspring: when mlook=0 (moving with mouse), "centerview" will be called with any mouse move
//??	freelook: allow to look with mouse (normal behaviour)
//?? Remove: mlook=1, klook=0, strafe=0, lookstrafe=0, lookspring=? (should not be path at all), freelook=1
static bool mlooking;

static void MLookDown ()
{
	mlooking = true;
}

static void MLookUp ()
{
	mlooking = false;
	if (!freelook->integer && lookspring->integer)
		IN_CenterView ();
}

//-----------------------------------------------------------------------------


// forwards
void MouseEvent (unsigned buttons);

#define BUFFERED_MOUSE

// mouse variables
static cvar_t	*in_mouse;
static cvar_t	*m_filter;


/*-----------------------------------------------------------------------------
	DirectInput mouse
-----------------------------------------------------------------------------*/

static int	move_x, move_y;

static HINSTANCE hInstDI;

#if DIRECTINPUT_VERSION < 0x0800
static IDirectInput *pDI;
static IDirectInputDevice *pMouse;
#else
static IDirectInput8 *pDI;
static IDirectInputDevice8 *pMouse;
#endif
static int mouseType;		// copy of in_mouse


static void DXMouse_Free ()
{
	Com_DPrintf ("Shutdown DirectInput mouse\n");
	if (pDI)
	{
		if (pMouse)
		{
			pMouse->Unacquire ();
			pMouse->Release ();
			pMouse = NULL;
		}
		pDI->Release ();
		pDI = NULL;
	}
	if (hInstDI)
	{
		FreeLibrary (hInstDI);
		hInstDI = NULL;
	}
	ShowCursor (TRUE);
}


static bool DXMouse_Init ()
{
	Com_Printf ("Initializing DirectInput\n");
#if DIRECTINPUT_VERSION < 0x0800
	typedef HRESULT (WINAPI * pDirectInputCreate_t) (HINSTANCE, DWORD, LPDIRECTINPUT *, LPUNKNOWN);
	static pDirectInputCreate_t pDirectInputCreate;
	if (!hInstDI)
	{
		if (!(hInstDI = LoadLibrary ("dinput.dll")))
		{
			Com_WPrintf ("dinput.dll not found\n");
			return false;
		}
		if (!(pDirectInputCreate = (pDirectInputCreate_t) GetProcAddress (hInstDI, "DirectInputCreateA")))
		{
			Com_WPrintf ("Wrong dinput.dll\n");
			DXMouse_Free ();
			return false;
		}
	}
	if FAILED(pDirectInputCreate (global_hInstance, DIRECTINPUT_VERSION, &pDI, NULL))
	{
		Com_DPrintf ("... cannot create DirectInput object\n");
		DXMouse_Free ();
		return false;
	}

#else
	typedef HRESULT (WINAPI * pDirectInput8Create_t) (HINSTANCE, DWORD, REFIID, LPVOID *, LPUNKNOWN);
	static pDirectInput8Create_t pDirectInput8Create;
	if (!hInstDI)
	{
		if (!(hInstDI = LoadLibrary ("dinput8.dll")))
		{
			Com_WPrintf ("dinput8.dll not found\n");
			return false;
		}
		if (!(pDirectInput8Create = (pDirectInput8Create_t) GetProcAddress (hInstDI, "DirectInput8Create")))
		{
			Com_WPrintf ("Wrong dinput8.dll\n");
			DXMouse_Free ();
			return false;
		}
	}
	if FAILED(pDirectInput8Create (global_hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&pDI, NULL))
	{
		Com_DPrintf ("... cannot create DirectInput object\n");
		DXMouse_Free ();
		return false;
	}

#endif

	if FAILED(pDI->CreateDevice (GUID_SysMouse, &pMouse, NULL))
	{
		Com_DPrintf ("... cannot create DirectInput mouse\n");
		DXMouse_Free ();
		return false;
	}

#if DIRECTINPUT_VERSION >= 0x0700
	HRESULT res = pMouse->SetDataFormat (&c_dfDIMouse2);	// up to 8 mouse buttons
#else
	HRESULT res = pMouse->SetDataFormat (&c_dfDIMouse);		// up to 4 mouse buttons
#endif
	if FAILED(res)
	{
		Com_Printf ("... cannot set data format\n");
		DXMouse_Free ();
		return false;
	}
	if FAILED(pMouse->SetCooperativeLevel (cl_hwnd, DISCL_EXCLUSIVE|DISCL_FOREGROUND))
	{
		//?? set fullscreen to 0 -- if cannot capture mouse in fullscreen mode
		//?? (if fullscreen is already 0, it will not be changed)
		Com_Printf ("... cannot set cooperative level\n");
		DXMouse_Free ();
		return false;
	}
#ifdef BUFFERED_MOUSE
	DIPROPDWORD dipdw;
	// the header
	dipdw.diph.dwSize		= sizeof(DIPROPDWORD);
	dipdw.diph.dwHeaderSize	= sizeof(DIPROPHEADER);
	dipdw.diph.dwObj		= 0;
	dipdw.diph.dwHow		= DIPH_DEVICE;
	// the data
	dipdw.dwData			= 16;							// size of buffer
	if FAILED(pMouse->SetProperty (DIPROP_BUFFERSIZE, &dipdw.diph))
	{
		Com_Printf ("... cannot set mouse buffered mode\n");
		DXMouse_Free ();
		return false;
	}
#endif
#if 0
	if FAILED(pMouse->Acquire ())
	{
		//?? fullscreen <- 0
		Com_Printf ("... cannot acquire mouse\n");
		//?? do not Free() -- just keep mouse unacquired; when window will be activated -- acquire mouse
		IN_DXMouse_Free ();
		return false;
	}
#endif

	ShowCursor (FALSE);
	SetCursor (NULL);		// DirectX manual says: "cursor will disappear"; but, in some circusmances it appears ...
	return true;
}


static void DXMouse_Frame ()
{
	// poll DirectInput mouse
#ifdef BUFFERED_MOUSE
	move_x = move_y = 0;
	while (true)
	{
		DIDEVICEOBJECTDATA od;
		DWORD	dwElements = 1;			// number of items to be retreived
		HRESULT res = pMouse->GetDeviceData (sizeof(od), &od, &dwElements, 0);
		if (res == DIERR_INPUTLOST || res == DIERR_NOTACQUIRED)
		{
			// try to acquire mouse
			pMouse->Acquire ();
			return;
		}
		if FAILED(res)
		{
			Com_WPrintf ("Error %X on DI mouse.GetData()\n", res);
			return;
		}
		if (dwElements == 0) return;	// no data available
#if DIRECTINPUT_VERSION >= 0x0700
		if (od.dwOfs >= DIMOFS_BUTTON0 && od.dwOfs <= DIMOFS_BUTTON7)
#else
		if (od.dwOfs >= DIMOFS_BUTTON0 && od.dwOfs <= DIMOFS_BUTTON3)
#endif
		{
			Key_Event (K_MOUSE1 + od.dwOfs - DIMOFS_BUTTON0, (od.dwData & 0x80) != 0);	//?? can use od.dwTimeStamp for timing
			continue;
		}

		switch (od.dwOfs)
		{
		case DIMOFS_X:
			move_x += od.dwData;
			break;
		case DIMOFS_Y:
			move_y += od.dwData;
			break;
		case DIMOFS_Z:
			int msg = ((int)od.dwData > 0) ? K_MWHEELUP : K_MWHEELDOWN;
			Key_Event (msg, true);
			Key_Event (msg, false);
			break;
		}
	}
#else // BUFFERED_MOUSE
#if DIRECTINPUT_VERSION >= 0x0700
	DIMOUSESTATE2 ms;
#else
	DIMOUSESTATE ms;
#endif
	HRESULT res = pMouse->GetDeviceState (sizeof(ms), &ms);
	if (res == DIERR_INPUTLOST || res == DIERR_NOTACQUIRED)
	{
		// try to acquire mouse
		pMouse->Acquire ();
		return;
	}
	if FAILED(res)
	{
		Com_WPrintf ("Error %X on DI mouse.GetState()\n", res);
		return;
	}
	// process mouse wheel
	if (ms.lZ)
	{
		int msg = (ms.lZ > 0) ? K_MWHEELUP : K_MWHEELDOWN;
		Key_Event (msg, true);
		Key_Event (msg, false);
	}
	// process X and Y axes
	move_x = ms.lX;
	move_y = ms.lY;
	// process buttons
	int tmp = 0;
	for (int i = 0; i < ARRAY_COUNT(ms.rgbButtons); i++)
		if (ms.rgbButtons[i]) tmp |= 1 << i;
	MouseEvent (tmp);			// call this always - to detect button on/off
#endif // BUFFERED_MOUSE
}


/*-----------------------------------------------------------------------------
	Standard Win32 mouse
-----------------------------------------------------------------------------*/

static int	window_center_x, window_center_y;

static bool	haveSpiMouse, haveSpiMouseSpeed;
static int	originalMouseParms[3], originalMouseSpeed;

static bool MouseMsgHook (UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_MOUSEWHEEL:
		// this chunk of code theoretically only works under NT4+ and Win98+
		// since this message doesn't exist under Win95
		if (wParam >> 31)	// really, HIWORD(wParam) should be checked, but LOWORD() does not affects the sign of result
		{
			Key_Event (K_MWHEELDOWN, true);
			Key_Event (K_MWHEELDOWN, false);
		}
		else
		{
			Key_Event (K_MWHEELUP, true);
			Key_Event (K_MWHEELUP, false);
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
			MouseEvent (temp);
		}
		break;
	}

	return false;
}


static void WinMouse_Init ()
{
	Com_Printf ("Initializing Win32 mouse support\n");
	AddMsgHook (MouseMsgHook);

	static int mouseParms[3] = {0, 0, 0};
	static int mouseSpeed = 10;
	if (haveSpiMouse)
		SystemParametersInfo (SPI_SETMOUSE, 0, mouseParms, 0);
	if (haveSpiMouseSpeed)
		SystemParametersInfo (SPI_SETMOUSESPEED, 0, &mouseSpeed, 0);

	int width = GetSystemMetrics (SM_CXSCREEN);
	int height = GetSystemMetrics (SM_CYSCREEN);

	RECT window_rect;
	GetWindowRect (cl_hwnd, &window_rect);
	if (window_rect.left < 0)	window_rect.left = 0;
	if (window_rect.top < 0) 	window_rect.top = 0;
	if (window_rect.right >= width)		window_rect.right = width-1;
	if (window_rect.bottom >= height-1)	window_rect.bottom = height-1;

	window_center_x = (window_rect.right + window_rect.left) / 2;
	window_center_y = (window_rect.top + window_rect.bottom) / 2;

	SetCursorPos (window_center_x, window_center_y);

	SetCapture (cl_hwnd);
	ClipCursor (&window_rect);

	SetCursor (NULL);
	ShowCursor (FALSE);
}


static void WinMouse_Free ()
{
	Com_DPrintf ("Shutdown Win32 mouse\n");
	RemoveMsgHook (MouseMsgHook);
	if (haveSpiMouse)
		SystemParametersInfo (SPI_SETMOUSE, 0, originalMouseParms, 0);
	if (haveSpiMouseSpeed)
		SystemParametersInfo (SPI_SETMOUSESPEED, 0, &originalMouseSpeed, 0);

	ClipCursor (NULL);
	ReleaseCapture ();
	ShowCursor (TRUE);
}


/*-----------------------------------------------------------------------------
	Common mouse support
-----------------------------------------------------------------------------*/

static void DeactivateMouse (bool complete)
{
	if (mouseType == 2)
	{
		if (complete)
			DXMouse_Free ();
		else
			pMouse->Unacquire ();
	}
	else if (mouseType == 1)
	{
		WinMouse_Free ();
		complete = true;		// required
	}

	if (complete)
		mouseType = 0;
}


static void ActivateMouse ()
{
	in_mouse->modified = false;
	if (mouseType == in_mouse->integer) return;

	if (mouseType)
	{
		if (!in_mouse->modified)
			return;
		Com_DPrintf ("Changing mouse type\n");
		// deactivate then reactivate mouse
		DeactivateMouse (true);
	}

	if (in_mouse->integer == 2 && DXMouse_Init ())
		mouseType = 2;
	else if (in_mouse->integer)
	{
		WinMouse_Init ();
		mouseType = 1;
	}
	else
		mouseType = 0;
}


static void StartupMouse ()
{
	haveSpiMouse = SystemParametersInfo (SPI_GETMOUSE, 0, originalMouseParms, 0) != 0;
	haveSpiMouseSpeed = SystemParametersInfo (SPI_GETMOUSESPEED, 0, &originalMouseSpeed, 0) != 0;	// Win98+, Win2K+
}


static void MouseEvent (unsigned buttons)
{
	static unsigned oldButtons;
	unsigned old = oldButtons;
	oldButtons = buttons;

	// perform button actions
	int key = K_MOUSE1;
	while (buttons || old)
	{
		if ((buttons ^ old) & 1)
			Key_Event (key, buttons & 1);

		buttons >>= 1;
		old >>= 1;
		key++;
	}
}


static void MouseMove (usercmd_t *cmd)
{
	static int	mouse_x, mouse_y, old_mouse_x, old_mouse_y;
	int		mx, my;

	if (!mouseType)
		return;

	if (mouseType == 2)
	{
		mx = move_x;
		my = move_y;
		DXMouse_Frame ();
	}
	else
	{
		// get mouse position
		POINT current_pos;
		if (!GetCursorPos (&current_pos)) return;
		// compute movement
		mx = current_pos.x - window_center_x;
		my = current_pos.y - window_center_y;
		// force the mouse to the center, so there's room to move
		SetCursorPos (window_center_x, window_center_y);
	}

#if 0
	if (!mx && !my)
		return;
#endif

	// filter mouse movement
	if (m_filter->integer)
	{
		mouse_x = (mx + old_mouse_x) / 2;
		mouse_y = (my + old_mouse_y) / 2;
	}
	else
	{
		mouse_x = mx;
		mouse_y = my;
	}

	old_mouse_x = mx;
	old_mouse_y = my;

	mouse_x = appRound (mouse_x * sensitivity->value * 1.5f);
	mouse_y = appRound (mouse_y * sensitivity->value * 1.5f);

	//!! NOTE: following code is the same on all platforms (can be migrated to cl_input.cpp)
	//!! If do the same with joystick code, we can set in_Strafe, m_pitch etc to static
	// add mouse X/Y movement to cmd
	if ((in_Strafe.state & 1) || (lookstrafe->integer && mlooking))
		cmd->sidemove += appRound (m_side->value * mouse_x);
	else
		cl.viewangles[YAW] -= m_yaw->value * mouse_x;

	if ((mlooking || freelook->integer) && !(in_Strafe.state & 1))
		cl.viewangles[PITCH] += m_pitch->value * mouse_y;
	else
		cmd->forwardmove -= appRound (m_forward->value * mouse_y);
}


/*-----------------------------------------------------------------------------
	Win32 accessibility feature
-----------------------------------------------------------------------------*/

#define LIST		\
	F(FILTERKEYS)	\
	F(HIGHCONTRAST)	\
	F(MOUSEKEYS)	\
	F(STICKYKEYS)	\
	F(TOGGLEKEYS)

// declare variables
#define F(type)		\
static type acc##type;	\
static bool have##type;
LIST
#undef F

// save state and disable accessibility
#define F(type)		\
	acc##type.cbSize = sizeof(type); \
	have##type = SystemParametersInfo (SPI_GET##type, sizeof(type), &acc##type, 0) != 0; \
	if (have##type) {			\
		type tmp = acc##type;	\
		tmp.dwFlags = 0;		\
		SystemParametersInfo (SPI_SET##type, sizeof(type), &tmp, 0); \
	}

static void DisableAccessibility ()
{
	LIST
}
#undef F

// restore accessibility to original state
#define F(type)		\
	if (have##type)	\
		SystemParametersInfo (SPI_SET##type, sizeof(type), &acc##type, 0);

static void RestoreAccessibility ()
{
	LIST
}
#undef F

#undef LIST

/*
=========================================================================

JOYSTICK

=========================================================================
*/

#define JOY_ABSOLUTE_AXIS	0x00000000		// control like a joystick
#define JOY_RELATIVE_AXIS	0x00000010		// control like a mouse, spinner, trackball

enum {
	JOY_AXIS_X, JOY_AXIS_Y, JOY_AXIS_Z, JOY_AXIS_R, JOY_AXIS_U, JOY_AXIS_V,
	JOY_MAX_AXES
};

#define F(name)		FIELD2OFS(JOYINFOEX,name)
static const byte joyOffsets[] = {
	F(dwXpos), F(dwYpos), F(dwZpos), F(dwRpos), F(dwUpos), F(dwVpos)
};
#undef F

enum {
	AxisNada = 0, AxisForward, AxisLook, AxisSide, AxisTurn, AxisUp
};

static DWORD	dwAxisFlags[JOY_MAX_AXES] =
{
	JOY_RETURNX, JOY_RETURNY, JOY_RETURNZ, JOY_RETURNR, JOY_RETURNU, JOY_RETURNV
};

static DWORD	dwAxisMap[JOY_MAX_AXES];
static DWORD	dwControlMap[JOY_MAX_AXES];

static bool		joy_avail, joy_advancedinit, joy_haspov;
static DWORD	joy_oldbuttonstate, joy_oldpovstate;

static int		joy_id;
static DWORD	joy_flags;
static DWORD	joy_numbuttons;

static JOYINFOEX ji;


// cvars
cvar_t	*in_joystick;	// non-static: used in menus

// none of these cvars are saved over a session
// this means that advanced controller configuration needs to be executed
// each time.  this avoids any problems with getting back to a default usage
// or when changing from one controller to another.  this way at least something
// works.
static cvar_t	*joy_name;
static cvar_t	*joy_advanced;
static cvar_t	*joy_advaxisx, *joy_advaxisy, *joy_advaxisz;
static cvar_t	*joy_advaxisr, *joy_advaxisu, *joy_advaxisv;
static cvar_t	*joy_forwardthreshold;
static cvar_t	*joy_sidethreshold;
static cvar_t	*joy_pitchthreshold;
static cvar_t	*joy_yawthreshold;
static cvar_t	*joy_forwardsensitivity;
static cvar_t	*joy_sidesensitivity;
static cvar_t	*joy_pitchsensitivity;
static cvar_t	*joy_yawsensitivity;
static cvar_t	*joy_upthreshold;
static cvar_t	*joy_upsensitivity;


static void StartupJoystick ()
{
	int			numdevs;
	JOYCAPS		jc;
	MMRESULT	mmr;

 	// assume no joystick
	joy_avail = false;

	// verify joystick driver is present
	if (!(numdevs = joyGetNumDevs ()))
	{
//		Com_Printf ("\njoystick not found -- driver not present\n\n");
		return;
	}

	// cycle through the joystick ids for the first valid one
	for (joy_id = JOYSTICKID1; joy_id < JOYSTICKID1+numdevs; joy_id++)
	{
		memset (&ji, 0, sizeof(ji));
		ji.dwSize = sizeof(ji);
		ji.dwFlags = JOY_RETURNCENTERED;

		mmr = joyGetPosEx (joy_id, &ji);
		if (mmr == JOYERR_NOERROR || mmr == JOYERR_PARMS) break;		// found a connected joystick
	}

	// MSDN Q133065
	if (mmr == JOYERR_PARMS)
	{
		Com_Printf ("No connected joysticks found\n");
		return;
	}

	if (mmr != JOYERR_NOERROR)
	{
		Com_Printf ("No valid joysticks (err=%X)\n", mmr);
		return;
	}

	// get the capabilities of the selected joystick
	// abort startup if command fails
	memset (&jc, 0, sizeof(jc));
	if ((mmr = joyGetDevCaps (joy_id, &jc, sizeof(jc))) != JOYERR_NOERROR)
	{
		Com_Printf ("Invalid joystick capabilities (err=%X)\n", mmr);
		return;
	}

	// save the joystick's number of buttons and POV status
	joy_numbuttons = jc.wNumButtons;
	joy_haspov = (jc.wCaps & JOYCAPS_HASPOV) != 0;

	// old button and POV states default to no buttons pressed
	joy_oldbuttonstate = joy_oldpovstate = 0;

	// mark the joystick as available and advanced initialization not completed
	// this is needed as cvars are not available during initialization

	joy_avail = true;
	joy_advancedinit = false;

	Com_Printf ("Joystick detected\n");
}


/*
===========
Joy_AdvancedUpdate_f
===========
*/
static void Joy_AdvancedUpdate_f ()
{

	// called once by IN_ReadJoystick and by user whenever an update is needed
	// cvars are now available
	int		i;

	// initialize all the maps
	for (i = 0; i < JOY_MAX_AXES; i++)
	{
		dwAxisMap[i] = AxisNada;
		dwControlMap[i] = JOY_ABSOLUTE_AXIS;
	}

	if (!joy_advanced->integer)
	{
		// default joystick initialization
		// 2 axes only with joystick control
		dwAxisMap[JOY_AXIS_X] = AxisTurn;
		// dwControlMap[JOY_AXIS_X] = JOY_ABSOLUTE_AXIS;
		dwAxisMap[JOY_AXIS_Y] = AxisForward;
		// dwControlMap[JOY_AXIS_Y] = JOY_ABSOLUTE_AXIS;
	}
	else
	{
		if (strcmp (joy_name->string, "joystick") != 0)
		{
			// notify user of advanced controller
			Com_Printf ("\n%s configured\n\n", joy_name->string);
		}

		// advanced initialization here
		// data supplied by user via joy_axisn cvars
		dwAxisMap[JOY_AXIS_X] =    joy_advaxisx->integer & 0xF;
		dwControlMap[JOY_AXIS_X] = joy_advaxisx->integer & JOY_RELATIVE_AXIS;
		dwAxisMap[JOY_AXIS_Y] =    joy_advaxisy->integer & 0xF;
		dwControlMap[JOY_AXIS_Y] = joy_advaxisy->integer & JOY_RELATIVE_AXIS;
		dwAxisMap[JOY_AXIS_Z] =    joy_advaxisz->integer & 0xF;
		dwControlMap[JOY_AXIS_Z] = joy_advaxisz->integer & JOY_RELATIVE_AXIS;
		dwAxisMap[JOY_AXIS_R] =    joy_advaxisr->integer & 0xF;
		dwControlMap[JOY_AXIS_R] = joy_advaxisr->integer & JOY_RELATIVE_AXIS;
		dwAxisMap[JOY_AXIS_U] =    joy_advaxisu->integer & 0xF;
		dwControlMap[JOY_AXIS_U] = joy_advaxisu->integer & JOY_RELATIVE_AXIS;
		dwAxisMap[JOY_AXIS_V] =    joy_advaxisv->integer & 0xF;
		dwControlMap[JOY_AXIS_V] = joy_advaxisv->integer & JOY_RELATIVE_AXIS;
	}

	// compute the axes to collect from DirectInput
	joy_flags = JOY_RETURNCENTERED | JOY_RETURNBUTTONS | JOY_RETURNPOV;
	for (i = 0; i < JOY_MAX_AXES; i++)
		if (dwAxisMap[i] != AxisNada)
			joy_flags |= dwAxisFlags[i];
}


/*
===========
IN_Commands
===========
*/
void IN_Commands ()
{
	int		i, key_index;
	DWORD	buttonstate, povstate;

	if (!joy_avail) return;

	// loop through the joystick buttons
	// key a joystick event or auxillary event for higher number buttons for each state change
	buttonstate = ji.dwButtons;
	for (i = 0; i < joy_numbuttons; i++)
	{
		if ( (buttonstate & (1<<i)) && !(joy_oldbuttonstate & (1<<i)) )
		{
			key_index = (i < 4) ? K_JOY1 : K_AUX1;
			Key_Event (key_index + i, true);
		}

		if ( !(buttonstate & (1<<i)) && (joy_oldbuttonstate & (1<<i)) )
		{
			key_index = (i < 4) ? K_JOY1 : K_AUX1;
			Key_Event (key_index + i, false);
		}
	}
	joy_oldbuttonstate = buttonstate;

	if (joy_haspov)
	{
		// convert POV information into 4 bits of state information
		// this avoids any potential problems related to moving from one
		// direction to another without going through the center position
		povstate = 0;
		if(ji.dwPOV != JOY_POVCENTERED)
		{
			if (ji.dwPOV == JOY_POVFORWARD)		povstate |= 0x01;
			if (ji.dwPOV == JOY_POVRIGHT)		povstate |= 0x02;
			if (ji.dwPOV == JOY_POVBACKWARD)	povstate |= 0x04;
			if (ji.dwPOV == JOY_POVLEFT)		povstate |= 0x08;
		}
		// determine which bits have changed and key an auxillary event for each change
		for (i=0 ; i < 4 ; i++)
		{
			if ((povstate & (1<<i)) && !(joy_oldpovstate & (1<<i)))
				Key_Event (K_AUX29 + i, true);

			if (!(povstate & (1<<i)) && (joy_oldpovstate & (1<<i)))
				Key_Event (K_AUX29 + i, false);
		}
		joy_oldpovstate = povstate;
	}
}


static bool ReadJoystick ()
{

	memset (&ji, 0, sizeof(ji));
	ji.dwSize = sizeof(ji);
	ji.dwFlags = joy_flags;

	if (joyGetPosEx (joy_id, &ji) == JOYERR_NOERROR)
		return true;
	else
	{
		// read error occurred
		// turning off the joystick seems too harsh for 1 read error,\
		// but what should be done?
		// Com_Printf ("IN_ReadJoystick: no response\n");
		// joy_avail = false;
		return false;
	}
}


static void JoyMove (usercmd_t *cmd)
{
	float	speed, aspeed;
	float	fAxisValue;

	// complete initialization if first time in
	// this is needed as cvars are not available at initialization time
	if (!joy_advancedinit)
	{
		Joy_AdvancedUpdate_f ();
		joy_advancedinit = true;
	}

	// verify joystick is available and that the user wants to use it
	if (!joy_avail || !in_joystick->integer)
		return;

	// collect the joystick data, if possible
	if (!ReadJoystick ()) return;

	if ((in_Speed.state & 1) ^ cl_run->integer)
		speed = 2;
	else
		speed = 1;
	aspeed = speed * cls.frametime;

	// loop through the axes
	for (int i = 0; i < JOY_MAX_AXES; i++)
	{
		// get the floating point zero-centered, potentially-inverted data for the current axis
		// move centerpoint to zero
		// convert range from -32768..32767 to -1..1
		fAxisValue = (OFS2FIELD(&ji, joyOffsets[i], unsigned) - 32768) / 32768.0f;

		switch (dwAxisMap[i])
		{
		case AxisForward:
			if ((!joy_advanced->integer) && mlooking)
			{
				// user wants forward control to become look control
				if (fabs(fAxisValue) > joy_pitchthreshold->value)
				{
					// if mouse invert is on, invert the joystick pitch value
					// only absolute control support here (joy_advanced is false)
					if (m_pitch->value < 0.0f)
						cl.viewangles[PITCH] -= (fAxisValue * joy_pitchsensitivity->value) * aspeed * cl_pitchspeed->value;
					else
						cl.viewangles[PITCH] += (fAxisValue * joy_pitchsensitivity->value) * aspeed * cl_pitchspeed->value;
				}
			}
			else
			{
				// user wants forward control to be forward control
				if (fabs(fAxisValue) > joy_forwardthreshold->value)
					cmd->forwardmove += appRound ((fAxisValue * joy_forwardsensitivity->value) * speed * cl_forwardspeed->value);
			}
			break;

		case AxisSide:
			if (fabs(fAxisValue) > joy_sidethreshold->value)
				cmd->sidemove += appRound ((fAxisValue * joy_sidesensitivity->value) * speed * cl_sidespeed->value);
			break;

		case AxisUp:
			if (fabs(fAxisValue) > joy_upthreshold->value)
				cmd->upmove += appRound ((fAxisValue * joy_upsensitivity->value) * speed * cl_upspeed->value);
			break;

		case AxisTurn:
			if ((in_Strafe.state & 1) || (lookstrafe->value && mlooking))
			{
				// user wants turn control to become side control
				if (fabs(fAxisValue) > joy_sidethreshold->value)
					cmd->sidemove -= appRound ((fAxisValue * joy_sidesensitivity->value) * speed * cl_sidespeed->value);
			}
			else
			{
				// user wants turn control to be turn control
				if (fabs(fAxisValue) > joy_yawthreshold->value)
				{
					if(dwControlMap[i] == JOY_ABSOLUTE_AXIS)
						cl.viewangles[YAW] += (fAxisValue * joy_yawsensitivity->value) * aspeed * cl_yawspeed->value;
					else
						cl.viewangles[YAW] += (fAxisValue * joy_yawsensitivity->value) * speed * 180.0f;

				}
			}
			break;

		case AxisLook:
			if (mlooking)
			{
				if (fabs(fAxisValue) > joy_pitchthreshold->value)
				{
					// pitch movement detected and pitch movement desired by user
					if(dwControlMap[i] == JOY_ABSOLUTE_AXIS)
						cl.viewangles[PITCH] += (fAxisValue * joy_pitchsensitivity->value) * aspeed * cl_pitchspeed->value;
					else
						cl.viewangles[PITCH] += (fAxisValue * joy_pitchsensitivity->value) * speed * 180.0f;
				}
			}
			break;

		default:
			break;
		}
	}
}


/*-----------------------------------------------------------------------------
	Initialization/finalization
-----------------------------------------------------------------------------*/

void IN_Init ()
{
CVAR_BEGIN(vars)
	// mouse variables
	CVAR_VAR(m_filter, 0, 0),
    CVAR_VAR(in_mouse, 2, CVAR_ARCHIVE),

	// joystick variables
	CVAR_VAR(in_joystick, 0, CVAR_ARCHIVE),
	CVAR_VAR(joy_name, joystick, 0),
	CVAR_VAR(joy_advanced, 0, 0),
	CVAR_VAR(joy_advaxisx, 0, 0),
	CVAR_VAR(joy_advaxisy, 0, 0),
	CVAR_VAR(joy_advaxisz, 0, 0),
	CVAR_VAR(joy_advaxisr, 0, 0),
	CVAR_VAR(joy_advaxisu, 0, 0),
	CVAR_VAR(joy_advaxisv, 0, 0),
	CVAR_VAR(joy_forwardthreshold, 0.15, 0),
	CVAR_VAR(joy_sidethreshold, 0.15, 0),
	CVAR_VAR(joy_upthreshold, 0.15, 0),
	CVAR_VAR(joy_pitchthreshold, 0.15, 0),
	CVAR_VAR(joy_yawthreshold, 0.15, 0),
	CVAR_VAR(joy_forwardsensitivity, -1, 0),
	CVAR_VAR(joy_sidesensitivity, -1, 0),
	CVAR_VAR(joy_upsensitivity, -1, 0),
	CVAR_VAR(joy_pitchsensitivity, 1, 0),
	CVAR_VAR(joy_yawsensitivity, -1, 0),
CVAR_END

	Cvar_GetVars (ARRAY_ARG(vars));

	RegisterCommand ("+mlook", MLookDown);
	RegisterCommand ("-mlook", MLookUp);

	RegisterCommand ("joy_advancedUpdate", Joy_AdvancedUpdate_f);

	DisableAccessibility ();
	StartupMouse ();
	StartupJoystick ();
}


void IN_Shutdown ()
{
	DeactivateMouse (true);
	RestoreAccessibility ();
}


void IN_Move (usercmd_t *cmd)
{
	MouseMove (cmd);
	if (ActiveApp) JoyMove (cmd);
}

/*
===========
IN_Activate

Called when the main window gains or loses focus.
The window may have been destroyed and recreated
between a deactivate and an activate.
===========
*/
static bool in_active = false;

void IN_Activate (bool active)
{
	in_active = active;
	/* NOTES:
	 * 1) when call IN_ActivateMouse()/IN_DeactivateMouse() directly from here, will appear mouse capture
	 *	bugs (should call IN_[De]activateMouse() from IN_Frame())
	 * 2) but, if we call this functions from here, require to check initialization of mouse cvars: this
	 *  function can be called after creation of main window from Vid_Init() before CL_Init() calls IN_Init()
	 */
}


/*
==================
IN_Frame

Called every frame, even if not generating commands
==================
*/
void IN_Frame ()
{
	guard(IN_Frame);

	if ((!cl.rendererReady || cls.key_dest == key_console || cls.key_dest == key_menu) && !FullscreenApp)
	{
		//?? deactivate for console only (when GUI mouse will be implemented)
		// temporarily deactivate if not in fullscreen
		DeactivateMouse (false);
		return;
	}

	if (!in_active)
	{
		if (mouseType) DeactivateMouse (false);
		return;
	}
	if (in_active && !mouseType)
	{
		in_needRestart = false;
		ActivateMouse ();
		return;
	}

	if (in_needRestart)
	{
		in_needRestart = false;
		DeactivateMouse (true);
		ActivateMouse ();
		return;
	}

	if (in_mouse->modified || !mouseType)
	{
		ActivateMouse ();
		return;
	}

	unguard;
}
