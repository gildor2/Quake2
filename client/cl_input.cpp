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
// cl.input.cpp  -- builds an intended movement command to send to the server

#include "client.h"

static cvar_t	*cl_nodelta;

// used as time delta from previous input frame; UGLY!!
//?? PROBLEMS:
//?? - we needs sys_frame_time -> exact time of key down/up (comes from message in win32) - for exact
//??   timings of button down time in systems with low FPS (to allow downtime less than frame time)
//?? - but, we must compare sys_frame_time with some timer, when button holded more than one frame; this
//??   timer should be the same, as used for sys_frame_time
//?? - exact downtime value will be received, when button will be released (up)
//?? - so, when buttom is pressed/released, we will get following frames:
//??   1.      down:   downtime = time - msg.sys_frame_time     // where to get "time" ?
//??   2..N-1. holded: downtime = frame_time                    // all OK - frame_time is known (input for CL_Frame())
//??   N.      up:     downtime = msg.sys_frame_time - prevTime // prevTime = time of prev frame (see 1.)
extern unsigned sys_frame_time;
static unsigned old_sys_frame_time;
static unsigned frame_msec;
//!! NOTE: may be, use timescale for msec values (will affect all movement, depending on KeyState() call value)

static float accum_frame_time;




#define STATE_DOWN			1
#define STATE_IMPULSEDOWN	2	// used for "attack" and "use" only
#define STATE_IMPULSEUP		4	// not used


static kbutton_t	in_Left, in_Right, in_Forward, in_Back;
static kbutton_t	in_Lookup, in_Lookdown, in_Moveleft, in_Moveright;
static kbutton_t	in_Use, in_Attack;
static kbutton_t	in_Up, in_Down;
kbutton_t			in_Speed;


//?? make as method of kbutton_t
static void KeyDown (kbutton_t &b, char **argv)
{
	int		k;

	if (*argv[1])
		k = atoi (argv[1]);
	else
		k = -1;					// typed manually at the console for continuous down

	if (k == b.down[0] || k == b.down[1])
		return;					// repeating key

	if (!b.down[0])
		b.down[0] = k;
	else if (!b.down[1])
		b.down[1] = k;
	else
	{
		Com_DPrintf ("KeyDown: 3 keys down for a button\n");
		return;
	}

	if (b.state & STATE_DOWN) return;

	// save timestamp
	b.downtime = atoi (argv[2]);
	if (!b.downtime)
		b.downtime = sys_frame_time - 100;	//?? const=100; situation: KeyDown() <- console, downtime = time of prev frame

	b.state |= STATE_DOWN|STATE_IMPULSEDOWN;
}

//?? make as method of kbutton_t
static void KeyUp (kbutton_t &b, char **argv)
{
	int		k;

	if (*argv[1])
		k = atoi (argv[1]);
	else
	{
		// typed manually at the console, assume for unsticking, so clear all
		b.down[0] = b.down[1] = 0;
		b.state = STATE_IMPULSEUP;
		return;
	}

	if (b.down[0] == k)
		b.down[0] = 0;
	else if (b.down[1] == k)
		b.down[1] = 0;
	else
		return;					// key up without corresponding down (menu pass through)

	if (b.down[0] || b.down[1])
		return;					// some other key is still holding it down

	if (!(b.state & STATE_DOWN))
		return;					// up (this should not happen: 2 times "up" one by one)

	// save timestamp
	unsigned uptime = atoi (argv[2]);
	if (uptime)
		b.msec += uptime - b.downtime;
	else
		b.msec += 10;			//?? const=10; should allow using KeyDown()+KeyUp() in a single frame to produce msec>0

	b.state &= ~STATE_DOWN;
	b.state |= STATE_IMPULSEUP;
}


