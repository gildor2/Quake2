//#define DIRECTINPUT_VERSION  0x0700
#define DIRECTINPUT_VERSION  0x0800

//#define INITGUID
#include "WinPrivate.h"
#include <mmsystem.h>		// for mmsystem joystick
#include "dinput.h"

// from cl_imput.cpp; temp!!
#define STATE_DOWN	1
//?? should remove all "cl.xxxxx" and usercmd_t stuff (should be pure input devices)


/*-----------------------------------------------------------------------------
	Some DirectInput GUIDs
	Placed here to reduce executable size (if create using #define INITGUID
	before #include "dinput.h", ALL DirectInput GUIDs will be created)
-----------------------------------------------------------------------------*/

// library used for c_dfDI* variables (DirectInput format)
#if DIRECTINPUT_VERSION < 0x0800
#pragma comment (lib, "dinput.lib")
#else
#pragma comment (lib, "dinput8.lib")
#endif

#ifndef INITGUID

#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
		EXTERN_C const GUID name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

#if DIRECTINPUT_VERSION >= 0x0800
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
 */

bool in_needRestart;


/*-----------------------------------------------------------------------------
	Mouse support
-----------------------------------------------------------------------------*/

// forwards
static void MouseEvent (unsigned buttons);

// buffered DirectInput mouse
#define BUFFERED_MOUSE		1

// mouse variables
static cvar_t	*in_mouse;
static cvar_t	*m_filter;
// non-static (used by menus)
cvar_t			*sensitivity;
cvar_t			*m_invert;

#define MOUSE_SCALE			0.022


/*-----------------------------------------------------------------------------
	DirectInput mouse
-----------------------------------------------------------------------------*/

static CDynamicLib libDI;

#if DIRECTINPUT_VERSION < 0x0800
static IDirectInput			*pDI;
static IDirectInputDevice	*pMouse;
#else
static IDirectInput8		*pDI;
static IDirectInputDevice8	*pMouse;
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
	libDI.Free ();
	ShowCursor (TRUE);
}


static bool DXMouse_Init ()
{
	appPrintf ("Initializing DirectInput\n");
#if DIRECTINPUT_VERSION < 0x0800
	typedef HRESULT (WINAPI * pDirectInputCreate_t) (HINSTANCE, DWORD, LPDIRECTINPUT *, LPUNKNOWN);
	static pDirectInputCreate_t pDirectInputCreate;
	if (!libDI)
	{
		if (!(libDI.Load ("dinput.dll")))
		{
			appWPrintf ("dinput.dll not found\n");
			return false;
		}
		if (!(libDI.GetProc ("DirectInputCreateA", &pDirectInputCreate)))
		{
			appWPrintf ("Wrong dinput.dll\n");
			DXMouse_Free ();
			return false;
		}
	}
	if FAILED(pDirectInputCreate (hInstance, DIRECTINPUT_VERSION, &pDI, NULL))
	{
		Com_DPrintf ("... cannot create DirectInput object\n");
		DXMouse_Free ();
		return false;
	}
#else
	typedef HRESULT (WINAPI * pDirectInput8Create_t) (HINSTANCE, DWORD, REFIID, LPVOID *, LPUNKNOWN);
	static pDirectInput8Create_t pDirectInput8Create;
	if (!libDI)
	{
		if (!(libDI.Load ("dinput8.dll")))
		{
			appWPrintf ("dinput8.dll not found\n");
			return false;
		}
		if (!(libDI.GetProc ("DirectInput8Create", &pDirectInput8Create)))
		{
			appWPrintf ("Wrong dinput8.dll\n");
			DXMouse_Free ();
			return false;
		}
	}
	if FAILED(pDirectInput8Create (hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&pDI, NULL))
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
		appPrintf ("... cannot set data format\n");
		DXMouse_Free ();
		return false;
	}
	if FAILED(pMouse->SetCooperativeLevel (cl_hwnd, DISCL_EXCLUSIVE|DISCL_FOREGROUND))
	{
		//?? set fullscreen to 0 -- if cannot capture mouse in fullscreen mode
		//?? (if fullscreen is already 0, it will not be changed)
		appPrintf ("... cannot set cooperative level\n");
		DXMouse_Free ();
		return false;
	}
#if BUFFERED_MOUSE
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
		appPrintf ("... cannot set mouse buffered mode\n");
		DXMouse_Free ();
		return false;
	}
#endif
#if 0
	if FAILED(pMouse->Acquire ())
	{
		//?? fullscreen <- 0
		appPrintf ("... cannot acquire mouse\n");
		//?? do not Free() -- just keep mouse unacquired; when window will be activated -- acquire mouse
		IN_DXMouse_Free ();
		return false;
	}
