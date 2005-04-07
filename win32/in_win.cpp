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

#define INITGUID
#include "winquake.h"
#include <dinput.h>

#include "../client/client.h"


/*!! Add DirectInput keyboard support, remove win32 mouse/keyboard support; joystick - DirectInput only;
 *   After this, can change "in_mouse==0|1|2" to "nomouse" cvar + can remove mouse/keyboard code from MainWndProc()
 */

extern	unsigned	sys_msg_time;

bool in_needRestart;


/*
============================================================

  MOUSE CONTROL

============================================================
*/

#define NUM_MOUSE_BUTTONS	3

// mouse variables
static cvar_t	*in_mouse;
static cvar_t	*m_filter;


static bool mlooking;

static void IN_MLookDown (void)
{
	mlooking = true;
}

static void IN_MLookUp (void)
{
	mlooking = false;
	if (!freelook->integer && lookspring->integer)
		IN_CenterView ();
}

static int	mouse_oldbuttonstate;
static POINT current_pos;
static int	mouse_x, mouse_y, old_mouse_x, old_mouse_y, mx_accum, my_accum;

static int	old_x, old_y;

static int	window_center_x, window_center_y;
static RECT	window_rect;


static int move_x, move_y;

//----------------- DirectInput mouse -----------------------

typedef HRESULT (WINAPI * pDirectInputCreate_t) (HINSTANCE, DWORD, LPDIRECTINPUT *, LPUNKNOWN);
static pDirectInputCreate_t pDirectInputCreate;

static HINSTANCE hInstDI;

static IDirectInput *pDI;
static IDirectInputDevice *pDID;
static int mouseType;		// copy of in_mouse