// Returns the fraction of the frame that the key was down
//?? make as method of kbutton_t
static float KeyState (kbutton_t &key)
{
	int msec = key.msec;		// != 0 only if KeyUp() called ...

	key.state &= STATE_DOWN;	// clear impulses
	key.msec = 0;

	// here: key.state == 0|STATE_DOWN
	if (key.state)
	{
		// still down; really, "msec" here == 0
		msec += sys_frame_time - key.downtime;
		key.downtime = sys_frame_time;
	}

	float val = (float)msec / frame_msec;
	return bound(val, 0, 1);
}


static float KeyDelta (kbutton_t &key1, kbutton_t &key2)
{
	return KeyState (key1) - KeyState (key2);
}



// can make this cvars static, if move joystick movement code here (cvars used in JoyMove() ...)
// some cvars used in menu.cpp, but this is not critical
cvar_t	*cl_upspeed;
cvar_t	*cl_forwardspeed;
cvar_t	*cl_sidespeed;

cvar_t	*cl_yawspeed;
cvar_t	*cl_pitchspeed;

cvar_t	*cl_run;

static cvar_t *cl_anglespeedkey;


static void KeyboardMove (usercmd_t *cmd)
{
	memset (cmd, 0, sizeof(usercmd_t));

	// rotate player with keyboard
	float speed = cls.frametime;
	if (in_Speed.state & STATE_DOWN)
		speed *= cl_anglespeedkey->value;

	cl.viewangles[YAW]   += speed * cl_yawspeed->value   * KeyDelta (in_Left, in_Right);
	cl.viewangles[PITCH] += speed * cl_pitchspeed->value * KeyDelta (in_Lookdown, in_Lookup);

	// copy angles with float->short
	cmd->angles[0] = appRound (cl.viewangles[0]);
	cmd->angles[1] = appRound (cl.viewangles[1]);
	cmd->angles[2] = appRound (cl.viewangles[2]);

	// movement
	cmd->forwardmove += appRound (cl_forwardspeed->value * KeyDelta (in_Forward, in_Back));
	cmd->sidemove    += appRound (cl_sidespeed->value    * KeyDelta (in_Moveright, in_Moveleft));
	cmd->upmove      += appRound (cl_upspeed->value      * KeyDelta (in_Up, in_Down));

	// adjust for speed key / running
	if ((in_Speed.state & STATE_DOWN) ^ cl_run->integer)
	{
		cmd->forwardmove *= 2;
		cmd->sidemove    *= 2;
		cmd->upmove      *= 2;
	}
}


static void ClampPitch ()
{
	float pitch = SHORT2ANGLE(cl.frame.playerstate.pmove.delta_angles[PITCH]);
	if (pitch > 180)
		pitch -= 360;

	if (cl.viewangles[PITCH] + pitch > 89)
		cl.viewangles[PITCH] = 89 - pitch;
	if (cl.viewangles[PITCH] + pitch < -89)
		cl.viewangles[PITCH] = -89 - pitch;
}


static void FinishMove (usercmd_t *cmd)
{
	// figure button bits
	if (in_Attack.state & (STATE_DOWN|STATE_IMPULSEDOWN))
		cmd->buttons |= BUTTON_ATTACK;
	in_Attack.state &= ~STATE_IMPULSEDOWN;

	if (in_Use.state & (STATE_DOWN|STATE_IMPULSEDOWN))
		cmd->buttons |= BUTTON_USE;
	in_Use.state &= ~STATE_IMPULSEDOWN;

	if (keysDown && cls.key_dest == key_game)
		cmd->buttons |= BUTTON_ANY;

	// send milliseconds of time to apply the move
	int ms = appRound (accum_frame_time * 1000);
	if (ms > 250) ms = 250;

	cmd->msec = ms;

	ClampPitch ();
	for (int i = 0; i < 3; i++)
		cmd->angles[i] = ANGLE2SHORT(cl.viewangles[i]);

	cmd->impulse    = 0;		//!! unused
	// send the ambient light level at the player's current position
	cmd->lightlevel = appRound (RE_GetClientLight ());
}


