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
// in_win.c -- windows 95 mouse and joystick code
// 02/21/97 JCB Added extended DirectInput code to support external controllers.

#define INITGUID
#include "winquake.h"
#include <dinput.h>

#include "../client/client.h"


extern	unsigned	sys_msg_time;

// joystick defines and variables
// where should defines be moved?
#define JOY_ABSOLUTE_AXIS	0x00000000		// control like a joystick
#define JOY_RELATIVE_AXIS	0x00000010		// control like a mouse, spinner, trackball

enum _JoyAxes
{
	JOY_AXIS_X, JOY_AXIS_Y, JOY_AXIS_Z, JOY_AXIS_R, JOY_AXIS_U, JOY_AXIS_V,
	JOY_MAX_AXES
};

enum _ControlList
{
	AxisNada = 0, AxisForward, AxisLook, AxisSide, AxisTurn, AxisUp
};

static DWORD	dwAxisFlags[JOY_MAX_AXES] =
{
	JOY_RETURNX, JOY_RETURNY, JOY_RETURNZ, JOY_RETURNR, JOY_RETURNU, JOY_RETURNV
};

static DWORD	dwAxisMap[JOY_MAX_AXES];
static DWORD	dwControlMap[JOY_MAX_AXES];
static PDWORD	pdwRawValue[JOY_MAX_AXES];

static cvar_t *in_mouse;
qboolean in_needRestart;

cvar_t	*in_joystick;

// none of these cvars are saved over a session
// this means that advanced controller configuration needs to be executed
// each time.  this avoids any problems with getting back to a default usage
// or when changing from one controller to another.  this way at least something
// works.
static cvar_t	*joy_name;
static cvar_t	*joy_advanced;
static cvar_t	*joy_advaxisx;
static cvar_t	*joy_advaxisy;
static cvar_t	*joy_advaxisz;
static cvar_t	*joy_advaxisr;
static cvar_t	*joy_advaxisu;
static cvar_t	*joy_advaxisv;
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

static qboolean	joy_avail, joy_advancedinit, joy_haspov;
static DWORD	joy_oldbuttonstate, joy_oldpovstate;

static int		joy_id;
static DWORD	joy_flags;
static DWORD	joy_numbuttons;

static JOYINFOEX ji;

static qboolean	in_appactive;
static qboolean	mouseinitialized;


// forward-referenced functions
void IN_StartupJoystick (void);
void Joy_AdvancedUpdate_f (void);
void IN_JoyMove (usercmd_t *cmd);

/*
============================================================

  MOUSE CONTROL

============================================================
*/

// mouse variables
static cvar_t	*m_filter;

static qboolean	mlooking;

void IN_MLookDown (void)
{
	mlooking = true;
}

void IN_MLookUp (void)
{
	mlooking = false;
	if (!freelook->integer && lookspring->integer)
		IN_CenterView ();
}

static int	mouse_buttons;
static int	mouse_oldbuttonstate;
static POINT current_pos;
static int	mouse_x, mouse_y, old_mouse_x, old_mouse_y, mx_accum, my_accum;

static int	old_x, old_y;

int		window_center_x, window_center_y;
RECT	window_rect;


//-------------------- DirectInput --------------------------

static HRESULT (WINAPI *pDirectInputCreate)(HINSTANCE hinst, DWORD dwVersion, LPDIRECTINPUT * lplpDirectInput, LPUNKNOWN punkOuter);
static HINSTANCE hInstDI;

static LPDIRECTINPUT pDI;
static LPDIRECTINPUTDEVICE pDID;
static int mouseType;		// copy of in_mouse


static void IN_FreeDirect (void)
{
	Com_DPrintf ("Shutdown DirectInput mouse\n");
	if (pDI)
	{
		if (pDID)
		{
			IDirectInputDevice_Unacquire (pDID);
			IDirectInputDevice_Release (pDID);
			pDID = NULL;
		}
		IDirectInput_Release (pDI);
		pDI = NULL;
	}
	if (hInstDI)
	{
		FreeLibrary (hInstDI);
		hInstDI = NULL;
	}
	ShowCursor (TRUE);
}


