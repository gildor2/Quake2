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
// cl.input.c  -- builds an intended movement command to send to the server

#include "client.h"

static cvar_t	*cl_nodelta;

extern	unsigned	sys_frame_time;

static unsigned	frame_msec;
static unsigned	old_sys_frame_time;
static float accum_frame_time;

/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as a parameter to the command so it can be matched up with
the release.

state bit 0 is the current state of the key
state bit 1 is edge triggered on the up to down transition
state bit 2 is edge triggered on the down to up transition


Key_Event (int key, bool down, unsigned time);

  +mlook src time

===============================================================================
*/


static kbutton_t	in_KLook;
static kbutton_t	in_Left, in_Right, in_Forward, in_Back;
static kbutton_t	in_Lookup, in_Lookdown, in_Moveleft, in_Moveright;
static kbutton_t	in_Use, in_Attack;
static kbutton_t	in_Up, in_Down;
kbutton_t	in_Strafe, in_Speed;


static void KeyDown (kbutton_t *b, char **argv)
{
	int		k;

	char *c = argv[1];
	if (c[0])
		k = atoi(c);
	else
		k = -1;					// typed manually at the console for continuous down

	if (k == b->down[0] || k == b->down[1])
		return;					// repeating key

	if (!b->down[0])
		b->down[0] = k;
	else if (!b->down[1])
		b->down[1] = k;
	else
	{
		Com_Printf ("Three keys down for a button!\n");
		return;
	}

	if (b->state & 1)
		return;					// still down

	// save timestamp
	c = argv[2];
	b->downtime = atoi(c);
	if (!b->downtime)
		b->downtime = sys_frame_time - 100;

	b->state |= 1 + 2;			// down + impulse down
}

static void KeyUp (kbutton_t *b, char **argv)
{
	int		k;
	char	*c;
	unsigned	uptime;

	c = argv[1];
	if (c[0])
		k = atoi(c);
	else
	{	// typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->state = 4;			// impulse up
		return;
	}

	if (b->down[0] == k)
		b->down[0] = 0;
	else if (b->down[1] == k)
		b->down[1] = 0;
	else
		return;					// key up without corresponding down (menu pass through)
	if (b->down[0] || b->down[1])
		return;					// some other key is still holding it down

	if (!(b->state & 1))
		return;					// still up (this should not happen)

	// save timestamp
	c = argv[2];
	uptime = atoi(c);
	if (uptime)
		b->msec += uptime - b->downtime;
	else
		b->msec += 10;

	b->state &= ~1;				// now up
	b->state |= 4; 				// impulse up
}

// Declare console functions
#define KB(name)	\
static void IN_##name##Up (int argc, char **argv) { KeyUp(&in_##name, argv); }	\
static void IN_##name##Down (int argc, char **argv) { KeyDown(&in_##name, argv); }

	KB(KLook)
	KB(Up)			KB(Down)
	KB(Right)		KB(Left)
	KB(Forward)		KB(Back)
	KB(Lookup)		KB(Lookdown)
	KB(Moveleft)	KB(Moveright)
	KB(Speed)
	KB(Strafe)
	KB(Attack)		KB(Use)

#undef KB

/*
===============
KeyState

Returns the fraction of the frame that the key was down
===============
*/
static float KeyState (kbutton_t *key)
{
	float		val;
	int			msec;

	key->state &= 1;			// clear impulses

	msec = key->msec;
	key->msec = 0;

	if (key->state)
	{	// still down
		msec += sys_frame_time - key->downtime;
		key->downtime = sys_frame_time;
	}

	val = (float)msec / frame_msec;
	val = bound(val, 0, 1);

	return val;
}




//==========================================================================

cvar_t	*cl_upspeed;
cvar_t	*cl_forwardspeed;
cvar_t	*cl_sidespeed;

cvar_t	*cl_yawspeed;
cvar_t	*cl_pitchspeed;