static void CreateCmd (usercmd_t *cmd)
{
	frame_msec = sys_frame_time - old_sys_frame_time;
	frame_msec = bound(frame_msec, 1, 200);

	// save view angles to restore it when needed
	CVec3 oldAngles = cl.viewangles;

	KeyboardMove (cmd);
	IN_Move (cmd);			// mouse and joystick movement (platform-dependent)
	FinishMove (cmd);

	// if client paused, do not produce movement
	if (cl_paused->integer)
	{
		//?? if not in game (menu, console, gui ...) -- clear usercmd too
		memset (cmd, 0, sizeof(usercmd_t));
		cl.viewangles = oldAngles;
	}

	old_sys_frame_time = sys_frame_time;

//??	cmd->impulse = ??? ; -- unused in server now
}


static void IN_CenterView ()
{
	cl.viewangles[PITCH] = -SHORT2ANGLE(cl.frame.playerstate.pmove.delta_angles[PITCH]);
}


static void IN_Lookdown (bool usage, int argc, char **argv)
{
	if (argc != 2 || usage)
	{
		appPrintf ("Usage: lookdown <angle>\n");
		return;
	}
	cl.viewangles[PITCH] += atof (argv[1]);
}


static void IN_Lookup (bool usage, int argc, char **argv)	// can be used "lookdown -angle" instead
{
	if (argc != 2 || usage)
	{
		appPrintf ("Usage: lookup <angle>\n");
		return;
	}
	cl.viewangles[PITCH] -= atof (argv[1]);
}


// Declare console functions
#define KB(name)	\
static void IN_##name##Up (int argc, char **argv) { KeyUp(in_##name, argv); }	\
static void IN_##name##Down (int argc, char **argv) { KeyDown(in_##name, argv); }
	KB(Up)			KB(Down)
	KB(Left)		KB(Right)
	KB(Forward)		KB(Back)
	KB(Lookup)		KB(Lookdown)
	KB(Moveleft)	KB(Moveright)
	KB(Speed)
	KB(Attack)		KB(Use)
#undef KB