static qboolean IN_InitDirect (void)
{
	Com_Printf ("Initializing DirectInput\n");
	if (!hInstDI)
	{
		Com_DPrintf ("...loading dinput.dll\n");
		hInstDI = LoadLibrary ("dinput.dll");
		if (!hInstDI)
		{
			Com_Printf ("failed\n");
			return false;
		}
		Com_Printf ("ok\n");
		pDirectInputCreate = (void *)GetProcAddress (hInstDI, "DirectInputCreateA");
		if (!pDirectInputCreate)
		{
			Com_WPrintf ("*** couldn't get DI proc addr ***\n");
			return false;
		}
	}

	Com_DPrintf ("...creating DirectInput object: ");
	if FAILED(pDirectInputCreate (global_hInstance, DIRECTINPUT_VERSION, &pDI, NULL))
	{
		Com_DPrintf ("failed\n");
		IN_FreeDirect ();
		return false;
	}

	Com_DPrintf ("ok\n...creating DirectInput mouse: ");
	if FAILED(IDirectInput_CreateDevice (pDI, (void*)&GUID_SysMouse, &pDID, NULL))
	{
		Com_DPrintf ("failed\n");
		IN_FreeDirect ();
		return false;
	}
	Com_DPrintf ("ok\n");
	IDirectInputDevice_SetDataFormat (pDID, &c_dfDIMouse);	// may fail
	if FAILED(IDirectInputDevice_SetCooperativeLevel (pDID, cl_hwnd, DISCL_EXCLUSIVE|DISCL_FOREGROUND))
	{
		Com_Printf ("Cannot set cooperative level\n");
		IN_FreeDirect ();
		return false;
	}
	if FAILED(IDirectInputDevice_Acquire (pDID))
	{
		Com_Printf ("Cannot acquire mouse\n");
		IN_FreeDirect ();
		return false;
	}

	old_x = old_y = 0;

	ShowCursor (FALSE);
	SetCursor (NULL);		// DirectX manual says: "cursor will disappear"; but, in some circusmances it appears ...
	return true;
}


//--------------- Standard Win32 mouse ----------------------

static qboolean	haveSpiMouse, haveSpiMouseSpeed;
static int originalMouseParms[3], originalMouseSpeed;


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
IN_ActivateMouse

Called when the window gains focus or changes in some way
===========
*/

void IN_DeactivateMouse (void);

