extern	cvar_t	*cl_upspeed;
extern	cvar_t	*cl_forwardspeed;
extern	cvar_t	*cl_sidespeed;

extern	cvar_t	*cl_yawspeed;
extern	cvar_t	*cl_pitchspeed;

extern	cvar_t	*cl_run;

extern	cvar_t	*sensitivity;
extern	cvar_t	*m_invert;

extern	cvar_t	*cl_infps;

// declarations for exporting to in_win32.cpp: (make local for cl_input.cpp!)
struct kbutton_t
{
	short		down[2];		// key nums holding it down
	byte		state;
	unsigned	downtime;		// msec timestamp
	unsigned	msec;			// != 0 only for frame, when button released
};
extern kbutton_t in_Speed;

void CL_InitInput ();
void CL_SendCmd ();

// platform code
void IN_Init ();
void IN_Shutdown ();
void IN_Activate (bool active);

void IN_Frame ();

void IN_Move (usercmd_t *cmd);