void CL_InitInput ()
{
CVAR_BEGIN(vars)
	// movement speed
	CVAR_VAR(cl_upspeed,      200, 0),
	CVAR_VAR(cl_forwardspeed, 200, 0),
	CVAR_VAR(cl_sidespeed,    200, 0),
	// rotation using keyboard speed
	CVAR_VAR(cl_yawspeed,   140, 0),
	CVAR_VAR(cl_pitchspeed, 150, 0),
	CVAR_VAR(cl_anglespeedkey, 1.5, 0),
	// other
	CVAR_VAR(cl_run, 0, CVAR_ARCHIVE),		// always run; make default==1 ??
CVAR_END
	Cvar_GetVars (ARRAY_ARG(vars));

	RegisterCommand ("centerview", IN_CenterView);
#define KB(name, str)	\
	RegisterCommand ("+" #str, IN_##name##Down); \
	RegisterCommand ("-" #str, IN_##name##Up);
	KB(Up, moveup);			KB(Down, movedown);
	KB(Left, left);			KB(Right, right);
	KB(Forward, forward);	KB(Back, back);
	KB(Lookup, lookup);		KB(Lookdown, lookdown);
	KB(Moveleft, moveleft);	KB(Moveright, moveright);
	KB(Speed, speed);
	KB(Attack, attack);		KB(Use, use);
#undef KB

	RegisterCommand ("lookdown", IN_Lookdown);
	RegisterCommand ("lookup", IN_Lookup);

	cl_nodelta = Cvar_Get ("cl_nodelta", "0");
}


// network stuff
void CL_SendCmd ()
{
	accum_frame_time += cls.frametime;

	// build a command even if not connected
	// save this command off for prediction
	int i = cls.netchan.outgoing_sequence & (CMD_BACKUP-1);
	usercmd_t *cmd = &cl.cmds[i];
	cl.cmd_time[i] = cls.realtime;	// for netgraph ping calculation

	CreateCmd (cmd);

	cls.netFrameDropped = true;		// will be set to false after sending command to server
	if (cl_infps->integer && accum_frame_time < 1.0f / cl_infps->value)
		return;

	if (cls.state == ca_disconnected || cls.state == ca_connecting)
		return;

	if (cls.state == ca_connected)
	{
		if (cls.netchan.message.cursize	|| cls.realtime - cls.netchan.last_sent > 1000)
			cls.netchan.Transmit (NULL, 0);
		return;
	}

	// restrict outgoing commands to avoid CMD_BACKUP overflow when cl.fps/sv.fps is too
	// large (fast system or small timescale)
	int fillness = cls.netchan.outgoing_sequence - cls.netchan.incoming_acknowledged;
	if (fillness > CMD_BACKUP/10 && (float)fillness / CMD_BACKUP > cl.lerpfrac * 0.9f) //!! restrict FPS: || accum_frame_time < 0.005)
		return;

	if (cmd->msec == 0)
		return;

	// send a userinfo update if needed
	// disallow sending userinfo notification while playing demos
	// (when enabled, one of demo' player will be re-skinned as local player)
	if ((cvar_t::modifiedFlags & CVAR_USERINFO) && !cl.attractloop)
	{
		cvar_t::modifiedFlags &= ~CVAR_USERINFO;
		// update model/gender (really, server will send clientinfo back, but we already
		// updated it; and more: this will update "gender" when "gender_auto" is set;
		// "gender" will be recomputed later (when server send info back) too, but this
		// will require to send userinfo to server again)
		CL_UpdatePlayerClientInfo ();
		// send userinfo
		MSG_WriteByte (&cls.netchan.message, clc_userinfo);
		MSG_WriteString (&cls.netchan.message, Cvar_BitInfo (CVAR_USERINFO));
	}

	sizebuf_t buf;
	byte	data[128];
	buf.Init (ARRAY_ARG(data));

	// stop the cinematic when any key is down
	if (cmd->buttons && cl.cinematicActive && !cl.attractloop)
		SCR_StopCinematic ();

	// begin a client move command
	MSG_WriteByte (&buf, clc_move);

	// save the position for a checksum byte
	int checksumIndex = buf.cursize;
	MSG_WriteByte (&buf, 0);

	// let the server know what the last frame we
	// got was, so the next message can be delta compressed
	if (cl_nodelta->integer || !cl.frame.valid || cls.demowaiting)
		MSG_WriteLong (&buf, -1);	// no compression
	else
		MSG_WriteLong (&buf, cl.frame.serverframe);

	// send this and the previous cmds in the message, so
	// if the last packet was dropped, it can be recovered
	usercmd_t nullcmd;
	memset (&nullcmd, 0, sizeof(nullcmd));

	cmd = &cl.cmds[(cls.netchan.outgoing_sequence-2) & (CMD_BACKUP-1)];
	MSG_WriteDeltaUsercmd (&buf, &nullcmd, cmd);
	usercmd_t *oldcmd = cmd;

	cmd = &cl.cmds[(cls.netchan.outgoing_sequence-1) & (CMD_BACKUP-1)];
	MSG_WriteDeltaUsercmd (&buf, oldcmd, cmd);
	oldcmd = cmd;

	cmd = &cl.cmds[(cls.netchan.outgoing_sequence) & (CMD_BACKUP-1)];
	MSG_WriteDeltaUsercmd (&buf, oldcmd, cmd);

	// calculate a checksum over the move commands
	buf.data[checksumIndex] = Com_BlockSequenceCRCByte(
		buf.data + checksumIndex + 1, buf.cursize - checksumIndex - 1,
		cls.netchan.outgoing_sequence);

	// deliver the message
	cls.netchan.Transmit (buf.data, buf.cursize);

	// prepare for next frame
	cls.netFrameDropped = false;
	accum_frame_time = 0;
}