cvar_t	*cl_run;

cvar_t	*cl_anglespeedkey;


/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
static void AdjustAngles (void)
{
	float	speed;
	float	up, down;

	if (in_Speed.state & 1)
		speed = cls.frametime * cl_anglespeedkey->value;
	else
		speed = cls.frametime;

	if (!(in_Strafe.state & 1))
	{
		cl.viewangles[YAW] -= speed * cl_yawspeed->value * KeyState (&in_Right);
		cl.viewangles[YAW] += speed * cl_yawspeed->value * KeyState (&in_Left);
	}
	if (in_KLook.state & 1)
	{
		cl.viewangles[PITCH] -= speed * cl_pitchspeed->value * KeyState (&in_Forward);
		cl.viewangles[PITCH] += speed * cl_pitchspeed->value * KeyState (&in_Back);
	}

	up = KeyState (&in_Lookup);
	down = KeyState(&in_Lookdown);

	cl.viewangles[PITCH] -= speed * cl_pitchspeed->value * up;
	cl.viewangles[PITCH] += speed * cl_pitchspeed->value * down;
}

/*
================
CL_BaseMove

Send the intended movement message to the server
================
*/
static void BaseMove (usercmd_t *cmd)
{
	AdjustAngles ();

	memset (cmd, 0, sizeof(*cmd));

	// copy angles with float->short
	cmd->angles[0] = appRound (cl.viewangles[0]);
	cmd->angles[1] = appRound (cl.viewangles[1]);
	cmd->angles[2] = appRound (cl.viewangles[2]);
	if (in_Strafe.state & 1)
	{
		cmd->sidemove += appRound (cl_sidespeed->value * KeyState (&in_Right));
		cmd->sidemove -= appRound (cl_sidespeed->value * KeyState (&in_Left));
	}

	cmd->sidemove += appRound (cl_sidespeed->value * KeyState (&in_Moveright));
	cmd->sidemove -= appRound (cl_sidespeed->value * KeyState (&in_Moveleft));

	cmd->upmove += appRound (cl_upspeed->value * KeyState (&in_Up));
	cmd->upmove -= appRound (cl_upspeed->value * KeyState (&in_Down));

	if (! (in_KLook.state & 1) )
	{
		cmd->forwardmove += appRound (cl_forwardspeed->value * KeyState (&in_Forward));
		cmd->forwardmove -= appRound (cl_forwardspeed->value * KeyState (&in_Back));
	}

	// adjust for speed key / running
	if ((in_Speed.state & 1) ^ cl_run->integer)
	{
		cmd->forwardmove *= 2;
		cmd->sidemove *= 2;
		cmd->upmove *= 2;
	}
}

static void ClampPitch (void)
{
	float	pitch;

	pitch = SHORT2ANGLE(cl.frame.playerstate.pmove.delta_angles[PITCH]);
	if (pitch > 180)
		pitch -= 360;

	if (cl.viewangles[PITCH] + pitch > 89)
		cl.viewangles[PITCH] = 89 - pitch;
	if (cl.viewangles[PITCH] + pitch < -89)
		cl.viewangles[PITCH] = -89 - pitch;
}

/*
==============
CL_FinishMove
==============
*/
static void FinishMove (usercmd_t *cmd)
{
	int		i, ms;

	// figure button bits
	if (in_Attack.state & 3)
		cmd->buttons |= BUTTON_ATTACK;
	in_Attack.state &= ~2;

	if (in_Use.state & 3)
		cmd->buttons |= BUTTON_USE;
	in_Use.state &= ~2;

	if (keysDown && cls.key_dest == key_game)
		cmd->buttons |= BUTTON_ANY;

	// send milliseconds of time to apply the move
	ms = appRound (accum_frame_time * 1000);
	if (ms > 250) ms = 250;

	cmd->msec = ms;

	ClampPitch ();
	for (i = 0; i < 3; i++)
		cmd->angles[i] = ANGLE2SHORT(cl.viewangles[i]);

	cmd->impulse = 0;		//!! unused

	// send the ambient light level at the player's current position
	cmd->lightlevel = appRound (re.GetClientLight ());
}