#endif

	ShowCursor (FALSE);
	SetCursor (NULL);		// DirectX manual says: "cursor will disappear"; but, in some circusmances it appears ...
	return true;
}


static void DXMouse_Frame (int &move_x, int &move_y)
{
	// poll DirectInput mouse
#if BUFFERED_MOUSE
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
			appWPrintf ("Error %X on DI mouse.GetData()\n", (unsigned)res);
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
		appWPrintf ("Error %X on DI mouse.GetState()\n", res);
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
		if (wParam >> 31)	// sign of HIWORD(wParam)
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
	// catch all mouse state messages
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
	case WM_MOUSEMOVE:
		{
			int temp = 0;
			if (wParam & MK_LBUTTON)  temp |= 1;
			if (wParam & MK_RBUTTON)  temp |= 2;
			if (wParam & MK_MBUTTON)  temp |= 4;
			if (wParam & MK_XBUTTON1) temp |= 8;
			if (wParam & MK_XBUTTON2) temp |= 16;
			MouseEvent (temp);
		}
		break;
	}

	return false;
}


static void WinMouse_Init ()
{
	appPrintf ("Initializing Win32 mouse support\n");
	AddMsgHook (MouseMsgHook);

	static int mouseParms[3] = {0, 0, 0};
	static int mouseSpeed = 10;
	if (haveSpiMouse)
		SystemParametersInfo (SPI_SETMOUSE, 0, mouseParms, 0);
	if (haveSpiMouseSpeed)
		SystemParametersInfo (SPI_SETMOUSESPEED, 0, &mouseSpeed, 0);

	int width  = GetSystemMetrics (SM_CXSCREEN);
	int height = GetSystemMetrics (SM_CYSCREEN);

	RECT window_rect;
	GetWindowRect (cl_hwnd, &window_rect);
	if (window_rect.left < 0)			window_rect.left   = 0;
	if (window_rect.top < 0)		 	window_rect.top    = 0;
	if (window_rect.right >= width)		window_rect.right  = width-1;
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
	haveSpiMouse      = SystemParametersInfo (SPI_GETMOUSE, 0, originalMouseParms, 0) != 0;
	haveSpiMouseSpeed = SystemParametersInfo (SPI_GETMOUSESPEED, 0, &originalMouseSpeed, 0) != 0;	// Win98+, Win2K+
}


static void MouseEvent (unsigned buttons)
{
	static unsigned oldButtons = 0;
	unsigned old = oldButtons;
	oldButtons = buttons;

	// perform button actions
	int key = K_MOUSE1;
	while (buttons || old)
	{
		if ((buttons ^ old) & 1)
			Key_Event (key, buttons & 1);

		buttons >>= 1;
		old     >>= 1;
		key++;
	}
}