static void IN_FreeDirect (void)
{
	Com_DPrintf ("Shutdown DirectInput mouse\n");
	if (pDI)
	{
		if (pDID)
		{
			pDID->Unacquire ();
			pDID->Release ();
			pDID = NULL;
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


static bool IN_InitDirect (void)
{
	Com_Printf ("Initializing DirectInput\n");
	if (!hInstDI)
	{
		if (!(hInstDI = LoadLibrary ("dinput.dll")))
		{
			Com_WPrintf ("... loading dinput.dll failed\n");
			return false;
		}
		if (!(pDirectInputCreate = (pDirectInputCreate_t) GetProcAddress (hInstDI, "DirectInputCreateA")))
		{
			Com_WPrintf ("... couldn't get DI proc address\n");
			return false;
		}
	}

	if FAILED(pDirectInputCreate (global_hInstance, DIRECTINPUT_VERSION, &pDI, NULL))
	{
		Com_DPrintf ("... cannot create DirectInput object\n");
		IN_FreeDirect ();
		return false;
	}

	if FAILED(pDI->CreateDevice (GUID_SysMouse, &pDID, NULL))
	{
		Com_DPrintf ("... cannot create DirectInput mouse\n");
		IN_FreeDirect ();
		return false;
	}

	pDID->SetDataFormat (&c_dfDIMouse);	// may fail
	if FAILED(pDID->SetCooperativeLevel (cl_hwnd, DISCL_EXCLUSIVE|DISCL_FOREGROUND))
	{
		//?? set fullscreen to 0 -- if cannot capture mouse in fullscreen mode
		//?? (if fullscreen is already 0, it will not be changed)
		Com_Printf ("... cannot set cooperative level\n");
		IN_FreeDirect ();
		return false;
	}
	if FAILED(pDID->Acquire ())
	{
		//?? fullscreen <- 0
		Com_Printf ("... cannot acquire mouse\n");
		IN_FreeDirect ();
		return false;
	}

	old_x = old_y = 0;

	ShowCursor (FALSE);
	SetCursor (NULL);		// DirectX manual says: "cursor will disappear"; but, in some circusmances it appears ...
	return true;
}


static void IN_DirectMouseFrame (void)
{
	DIMOUSESTATE ms;

	// poll DirectInput mouse
	HRESULT hresult = pDID->GetDeviceState (sizeof(ms), &ms);
	if (hresult == DIERR_INPUTLOST)
	{
		// try to acquire mouse
		pDID->Acquire ();
		return;
	}
	if FAILED(hresult)
	{
		Com_WPrintf ("error on DI mouse GetState()\n");
		return;
	}
	// process mouse wheel
	if (ms.lZ)
	{
		int msg = (ms.lZ > 0) ? K_MWHEELUP : K_MWHEELDOWN;
		Key_Event (msg, true, sys_msg_time);	//!! check for correct sys_msg_time
		Key_Event (msg, false, sys_msg_time);
	}
	// process X and Y axes
	move_x = ms.lX;
	move_y = ms.lY;
	// process buttons
	int tmp = 0;
	if (ms.rgbButtons[0]) tmp |= 1;
	if (ms.rgbButtons[1]) tmp |= 2;
	if (ms.rgbButtons[2]) tmp |= 4;
	IN_MouseEvent (tmp);			// call this always - to detect button on/off
	//?? can access 4 (and more?) buttons
}


//--------------- Standard Win32 mouse ----------------------

static bool	haveSpiMouse, haveSpiMouseSpeed;
static int	originalMouseParms[3], originalMouseSpeed;


static void IN_InitWin32 (void)
{
	static int mouseParms[3] = {0, 0, 0};
	static int mouseSpeed = 10;
	int		width, height;

	Com_Printf ("Initializing Win32 mouse support\n");

	if (haveSpiMouse)
		SystemParametersInfo (SPI_SETMOUSE, 0, mouseParms, 0);
	if (haveSpiMouseSpeed)
		SystemParametersInfo (SPI_SETMOUSESPEED, 0, &mouseSpeed, 0);

	width = GetSystemMetrics (SM_CXSCREEN);
	height = GetSystemMetrics (SM_CYSCREEN);

	GetWindowRect (cl_hwnd, &window_rect);
	if (window_rect.left < 0)	window_rect.left = 0;
	if (window_rect.top < 0) 	window_rect.top = 0;
	if (window_rect.right >= width)		window_rect.right = width-1;
	if (window_rect.bottom >= height-1)	window_rect.bottom = height-1;

	window_center_x = (window_rect.right + window_rect.left) / 2;
	window_center_y = (window_rect.top + window_rect.bottom) / 2;

	SetCursorPos (window_center_x, window_center_y);

	old_x = window_center_x;
	old_y = window_center_y;

	SetCapture (cl_hwnd);
	ClipCursor (&window_rect);

	SetCursor (NULL);
	ShowCursor (FALSE);
}


static void IN_FreeWin32 (void)
{
	Com_DPrintf ("Shutdown Win32 mouse\n");
	if (haveSpiMouse)
		SystemParametersInfo (SPI_SETMOUSE, 0, originalMouseParms, 0);
	if (haveSpiMouseSpeed)
		SystemParametersInfo (SPI_SETMOUSESPEED, 0, &originalMouseSpeed, 0);

	ClipCursor (NULL);
	ReleaseCapture ();
	ShowCursor (TRUE);
}


//------------- Common mouse support ----------------------

/*
===========
IN_DeactivateMouse

Called when the window loses focus
===========
*/
static void IN_DeactivateMouse (void)
{
	if (mouseType == 2)
		IN_FreeDirect ();
	else if (mouseType == 1)
		IN_FreeWin32 ();

	mouseType = 0;
}


/*
===========
IN_ActivateMouse

Called when the window gains focus or changes in some way
===========
*/

static void IN_ActivateMouse (void)
{
	if (mouseType)
	{
		if (!in_mouse->modified)
			return;
		Com_DPrintf ("Changing mouse type\n");
		// deactivate then reactivate mouse
		IN_DeactivateMouse ();
	}

	if (in_mouse->integer == 2 && IN_InitDirect ())
		mouseType = 2;
	else if (in_mouse->integer)
	{
		IN_InitWin32 ();
		mouseType = 1;
	}
	else
		mouseType = 0;

	in_mouse->modified = false;
}


/*
===========
IN_StartupMouse
===========
*/

static void IN_StartupMouse (void)
{
	haveSpiMouse = SystemParametersInfo (SPI_GETMOUSE, 0, originalMouseParms, 0) != 0;
	haveSpiMouseSpeed = SystemParametersInfo (SPI_GETMOUSESPEED, 0, &originalMouseSpeed, 0) != 0;	// Win98+, Win2K+
}

/*
===========
IN_MouseEvent
===========
*/
void IN_MouseEvent (int mstate)
{
	// perform button actions
	for (int i = 0; i < NUM_MOUSE_BUTTONS; i++)
	{
		if ((mstate & (1<<i)) && !(mouse_oldbuttonstate & (1<<i)))
			Key_Event (K_MOUSE1 + i, true, sys_msg_time);

		if (!(mstate & (1<<i)) && (mouse_oldbuttonstate & (1<<i)))
			Key_Event (K_MOUSE1 + i, false, sys_msg_time);
	}

	mouse_oldbuttonstate = mstate;
}


/*
===========
IN_MouseMove
===========
*/

static void IN_MouseMove (usercmd_t *cmd)
{
	int		mx, my;

	if (!mouseType)
		return;

	if (mouseType == 2)
	{
		mx = move_x;
		my = move_y;
	}
	else
	{
		// get mouse position
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


/*
===================
IN_ClearStates
===================
*/
static void IN_ClearStates (void)
{
	mx_accum = 0;
	my_accum = 0;
	mouse_oldbuttonstate = 0;
}


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

enum {
	AxisNada = 0, AxisForward, AxisLook, AxisSide, AxisTurn, AxisUp
};

static DWORD	dwAxisFlags[JOY_MAX_AXES] =
{
	JOY_RETURNX, JOY_RETURNY, JOY_RETURNZ, JOY_RETURNR, JOY_RETURNU, JOY_RETURNV
};

static DWORD	dwAxisMap[JOY_MAX_AXES];
static DWORD	dwControlMap[JOY_MAX_AXES];
static PDWORD	pdwRawValue[JOY_MAX_AXES];

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


/*
===============
IN_StartupJoystick
===============
*/
static void IN_StartupJoystick (void)
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
RawValuePointer
===========
*/
static PDWORD RawValuePointer (int axis)
{
	switch (axis)
	{
	case JOY_AXIS_X:
		return &ji.dwXpos;
	case JOY_AXIS_Y:
		return &ji.dwYpos;
	case JOY_AXIS_Z:
		return &ji.dwZpos;
	case JOY_AXIS_R:
		return &ji.dwRpos;
	case JOY_AXIS_U:
		return &ji.dwUpos;
	case JOY_AXIS_V:
		return &ji.dwVpos;
	}
	return NULL;	// make compiler happy
}


/*
===========
Joy_AdvancedUpdate_f
===========
*/
static void Joy_AdvancedUpdate_f (void)
{

	// called once by IN_ReadJoystick and by user whenever an update is needed
	// cvars are now available
	int		i;

	// initialize all the maps
	for (i = 0; i < JOY_MAX_AXES; i++)
	{
		dwAxisMap[i] = AxisNada;
		dwControlMap[i] = JOY_ABSOLUTE_AXIS;
		pdwRawValue[i] = RawValuePointer(i);
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
void IN_Commands (void)
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
			Key_Event (key_index + i, true, 0);
		}

		if ( !(buttonstate & (1<<i)) && (joy_oldbuttonstate & (1<<i)) )
		{
			key_index = (i < 4) ? K_JOY1 : K_AUX1;
			Key_Event (key_index + i, false, 0);
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
				Key_Event (K_AUX29 + i, true, 0);

			if (!(povstate & (1<<i)) && (joy_oldpovstate & (1<<i)))
				Key_Event (K_AUX29 + i, false, 0);
		}
		joy_oldpovstate = povstate;
	}
}


/*
===============
IN_ReadJoystick
===============
*/
static bool IN_ReadJoystick (void)
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


/*
===========
IN_JoyMove
===========
*/
static void IN_JoyMove (usercmd_t *cmd)
{
	float	speed, aspeed;
	float	fAxisValue;

	// complete initialization if first time in
	// this is needed as cvars are not available at initialization time
	if( joy_advancedinit != true )
	{
		Joy_AdvancedUpdate_f ();
		joy_advancedinit = true;
	}

	// verify joystick is available and that the user wants to use it
	if (!joy_avail || !in_joystick->integer)
		return;

	// collect the joystick data, if possible
	if (IN_ReadJoystick () != true)
		return;

	if ((in_Speed.state & 1) ^ cl_run->integer)
		speed = 2;
	else
		speed = 1;
	aspeed = speed * cls.frametime;

	// loop through the axes
	for (int i = 0; i < JOY_MAX_AXES; i++)
	{
		// get the floating point zero-centered, potentially-inverted data for the current axis
		fAxisValue = (float) *pdwRawValue[i];
		// move centerpoint to zero
		fAxisValue -= 32768.0f;
		// convert range from -32768..32767 to -1..1
		fAxisValue /= 32768.0f;

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


void IN_Init (void)
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

	RegisterCommand ("+mlook", IN_MLookDown);
	RegisterCommand ("-mlook", IN_MLookUp);

	RegisterCommand ("joy_advancedUpdate", Joy_AdvancedUpdate_f);

	IN_StartupMouse ();
	IN_StartupJoystick ();
}


void IN_Shutdown (void)
{
	IN_DeactivateMouse ();
}


void IN_Move (usercmd_t *cmd)
{
	IN_MouseMove (cmd);
	if (ActiveApp) IN_JoyMove (cmd);
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
void IN_Frame (void)
{
	guard(IN_Frame);

	if ((!cl.refresh_prepped || cls.key_dest == key_console || cls.key_dest == key_menu) && !FullscreenApp)
	{
		// temporarily deactivate if not in fullscreen
		IN_DeactivateMouse ();
		return;
	}

	if (!in_active)
	{
		if (mouseType) IN_DeactivateMouse ();
		return;
	}
	if (in_active && !mouseType)
	{
		in_needRestart = false;
		IN_ActivateMouse ();
		return;
	}

	if (in_needRestart)
	{
		in_needRestart = false;
		IN_DeactivateMouse ();
		IN_ActivateMouse ();
		return;
	}

	if (in_mouse->modified || !mouseType)
	{
		IN_ActivateMouse ();
		return;
	}

	// DirectInput mouse frame
	if (mouseType == 2)
		IN_DirectMouseFrame ();

	unguard;
}