/*
=================
CL_CreateCmd
=================
*/
static void CreateCmd (usercmd_t *cmd)
{
	frame_msec = sys_frame_time - old_sys_frame_time;
	frame_msec = bound(frame_msec, 1, 200);

	// keyboard movement
	BaseMove (cmd);
	// mouse and joystick movement
	IN_Move (cmd);

	FinishMove (cmd);

	old_sys_frame_time = sys_frame_time;

//??	cmd->impulse = ??? ; -- unused in server now
}


void IN_CenterView (void)
{
	cl.viewangles[PITCH] = -SHORT2ANGLE(cl.frame.playerstate.pmove.delta_angles[PITCH]);
}


static void IN_Lookdown (bool usage, int argc, char **argv)
{
	if (argc != 2 || usage)
	{
		Com_Printf ("Usage: lookdown <angle>\n");
		return;
	}
	cl.viewangles[PITCH] += atof (argv[1]);
}


static void IN_Lookup (bool usage, int argc, char **argv)	// can be used "lookdown -angle" instead
{
	if (argc != 2 || usage)
	{
		Com_Printf ("Usage: lookup <angle>\n");
		return;
	}
	cl.viewangles[PITCH] -= atof (argv[1]);
}


/*
============
CL_InitInput
============
*/
void CL_InitInput (void)
{
	RegisterCommand ("centerview", IN_CenterView);

	RegisterCommand ("+moveup", IN_UpDown);
	RegisterCommand ("-moveup", IN_UpUp);
	RegisterCommand ("+movedown", IN_DownDown);
	RegisterCommand ("-movedown", IN_DownUp);
	RegisterCommand ("+left", IN_LeftDown);
	RegisterCommand ("-left", IN_LeftUp);
	RegisterCommand ("+right", IN_RightDown);
	RegisterCommand ("-right", IN_RightUp);
	RegisterCommand ("+forward", IN_ForwardDown);
	RegisterCommand ("-forward", IN_ForwardUp);
	RegisterCommand ("+back", IN_BackDown);
	RegisterCommand ("-back", IN_BackUp);
	RegisterCommand ("+lookup", IN_LookupDown);
	RegisterCommand ("-lookup", IN_LookupUp);
	RegisterCommand ("+lookdown", IN_LookdownDown);
	RegisterCommand ("-lookdown", IN_LookdownUp);
	RegisterCommand ("+strafe", IN_StrafeDown);
	RegisterCommand ("-strafe", IN_StrafeUp);
	RegisterCommand ("+moveleft", IN_MoveleftDown);
	RegisterCommand ("-moveleft", IN_MoveleftUp);
	RegisterCommand ("+moveright", IN_MoverightDown);
	RegisterCommand ("-moveright", IN_MoverightUp);
	RegisterCommand ("+speed", IN_SpeedDown);
	RegisterCommand ("-speed", IN_SpeedUp);
	RegisterCommand ("+attack", IN_AttackDown);
	RegisterCommand ("-attack", IN_AttackUp);
	RegisterCommand ("+use", IN_UseDown);
	RegisterCommand ("-use", IN_UseUp);
	RegisterCommand ("+klook", IN_KLookDown);
	RegisterCommand ("-klook", IN_KLookUp);

	RegisterCommand ("lookdown", IN_Lookdown);
	RegisterCommand ("lookup", IN_Lookup);

	cl_nodelta = Cvar_Get ("cl_nodelta", "0", 0);
}