static void MouseMove (usercmd_t *cmd)
{
	static int old_mouse_x, old_mouse_y;

	if (!mouseType)
		return;

	int mx, my;
	if (mouseType == 2)
		DXMouse_Frame (mx, my);
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

	// filter mouse movement
	if (m_filter->integer)	//?? change function
	{
		mx = (mx + old_mouse_x) / 2;
		my = (my + old_mouse_y) / 2;
	}

	old_mouse_x = mx;
	old_mouse_y = my;

	mx = appRound (mx * sensitivity->value * 1.5f);
	my = appRound (my * sensitivity->value * 1.5f);
	if (m_invert->integer) my = -my;

	// add mouse X/Y movement to cmd
	cl.viewangles[YAW]   -= mx * MOUSE_SCALE;
	cl.viewangles[PITCH] += my * MOUSE_SCALE;
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


/*-----------------------------------------------------------------------------
	Joystick support via MMSYSTEM
-----------------------------------------------------------------------------*/

static bool		joy_avail;

// NOTE: this consts used as cvar values! Used for turn/look axes only, when advanced configuration active
#define JOY_ABSOLUTE_AXIS	0x00000000			// control like a joystick
#define JOY_RELATIVE_AXIS	0x00000010			// control like a mouse, spinner, trackball

enum {
	JOY_AXIS_X, JOY_AXIS_Y, JOY_AXIS_Z, JOY_AXIS_R, JOY_AXIS_U, JOY_AXIS_V,
	JOY_MAX_AXES
};
// fiends in JOYINFOEX structure, corresponding to axis positions
#define F(name)		FIELD2OFS(JOYINFOEX,name)
static const byte joyOffsets[JOY_MAX_AXES] = {
	F(dwXpos), F(dwYpos), F(dwZpos), F(dwRpos), F(dwUpos), F(dwVpos)
};
#undef F

// axis map; values used as integers in cvars configuration
// using #define instead of enum{} for cvar initialization ...
#define AxisNone		0
#define AxisForward		1
#define AxisLook		2
#define AxisSide		3
#define AxisTurn		4
#define AxisUp			5

static int		joy_id;
// joystick caps
static DWORD	joy_numButtons;
static bool		joy_hasPov;


// cvars
cvar_t	*in_joystick;	// non-static: used in menus

// None of these cvars are saved over a session; this means that advanced controller configuration needs
// to be executed each time. This avoids any problems with getting back to a default usage or when changing
// from one controller to another. This way at least something works.
static cvar_t	*joy_threshold;					// 1 cvar for all axes; original q2 have 1 cvar per axis
// axis map
static cvar_t	*joy_mapVars[6];
// sensitivity
static cvar_t	*joy_sensVars[5];


static void StartupJoystick ()
{
	joy_avail = false;

	// verify joystick driver is present
	int numdevs;
	if (!(numdevs = joyGetNumDevs ()))
	{
//		appPrintf ("\njoystick not found -- driver not present\n\n");
		return;
	}

	// cycle through the joystick ids for the first valid one
	MMRESULT mmr;
	for (joy_id = JOYSTICKID1; joy_id < JOYSTICKID1+numdevs; joy_id++)
	{
		JOYINFOEX ji;
		memset (&ji, 0, sizeof(ji));
		ji.dwSize  = sizeof(ji);
		ji.dwFlags = JOY_RETURNCENTERED;

		mmr = joyGetPosEx (joy_id, &ji);
		if (mmr == JOYERR_NOERROR || mmr == JOYERR_PARMS) break;		// found a connected joystick
	}

	// MSDN Q133065
	if (mmr == JOYERR_PARMS)
	{
		appPrintf ("No connected joysticks found\n");
		return;
	}

	if (mmr != JOYERR_NOERROR)
	{
		appPrintf ("No valid joysticks; err=%X\n", mmr);
		return;
	}

	// get the capabilities of the selected joystick
	JOYCAPS jc;
	memset (&jc, 0, sizeof(jc));
	if ((mmr = joyGetDevCaps (joy_id, &jc, sizeof(jc))) != JOYERR_NOERROR)
	{
		appPrintf ("Invalid joystick capabilities (err=%X)\n", mmr);
		return;
	}

	// save the joystick's number of buttons and POV status
	joy_numButtons = jc.wNumButtons;
//	joy_numAxes    = jc.wNumAxes;
	joy_hasPov     = (jc.wCaps & JOYCAPS_HASPOV) != 0;

	joy_avail = true;
	appPrintf ("Joystick detected\n");
}


static void JoyMove (usercmd_t *cmd)
{
	if (!joy_avail || !in_joystick->integer)
		return;

	int i;
	static DWORD oldbuttonstate = 0, oldpovstate = 0;

	// acquire joystick data
	JOYINFOEX ji;
	memset (&ji, 0, sizeof(ji));
	ji.dwSize  = sizeof(ji);
	ji.dwFlags = JOY_RETURNALL;
	if (joyGetPosEx (joy_id, &ji) != JOYERR_NOERROR) return;

	//--------- loop through the buttons -----------------------
	// key a joystick event or auxillary event for higher number buttons for each state change
	DWORD buttonstate = ji.dwButtons;
	for (i = 0; i < joy_numButtons; i++)
	{
		int oldState = (oldbuttonstate >> i) & 1;
		int newState = (buttonstate >> i) & 1;
		if (oldState != newState)
			Key_Event (K_JOY1 + i, newState != 0);	// for button index >= 4 used K_AUXn (n = button-4)
	}
	oldbuttonstate = buttonstate;

	if (joy_hasPov)
	{
		// POV movement -> K_AUX29..K_AUX32
		DWORD povstate = 0;
		if (ji.dwPOV != JOY_POVCENTERED)
		{
			if (ji.dwPOV == JOY_POVFORWARD)		povstate |= 1;
			if (ji.dwPOV == JOY_POVRIGHT)		povstate |= 2;
			if (ji.dwPOV == JOY_POVBACKWARD)	povstate |= 4;
			if (ji.dwPOV == JOY_POVLEFT)		povstate |= 8;
		}
		// determine which bits have changed and key an auxillary event for each change
		for (i = 0; i < 4 ; i++)
		{
			int oldState = (oldpovstate >> i) & 1;
			int newState = (povstate >> i) & 1;
			if (oldState != newState)
				Key_Event (K_AUX29 + i, newState != 0);
		}
		oldpovstate = povstate;
	}

	//------------- loop through the axes ----------------------
	for (i = 0; i < JOY_MAX_AXES; i++)
	{
		int  axis      =  joy_mapVars[i]->integer & 0x0F;
		bool isAbsAxis = (joy_mapVars[i]->integer & JOY_RELATIVE_AXIS) == 0;
		// get the floating point zero-centered, potentially-inverted data for the current axis
		// move centerpoint to zero
		// convert range from -32768..32767 to -1..1
		float fAxisValue = (OFS2FIELD(&ji, joyOffsets[i], unsigned) - 32768) / 32768.0f;
		// check sensitivity threshold
		if (fabs (fAxisValue) < joy_threshold->value) continue;
		// "speed" key is down
		if ((in_Speed.state & STATE_DOWN) ^ cl_run->integer)
			fAxisValue *= 2;
		// apply sensitivity
		fAxisValue *= joy_sensVars[axis]->value;

		switch (axis)
		{
		// case AxisNone == 0 -- do nothing
		case AxisForward:
			cmd->forwardmove += appRound (cl_forwardspeed->value * fAxisValue);
			break;

		case AxisSide:
			cmd->sidemove    += appRound (cl_sidespeed->value * fAxisValue);
			break;

		case AxisUp:
			cmd->upmove      += appRound (cl_upspeed->value * fAxisValue);
			break;

		case AxisTurn:
			{
				if (isAbsAxis)
					fAxisValue *= cl_yawspeed->value * cls.frametime;
				else
					fAxisValue *= 180.0f;
				cl.viewangles[YAW] += fAxisValue;
			}
			break;

		case AxisLook:
			{
				if (isAbsAxis)
					fAxisValue *= cl_pitchspeed->value * cls.frametime;
				else
					fAxisValue *= 180.0f;
				// take into account inverted mouse
				if (m_invert->integer)
					cl.viewangles[PITCH] -= fAxisValue;
				else
					cl.viewangles[PITCH] += fAxisValue;
			}
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
	//--------- mouse variables ----------
	//?? move other mouse cvars here
	CVAR_VAR(in_mouse, 2, CVAR_ARCHIVE),
	CVAR_VAR(m_filter, 0, 0),
	CVAR_VAR(sensitivity, 3, CVAR_ARCHIVE),
	CVAR_VAR(m_invert, 0, CVAR_ARCHIVE),

	//-------- joystick variables --------
	CVAR_VAR(in_joystick, 0, CVAR_ARCHIVE),
	CVAR_VAR(joy_threshold, 0.15, 0),
	// axis map
	CVAR_FULL(&joy_mapVars[JOY_AXIS_X],   "joy_advAxisX", STR(AxisTurn), 0),
	CVAR_FULL(&joy_mapVars[JOY_AXIS_Y],   "joy_advAxisY", STR(AxisLook), 0),
	CVAR_FULL(&joy_mapVars[JOY_AXIS_Z],   "joy_advAxisZ", "0", 0),
	CVAR_FULL(&joy_mapVars[JOY_AXIS_R],   "joy_advAxisR", "0", 0),
	CVAR_FULL(&joy_mapVars[JOY_AXIS_U],   "joy_advAxisU", "0", 0),
	CVAR_FULL(&joy_mapVars[JOY_AXIS_V],   "joy_advAxisV", "0", 0),
	// sensitivity
	CVAR_FULL(&joy_sensVars[AxisForward], "joy_forwardSensitivity", "-1", 0),
	CVAR_FULL(&joy_sensVars[AxisLook],    "joy_pitchSensitivity",    "1", 0),
	CVAR_FULL(&joy_sensVars[AxisSide],    "joy_sideSensitivity",    "-1", 0),
	CVAR_FULL(&joy_sensVars[AxisTurn],    "joy_yawSensitivity",     "-1", 0),
	CVAR_FULL(&joy_sensVars[AxisUp],      "joy_upSensitivity",      "-1", 0)
CVAR_END

	Cvar_GetVars (ARRAY_ARG(vars));

	DisableAccessibility ();
	StartupMouse ();
	StartupJoystick ();
}


void IN_Shutdown ()
{
	DeactivateMouse (true);
	RestoreAccessibility ();
}


static bool in_active = false;

// Called when the main window gains or loses focus. The window may have been
// destroyed and recreated between a deactivate and an activate.
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


// Called every frame, even if not generating commands
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

	if (!mouseType)
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

void IN_Move (usercmd_t *cmd)
{
	MouseMove (cmd);
	if (ActiveApp) JoyMove (cmd);
}