static void IN_ActivateMouse (void)
{
	if (!mouseinitialized)
		return;

	if (mouseType)
	{
		if (!in_mouse->modified)
			return;
		Com_DPrintf ("reactivating mouse\n");
		// deactivate than reactivate mouse
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
IN_DeactivateMouse

Called when the window loses focus
===========
*/
static void IN_DeactivateMouse (void)
{
	if (!mouseinitialized)
		return;

	if (mouseType == 2)
		IN_FreeDirect ();
	else if (mouseType == 1)
		IN_FreeWin32 ();

	mouseType = 0;
}



/*
===========
IN_StartupMouse
===========
*/

void IN_StartupMouse (void)
{
	cvar_t		*cv;

	cv = Cvar_Get ("in_initmouse", "1", CVAR_NOSET);
	if (!cv->integer)
		return;

	haveSpiMouse = SystemParametersInfo (SPI_GETMOUSE, 0, originalMouseParms, 0);
	haveSpiMouseSpeed = SystemParametersInfo (SPI_GETMOUSESPEED, 0, &originalMouseSpeed, 0);	// Win98+, Win2K+
	mouse_buttons = 3;		//??
	mouseinitialized = true;
}

/*
===========
IN_MouseEvent
===========
*/
void IN_MouseEvent (int mstate)
{
	int		i;

	if (!mouseinitialized)
		return;

	// perform button actions
	for (i = 0; i < mouse_buttons; i++)
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

static int move_x, move_y;


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
		// find mouse movement
		if (!GetCursorPos (&current_pos))
			return;

		mx = current_pos.x - window_center_x;
		my = current_pos.y - window_center_y;
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

	mouse_x = Q_round (mouse_x * sensitivity->value * 1.5f);
	mouse_y = Q_round (mouse_y * sensitivity->value * 1.5f);

	// add mouse X/Y movement to cmd
	if ((in_Strafe.state & 1) || (lookstrafe->integer && mlooking))
		cmd->sidemove += Q_round (m_side->value * mouse_x);
	else
		cl.viewangles[YAW] -= m_yaw->value * mouse_x;

	if ((mlooking || freelook->value) && !(in_Strafe.state & 1))
		cl.viewangles[PITCH] += m_pitch->value * mouse_y;
	else
		cmd->forwardmove -= Q_round (m_forward->value * mouse_y);

	// force the mouse to the center, so there's room to move
	if (mouseType != 2)
		SetCursorPos (window_center_x, window_center_y);
}


/*
=========================================================================

VIEW CENTERING

=========================================================================
*/

cvar_t	*v_centermove;
cvar_t	*v_centerspeed;


/*
===========
IN_Init
===========
*/
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

	// centering
	CVAR_VAR(v_centermove, 0.15, 0),
	CVAR_VAR(v_centerspeed, 500, 0)
CVAR_END

	Cvar_GetVars (ARRAY_ARG(vars));

	Cmd_AddCommand ("+mlook", IN_MLookDown);
	Cmd_AddCommand ("-mlook", IN_MLookUp);

	Cmd_AddCommand ("joy_advancedupdate", Joy_AdvancedUpdate_f);

	IN_StartupMouse ();
	IN_StartupJoystick ();
}

/*
===========
IN_Shutdown
===========
*/
void IN_Shutdown (void)
{
	IN_DeactivateMouse ();
}


/*
===========
IN_Activate

Called when the main window gains or loses focus.
The window may have been destroyed and recreated
between a deactivate and an activate.
===========
*/
void IN_Activate (qboolean active)
{	//!! check this function
	in_appactive = active;
	mouseType = !active;		// force a new window check or turn off
}


/*
==================
IN_Frame

Called every frame, even if not generating commands
==================
*/
void IN_Frame (void)
{
	if (!mouseinitialized)
		return;

	if (!in_mouse || !in_appactive)	//??
	{
		IN_DeactivateMouse ();
		return;
	}

	if (in_needRestart)
	{
		in_needRestart = false;
		IN_DeactivateMouse ();
		IN_ActivateMouse ();
		return;
	}

	if (in_mouse->modified)
		IN_ActivateMouse ();

	if (mouseType == 2)
	{
		DIMOUSESTATE ms;
		HRESULT hresult;
		int		tmp;

		// poll DirectInput mouse
		hresult = IDirectInputDevice_GetDeviceState (pDID, sizeof(ms), &ms);
		if (hresult == DIERR_INPUTLOST)
		{
			// try to acquire mouse
			IDirectInputDevice_Acquire (pDID);
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
			int		msg;

			if (ms.lZ > 0)
				msg = K_MWHEELUP;
			else
				msg = K_MWHEELDOWN;
			Key_Event (msg, true, sys_msg_time);	//!! check for correct sys_msg_time
			Key_Event (msg, false, sys_msg_time);
		}
		// process X and Y axes
		move_x = ms.lX;
		move_y = ms.lY;
		// process buttons
		tmp = 0;
		if (ms.rgbButtons[0]) tmp |= 1;
		if (ms.rgbButtons[1]) tmp |= 2;
		if (ms.rgbButtons[2]) tmp |= 4;
		IN_MouseEvent (tmp);			// call this always - to detect button on/off
		//?? can access 4 buttons
		return;
	}


	if (!cl.refresh_prepped || cls.key_dest == key_console || cls.key_dest == key_menu)
	{
		// temporarily deactivate if in fullscreen
		if (!Cvar_VariableInt ("r_fullscreen"))
		{
			IN_DeactivateMouse ();
			return;
		}
	}

	IN_ActivateMouse ();
}

/*
===========
IN_Move
===========
*/
void IN_Move (usercmd_t *cmd)
{
	IN_MouseMove (cmd);

	if (ActiveApp)
		IN_JoyMove (cmd);
}


/*
===================
IN_ClearStates
===================
*/
void IN_ClearStates (void)
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

/*
===============
IN_StartupJoystick
===============
*/
void IN_StartupJoystick (void)
{
	int			numdevs;
	JOYCAPS		jc;
	MMRESULT	mmr;
	cvar_t		*cv;

 	// assume no joystick
	joy_avail = false;

	// abort startup if user requests no joystick
	cv = Cvar_Get ("in_initjoy", "1", CVAR_NOSET);
	if ( !cv->integer )
		return;

	// verify joystick driver is present
	if ((numdevs = joyGetNumDevs ()) == 0)
	{
//		Com_Printf ("\njoystick not found -- driver not present\n\n");
		return;
	}

	// cycle through the joystick ids for the first valid one
	for (joy_id = 0; joy_id < numdevs; joy_id++)
	{
		memset (&ji, 0, sizeof(ji));
		ji.dwSize = sizeof(ji);
		ji.dwFlags = JOY_RETURNCENTERED;

		if ((mmr = joyGetPosEx (joy_id, &ji)) == JOYERR_NOERROR)
			break;
	}

	// abort startup if we didn't find a valid joystick
	if (mmr != JOYERR_NOERROR)
	{
		Com_Printf ("\njoystick not found -- no valid joysticks (%x)\n\n", mmr);
		return;
	}

	// get the capabilities of the selected joystick
	// abort startup if command fails
	memset (&jc, 0, sizeof(jc));
	if ((mmr = joyGetDevCaps (joy_id, &jc, sizeof(jc))) != JOYERR_NOERROR)
	{
		Com_Printf ("\njoystick not found -- invalid joystick capabilities (%x)\n\n", mmr);
		return;
	}

	// save the joystick's number of buttons and POV status
	joy_numbuttons = jc.wNumButtons;
	joy_haspov = jc.wCaps & JOYCAPS_HASPOV;

	// old button and POV states default to no buttons pressed
	joy_oldbuttonstate = joy_oldpovstate = 0;

	// mark the joystick as available and advanced initialization not completed
	// this is needed as cvars are not available during initialization

	joy_avail = true;
	joy_advancedinit = false;

	Com_Printf ("\njoystick detected\n\n");
}


/*
===========
RawValuePointer
===========
*/
PDWORD RawValuePointer (int axis)
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
void Joy_AdvancedUpdate_f (void)
{

	// called once by IN_ReadJoystick and by user whenever an update is needed
	// cvars are now available
	int	i;
	DWORD dwTemp;

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
		dwTemp = (DWORD) joy_advaxisx->integer;
		dwAxisMap[JOY_AXIS_X] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_X] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisy->integer;
		dwAxisMap[JOY_AXIS_Y] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_Y] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisz->integer;
		dwAxisMap[JOY_AXIS_Z] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_Z] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisr->integer;
		dwAxisMap[JOY_AXIS_R] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_R] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisu->integer;
		dwAxisMap[JOY_AXIS_U] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_U] = dwTemp & JOY_RELATIVE_AXIS;
		dwTemp = (DWORD) joy_advaxisv->integer;
		dwAxisMap[JOY_AXIS_V] = dwTemp & 0x0000000f;
		dwControlMap[JOY_AXIS_V] = dwTemp & JOY_RELATIVE_AXIS;
	}

	// compute the axes to collect from DirectInput
	joy_flags = JOY_RETURNCENTERED | JOY_RETURNBUTTONS | JOY_RETURNPOV;
	for (i = 0; i < JOY_MAX_AXES; i++)
	{
		if (dwAxisMap[i] != AxisNada)
		{
			joy_flags |= dwAxisFlags[i];
		}
	}
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

	if (!joy_avail)
		return;

	// loop through the joystick buttons
	// key a joystick event or auxillary event for higher number buttons for each state change
	buttonstate = ji.dwButtons;
	for (i=0 ; i < joy_numbuttons ; i++)
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
qboolean IN_ReadJoystick (void)
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
void IN_JoyMove (usercmd_t *cmd)
{
	float	speed, aspeed;
	float	fAxisValue;
	int		i;

	// complete initialization if first time in
	// this is needed as cvars are not available at initialization time
	if( joy_advancedinit != true )
	{
		Joy_AdvancedUpdate_f ();
		joy_advancedinit = true;
	}

	// verify joystick is available and that the user wants to use it
	if (!joy_avail || !in_joystick->integer)
	{
		return;
	}

	// collect the joystick data, if possible
	if (IN_ReadJoystick () != true)
	{
		return;
	}

	if ((in_Speed.state & 1) ^ cl_run->integer)
		speed = 2;
	else
		speed = 1;
	aspeed = speed * cls.frametime;

	// loop through the axes
	for (i = 0; i < JOY_MAX_AXES; i++)
	{
		// get the floating point zero-centered, potentially-inverted data for the current axis
		fAxisValue = (float) *pdwRawValue[i];
		// move centerpoint to zero
		fAxisValue -= 32768.0;

		// convert range from -32768..32767 to -1..1
		fAxisValue /= 32768.0;

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
					if (m_pitch->value < 0.0)
						cl.viewangles[PITCH] -= (fAxisValue * joy_pitchsensitivity->value) * aspeed * cl_pitchspeed->value;
					else
						cl.viewangles[PITCH] += (fAxisValue * joy_pitchsensitivity->value) * aspeed * cl_pitchspeed->value;
				}
			}
			else
			{
				// user wants forward control to be forward control
				if (fabs(fAxisValue) > joy_forwardthreshold->value)
					cmd->forwardmove += Q_round ((fAxisValue * joy_forwardsensitivity->value) * speed * cl_forwardspeed->value);
			}
			break;

		case AxisSide:
			if (fabs(fAxisValue) > joy_sidethreshold->value)
				cmd->sidemove += Q_round ((fAxisValue * joy_sidesensitivity->value) * speed * cl_sidespeed->value);
			break;

		case AxisUp:
			if (fabs(fAxisValue) > joy_upthreshold->value)
				cmd->upmove += Q_round ((fAxisValue * joy_upsensitivity->value) * speed * cl_upspeed->value);
			break;

		case AxisTurn:
			if ((in_Strafe.state & 1) || (lookstrafe->value && mlooking))
			{
				// user wants turn control to become side control
				if (fabs(fAxisValue) > joy_sidethreshold->value)
					cmd->sidemove -= Q_round ((fAxisValue * joy_sidesensitivity->value) * speed * cl_sidespeed->value);
			}
			else
			{
				// user wants turn control to be turn control
				if (fabs(fAxisValue) > joy_yawthreshold->value)
				{
					if(dwControlMap[i] == JOY_ABSOLUTE_AXIS)
						cl.viewangles[YAW] += (fAxisValue * joy_yawsensitivity->value) * aspeed * cl_yawspeed->value;
					else
						cl.viewangles[YAW] += (fAxisValue * joy_yawsensitivity->value) * speed * 180.0;

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
						cl.viewangles[PITCH] += (fAxisValue * joy_pitchsensitivity->value) * speed * 180.0;
				}
			}
			break;

		default:
			break;
		}
	}
}