/*
=================
CL_SendCmd
=================
*/
void CL_SendCmd (void)
{
	sizebuf_t	buf;
	byte		data[128];
	int			i, fillness;
	usercmd_t	*cmd, *oldcmd;
	usercmd_t	nullcmd;
	int			checksumIndex;

	accum_frame_time += cls.frametime;

	// build a command even if not connected
	// save this command off for prediction
	i = cls.netchan.outgoing_sequence & (CMD_BACKUP-1);
	cmd = &cl.cmds[i];
	cl.cmd_time[i] = cls.realtime;	// for netgraph ping calculation

	CreateCmd (cmd);

	cls.netFrameDropped = true;		// will be set to false after sending command to server
	if (cl_infps->integer && accum_frame_time < 1.0f / cl_infps->value)
		return;

	if (cls.state == ca_disconnected || cls.state == ca_connecting)
		return;

	if (cls.state == ca_connected)
	{
		if (cls.netchan.message.cursize	|| curtime - cls.netchan.last_sent > 1000)
			Netchan_Transmit (&cls.netchan, 0, NULL);
		return;
	}

	// restrict outgoing commands to avoid CMD_BACKUP overflow when cl.fps/sv.fps is too
	// large (fast system or small timescale)
	fillness = cls.netchan.outgoing_sequence - cls.netchan.incoming_acknowledged;
	if (fillness > CMD_BACKUP/10 && (float)fillness / CMD_BACKUP > cl.lerpfrac * 0.9f) //!! restrict FPS: || accum_frame_time < 0.005)
		return;

	if (cmd->msec == 0)
		return;

	// send a userinfo update if needed
	// disallow sending userinfo notification while playing demos
	// (when enabled, one of demo' player will be re-skinned as local player)
	if (userinfo_modified && !cl.attractloop)
	{
		CL_FixUpGender();
		userinfo_modified = false;
		MSG_WriteByte (&cls.netchan.message, clc_userinfo);
		MSG_WriteString (&cls.netchan.message, Cvar_Userinfo() );
	}

	SZ_Init (&buf, data, sizeof(data));

	if (cmd->buttons && cl.cinematicTime > 0 && !cl.attractloop)
		SCR_FinishCinematic ();		// skip the rest of the cinematic with any key down

	// begin a client move command
	MSG_WriteByte (&buf, clc_move);

	// save the position for a checksum byte
	checksumIndex = buf.cursize;
	MSG_WriteByte (&buf, 0);

	// let the server know what the last frame we
	// got was, so the next message can be delta compressed
	if (cl_nodelta->integer || !cl.frame.valid || cls.demowaiting)
		MSG_WriteLong (&buf, -1);	// no compression
	else
		MSG_WriteLong (&buf, cl.frame.serverframe);

	// send this and the previous cmds in the message, so
	// if the last packet was dropped, it can be recovered
	i = (cls.netchan.outgoing_sequence-2) & (CMD_BACKUP-1);
	cmd = &cl.cmds[i];
	memset (&nullcmd, 0, sizeof(nullcmd));
	MSG_WriteDeltaUsercmd (&buf, &nullcmd, cmd);
	oldcmd = cmd;

	i = (cls.netchan.outgoing_sequence-1) & (CMD_BACKUP-1);
	cmd = &cl.cmds[i];
	MSG_WriteDeltaUsercmd (&buf, oldcmd, cmd);
	oldcmd = cmd;

	i = (cls.netchan.outgoing_sequence) & (CMD_BACKUP-1);
	cmd = &cl.cmds[i];
	MSG_WriteDeltaUsercmd (&buf, oldcmd, cmd);

	// calculate a checksum over the move commands
	buf.data[checksumIndex] = COM_BlockSequenceCRCByte(
		buf.data + checksumIndex + 1, buf.cursize - checksumIndex - 1,
		cls.netchan.outgoing_sequence);

	// deliver the message
	Netchan_Transmit (&cls.netchan, buf.cursize, buf.data);

	// prepare for next frame
	cls.netFrameDropped = false;
	accum_frame_time = 0;
}
