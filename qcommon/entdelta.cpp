#include "qcommon.h"


/*-----------------------------------------------------------------------------
	entity_state_t communication
-----------------------------------------------------------------------------*/

#define U_MODEL_N	(U_MODEL|U_MODEL2|U_MODEL3|U_MODEL4)
#define U_ORIGIN_N	(U_ORIGIN1|U_ORIGIN2|U_ORIGIN3)
#define U_ANGLE_N	(U_ANGLE1|U_ANGLE2|U_ANGLE3)

// try to pack the common update flags into the first byte
#define	U_ORIGIN1			(1<<0)
#define	U_ORIGIN2			(1<<1)
#define	U_ANGLE2			(1<<2)
#define	U_ANGLE3			(1<<3)
#define	U_FRAME8			(1<<4)		// frame is a byte
#define	U_EVENT				(1<<5)
#define	U_REMOVE			(1<<6)		// REMOVE this entity, don't add it
#define	U_MOREBITS1			(1<<7)		// read one additional byte

// second byte
#define	U_NUMBER16			(1<<8)		// NUMBER8 is implicit if not set
#define	U_ORIGIN3			(1<<9)
#define	U_ANGLE1			(1<<10)
#define	U_MODEL				(1<<11)
#define U_RENDERFX8			(1<<12)		// fullbright, etc
#define	U_EFFECTS8			(1<<14)		// autorotate, trails, etc
#define	U_MOREBITS2			(1<<15)		// read one additional byte

// third byte
#define	U_SKIN8				(1<<16)
#define	U_FRAME16			(1<<17)		// frame is a short
#define	U_RENDERFX16		(1<<18)		// 8 + 16 = 32
#define	U_EFFECTS16			(1<<19)		// 8 + 16 = 32
#define	U_MODEL2			(1<<20)		// weapons, flags, etc
#define	U_MODEL3			(1<<21)
#define	U_MODEL4			(1<<22)
#define	U_MOREBITS3			(1<<23)		// read one additional byte

// fourth byte
#define	U_OLDORIGIN			(1<<24)		// FIXME: get rid of this ??
#define	U_SKIN16			(1<<25)
#define	U_SOUND				(1<<26)
#define	U_SOLID				(1<<27)


void MSG_WriteDeltaEntity (sizebuf_t *msg, entity_state_t *from, entity_state_t *to, bool force, bool newentity)
{
	guard(MSG_WriteDeltaEntity);

	if (!to->number)
		Com_FatalError ("Unset entity number");
	if (to->number >= MAX_EDICTS)
		Com_FatalError ("Entity number >= MAX_EDICTS");

	unsigned bits = 0;

	if (to->number >= 256)
		bits |= U_NUMBER16;		// number8 is implicit otherwise

	if (to->origin[0] != from->origin[0])	bits |= U_ORIGIN1;
	if (to->origin[1] != from->origin[1])	bits |= U_ORIGIN2;
	if (to->origin[2] != from->origin[2])	bits |= U_ORIGIN3;

	if (to->angles[0] != from->angles[0])	bits |= U_ANGLE1;
	if (to->angles[1] != from->angles[1])	bits |= U_ANGLE2;
	if (to->angles[2] != from->angles[2])	bits |= U_ANGLE3;

	if (to->skinnum != from->skinnum)
	{
		if ((unsigned)to->skinnum < 256)
			bits |= U_SKIN8;
		else if ((unsigned)to->skinnum < 0x10000)
			bits |= U_SKIN16;
		else
			bits |= (U_SKIN8|U_SKIN16);
	}

	if (to->frame != from->frame)
	{
		if (to->frame < 256)
			bits |= U_FRAME8;
		else
			bits |= U_FRAME16;
	}

	if (to->effects != from->effects)
	{
		if (to->effects < 256)
			bits |= U_EFFECTS8;
		else if (to->effects < 0x8000)
			bits |= U_EFFECTS16;
		else
			bits |= U_EFFECTS8|U_EFFECTS16;
	}

	if (to->renderfx != from->renderfx)
	{
		if (to->renderfx < 256)
			bits |= U_RENDERFX8;
		else if (to->renderfx < 0x8000)
			bits |= U_RENDERFX16;
		else
			bits |= U_RENDERFX8|U_RENDERFX16;
	}

	if (to->solid != from->solid)		bits |= U_SOLID;

	// event is not delta compressed, just 0 compressed
	if (to->event)						bits |= U_EVENT;

	if (to->modelindex != from->modelindex)		bits |= U_MODEL;
	if (to->modelindex2 != from->modelindex2)	bits |= U_MODEL2;
	if (to->modelindex3 != from->modelindex3)	bits |= U_MODEL3;
	if (to->modelindex4 != from->modelindex4)	bits |= U_MODEL4;

	if (to->sound != from->sound)		bits |= U_SOUND;

	if (newentity || (to->renderfx & RF_BEAM))	bits |= U_OLDORIGIN;

	//----------------------------------------------------
	// write the message
	if (!bits && !force) return;		// nothing to send

	// write ent->num

	if (bits & 0xFF000000)
	{
		bits |= U_MOREBITS3 | U_MOREBITS2 | U_MOREBITS1;
		MSG_WriteLong (msg, bits);
	}
	else if (bits & 0x00FF0000)
	{
		bits |= U_MOREBITS2 | U_MOREBITS1;
		MSG_WriteShort (msg, bits);
		MSG_WriteByte (msg, bits >> 16);
	}
	else if (bits & 0x0000FF00)
	{
		bits |= U_MOREBITS1;
		MSG_WriteShort (msg, bits);
	}
	else
		MSG_WriteByte (msg, bits);

	//----------

	if (bits & U_NUMBER16)
		MSG_WriteShort (msg, to->number);
	else
		MSG_WriteByte (msg,	to->number);

	if (bits & U_MODEL)		MSG_WriteByte (msg,	to->modelindex);
	if (bits & U_MODEL2)	MSG_WriteByte (msg,	to->modelindex2);
	if (bits & U_MODEL3)	MSG_WriteByte (msg,	to->modelindex3);
	if (bits & U_MODEL4)	MSG_WriteByte (msg,	to->modelindex4);

	if (bits & U_FRAME8)	MSG_WriteByte (msg, to->frame);
	if (bits & U_FRAME16)	MSG_WriteShort (msg, to->frame);

	if ((bits & U_SKIN8) && (bits & U_SKIN16))		//used for laser colors
		MSG_WriteLong (msg, to->skinnum);
	else if (bits & U_SKIN8)
		MSG_WriteByte (msg, to->skinnum);
	else if (bits & U_SKIN16)
		MSG_WriteShort (msg, to->skinnum);


	if ((bits & (U_EFFECTS8|U_EFFECTS16)) == (U_EFFECTS8|U_EFFECTS16))
		MSG_WriteLong (msg, to->effects);
	else if (bits & U_EFFECTS8)
		MSG_WriteByte (msg, to->effects);
	else if (bits & U_EFFECTS16)
		MSG_WriteShort (msg, to->effects);

	if ((bits & (U_RENDERFX8|U_RENDERFX16)) == (U_RENDERFX8|U_RENDERFX16))
		MSG_WriteLong (msg, to->renderfx);
	else if (bits & U_RENDERFX8)
		MSG_WriteByte (msg, to->renderfx);
	else if (bits & U_RENDERFX16)
		MSG_WriteShort (msg, to->renderfx);

	if (bits & U_ORIGIN1)	MSG_WriteCoord (msg, to->origin[0]);
	if (bits & U_ORIGIN2)	MSG_WriteCoord (msg, to->origin[1]);
	if (bits & U_ORIGIN3)	MSG_WriteCoord (msg, to->origin[2]);

	if (bits & U_ANGLE1) 	MSG_WriteAngle(msg, to->angles[0]);
	if (bits & U_ANGLE2) 	MSG_WriteAngle(msg, to->angles[1]);
	if (bits & U_ANGLE3) 	MSG_WriteAngle(msg, to->angles[2]);

	if (bits & U_OLDORIGIN)
	{
		MSG_WriteCoord (msg, to->old_origin[0]);
		MSG_WriteCoord (msg, to->old_origin[1]);
		MSG_WriteCoord (msg, to->old_origin[2]);
	}

	if (bits & U_SOUND)  	MSG_WriteByte (msg, to->sound);
	if (bits & U_EVENT)  	MSG_WriteByte (msg, to->event);
	if (bits & U_SOLID)  	MSG_WriteShort (msg, to->solid);

	unguard;
}


void MSG_WriteRemoveEntity (sizebuf_t *msg, int num)
{
	if (num >= 256)
	{
		MSG_WriteShort (msg, U_REMOVE|U_NUMBER16|U_MOREBITS1);	// U_NUMBER16 is bit[8], => U_MOREBITS1
		MSG_WriteShort (msg, num);
	}
	else
	{
		MSG_WriteByte (msg, U_REMOVE);
		MSG_WriteByte (msg, num);
	}
}


// Returns the entity number and the header bits
int MSG_ReadEntityBits (sizebuf_t *msg, unsigned *bits, bool *remove)
{
	unsigned b;

	unsigned total = MSG_ReadByte (msg);
	if (total & U_MOREBITS1)
	{
		b = MSG_ReadByte (msg);
		total |= b<<8;
	}
	if (total & U_MOREBITS2)
	{
		b = MSG_ReadByte (msg);
		total |= b<<16;
	}
	if (total & U_MOREBITS3)
	{
		b = MSG_ReadByte (msg);
		total |= b<<24;
	}

	int number = (total & U_NUMBER16) ?  MSG_ReadShort (msg) : MSG_ReadByte (msg);

	*bits = total;
	if (remove) *remove = (total & U_REMOVE) != 0;

	return number;
}


void MSG_ReadDeltaEntity (sizebuf_t *msg, entity_state_t *from, entity_state_t *to, unsigned bits)
{
	guard(MSG_ReadDeltaEntity);

	if (bits & U_MODEL)		to->modelindex = MSG_ReadByte (msg);
	if (bits & U_MODEL2)	to->modelindex2 = MSG_ReadByte (msg);
	if (bits & U_MODEL3)	to->modelindex3 = MSG_ReadByte (msg);
	if (bits & U_MODEL4)	to->modelindex4 = MSG_ReadByte (msg);

	if (bits & U_FRAME8)	to->frame = MSG_ReadByte (msg);
	if (bits & U_FRAME16)	to->frame = MSG_ReadShort (msg);

	if ((bits & U_SKIN8) && (bits & U_SKIN16))		//used for laser colors
		to->skinnum = MSG_ReadLong (msg);
	else if (bits & U_SKIN8)
		to->skinnum = MSG_ReadByte (msg);
	else if (bits & U_SKIN16)
		to->skinnum = MSG_ReadShort (msg);

	if ((bits & (U_EFFECTS8|U_EFFECTS16)) == (U_EFFECTS8|U_EFFECTS16))
		to->effects = MSG_ReadLong (msg);
	else if (bits & U_EFFECTS8)
		to->effects = MSG_ReadByte (msg);
	else if (bits & U_EFFECTS16)
		to->effects = MSG_ReadShort (msg);

	if ((bits & (U_RENDERFX8|U_RENDERFX16)) == (U_RENDERFX8|U_RENDERFX16))
		to->renderfx = MSG_ReadLong (msg);
	else if (bits & U_RENDERFX8)
		to->renderfx = MSG_ReadByte (msg);
	else if (bits & U_RENDERFX16)
		to->renderfx = MSG_ReadShort (msg);

	if (bits & U_ORIGIN1)	to->origin[0] = MSG_ReadCoord (msg);
	if (bits & U_ORIGIN2)	to->origin[1] = MSG_ReadCoord (msg);
	if (bits & U_ORIGIN3)	to->origin[2] = MSG_ReadCoord (msg);

	if (bits & U_ANGLE1)	to->angles[0] = MSG_ReadAngle (msg);
	if (bits & U_ANGLE2)	to->angles[1] = MSG_ReadAngle (msg);
	if (bits & U_ANGLE3)	to->angles[2] = MSG_ReadAngle (msg);

	if (bits & U_OLDORIGIN)	MSG_ReadPos (msg, to->old_origin);
	if (bits & U_SOUND)		to->sound = MSG_ReadByte (msg);
	to->event = bits & U_EVENT ? MSG_ReadByte (msg) : 0;
	if (bits & U_SOLID)		to->solid = MSG_ReadShort (msg);

	unguard;
}


/*-----------------------------------------------------------------------------
	usercmd_t communication
-----------------------------------------------------------------------------*/

#define	CM_ANGLE1 			(1<<0)
#define	CM_ANGLE2		 	(1<<1)
#define	CM_ANGLE3 			(1<<2)
#define	CM_FORWARD			(1<<3)
#define	CM_SIDE				(1<<4)
#define	CM_UP				(1<<5)
#define	CM_BUTTONS			(1<<6)
#define	CM_IMPULSE			(1<<7)


void MSG_WriteDeltaUsercmd (sizebuf_t *buf, usercmd_t *from, usercmd_t *cmd)
{
	int		bits;

	guard(MSG_WriteDeltaUsercmd);

	// send the movement message
	bits = 0;
	if (cmd->angles[0] != from->angles[0])		bits |= CM_ANGLE1;
	if (cmd->angles[1] != from->angles[1])		bits |= CM_ANGLE2;
	if (cmd->angles[2] != from->angles[2])		bits |= CM_ANGLE3;
	if (cmd->forwardmove != from->forwardmove)	bits |= CM_FORWARD;
	if (cmd->sidemove != from->sidemove)		bits |= CM_SIDE;
	if (cmd->upmove != from->upmove)			bits |= CM_UP;
	if (cmd->buttons != from->buttons)			bits |= CM_BUTTONS;
	if (cmd->impulse != from->impulse)			bits |= CM_IMPULSE;

	MSG_WriteByte (buf, bits);

	if (bits & CM_ANGLE1)	MSG_WriteShort (buf, cmd->angles[0]);
	if (bits & CM_ANGLE2)	MSG_WriteShort (buf, cmd->angles[1]);
	if (bits & CM_ANGLE3)	MSG_WriteShort (buf, cmd->angles[2]);

	if (bits & CM_FORWARD)	MSG_WriteShort (buf, cmd->forwardmove);
	if (bits & CM_SIDE)	  	MSG_WriteShort (buf, cmd->sidemove);
	if (bits & CM_UP)		MSG_WriteShort (buf, cmd->upmove);

	if (bits & CM_BUTTONS) 	MSG_WriteByte (buf, cmd->buttons);
	if (bits & CM_IMPULSE)	MSG_WriteByte (buf, cmd->impulse);

	MSG_WriteByte (buf, cmd->msec);
	MSG_WriteByte (buf, cmd->lightlevel);

	unguard;
}


void MSG_ReadDeltaUsercmd (sizebuf_t *msg_read, usercmd_t *from, usercmd_t *move)
{
	guard(MSG_ReadDeltaUsercmd);

	memcpy (move, from, sizeof(*move));

	unsigned bits = MSG_ReadByte (msg_read);

	// read current angles
	if (bits & CM_ANGLE1)	move->angles[0] = MSG_ReadShort (msg_read);
	if (bits & CM_ANGLE2)	move->angles[1] = MSG_ReadShort (msg_read);
	if (bits & CM_ANGLE3)	move->angles[2] = MSG_ReadShort (msg_read);

	// read movement
	if (bits & CM_FORWARD)	move->forwardmove = MSG_ReadShort (msg_read);
	if (bits & CM_SIDE)   	move->sidemove = MSG_ReadShort (msg_read);
	if (bits & CM_UP)     	move->upmove = MSG_ReadShort (msg_read);

	// read buttons
	if (bits & CM_BUTTONS)	move->buttons = MSG_ReadByte (msg_read);

	if (bits & CM_IMPULSE)	move->impulse = MSG_ReadByte (msg_read);

	// read time to run command
	move->msec = MSG_ReadByte (msg_read);

	// read the light level
	move->lightlevel = MSG_ReadByte (msg_read);

	unguard;
}


/*-----------------------------------------------------------------------------
	player_state_t communication
-----------------------------------------------------------------------------*/

#define	PS_M_TYPE			(1<<0)
#define	PS_M_ORIGIN			(1<<1)
#define	PS_M_VELOCITY		(1<<2)
#define	PS_M_TIME			(1<<3)
#define	PS_M_FLAGS			(1<<4)
#define	PS_M_GRAVITY		(1<<5)
#define	PS_M_DELTA_ANGLES	(1<<6)

#define	PS_VIEWOFFSET		(1<<7)
#define	PS_VIEWANGLES		(1<<8)
#define	PS_KICKANGLES		(1<<9)
#define	PS_BLEND			(1<<10)
#define	PS_FOV				(1<<11)
#define	PS_WEAPONINDEX		(1<<12)
#define	PS_WEAPONFRAME		(1<<13)
#define	PS_RDFLAGS			(1<<14)


void MSG_ReadDeltaPlayerstate (sizebuf_t *msg, player_state_t *oldState, player_state_t *newState)
{
	guard(MSG_ReadDeltaPlayerstate);

	// clear to old value before delta parsing
	if (oldState)
		*newState = *oldState;
	else
		memset (newState, 0, sizeof(player_state_t));

	unsigned flags = MSG_ReadShort (msg);

	// parse the pmove_state_t
	if (flags & PS_M_TYPE)		newState->pmove.pm_type = (pmtype_t) MSG_ReadByte (msg);

	if (flags & PS_M_ORIGIN)
	{
		newState->pmove.origin[0] = MSG_ReadShort (msg);
		newState->pmove.origin[1] = MSG_ReadShort (msg);
		newState->pmove.origin[2] = MSG_ReadShort (msg);
	}

	if (flags & PS_M_VELOCITY)
	{
		newState->pmove.velocity[0] = MSG_ReadShort (msg);
		newState->pmove.velocity[1] = MSG_ReadShort (msg);
		newState->pmove.velocity[2] = MSG_ReadShort (msg);
	}

	if (flags & PS_M_TIME)		newState->pmove.pm_time = MSG_ReadByte (msg);
	if (flags & PS_M_FLAGS)		newState->pmove.pm_flags = MSG_ReadByte (msg);
	if (flags & PS_M_GRAVITY)	newState->pmove.gravity = MSG_ReadShort (msg);

	if (flags & PS_M_DELTA_ANGLES)
	{
		newState->pmove.delta_angles[0] = MSG_ReadShort (msg);
		newState->pmove.delta_angles[1] = MSG_ReadShort (msg);
		newState->pmove.delta_angles[2] = MSG_ReadShort (msg);
	}

	// parse the rest of the player_state_t
	if (flags & PS_VIEWOFFSET)
	{
		newState->viewoffset[0] = MSG_ReadChar (msg) * 0.25f;
		newState->viewoffset[1] = MSG_ReadChar (msg) * 0.25f;
		newState->viewoffset[2] = MSG_ReadChar (msg) * 0.25f;
	}

	if (flags & PS_VIEWANGLES)
	{
		newState->viewangles[0] = MSG_ReadAngle16 (msg);
		newState->viewangles[1] = MSG_ReadAngle16 (msg);
		newState->viewangles[2] = MSG_ReadAngle16 (msg);
	}

	if (flags & PS_KICKANGLES)
	{
		newState->kick_angles[0] = MSG_ReadChar (msg) * 0.25f;
		newState->kick_angles[1] = MSG_ReadChar (msg) * 0.25f;
		newState->kick_angles[2] = MSG_ReadChar (msg) * 0.25f;
	}

	if (flags & PS_WEAPONINDEX)
		newState->gunindex = MSG_ReadByte (msg);

	if (flags & PS_WEAPONFRAME)
	{
		newState->gunframe = MSG_ReadByte (msg);
		newState->gunoffset[0] = MSG_ReadChar (msg) * 0.25f;
		newState->gunoffset[1] = MSG_ReadChar (msg) * 0.25f;
		newState->gunoffset[2] = MSG_ReadChar (msg) * 0.25f;
		newState->gunangles[0] = MSG_ReadChar (msg) * 0.25f;
		newState->gunangles[1] = MSG_ReadChar (msg) * 0.25f;
		newState->gunangles[2] = MSG_ReadChar (msg) * 0.25f;
	}

	if (flags & PS_BLEND)
	{
		newState->blend[0] = MSG_ReadByte (msg) / 255.0f;
		newState->blend[1] = MSG_ReadByte (msg) / 255.0f;
		newState->blend[2] = MSG_ReadByte (msg) / 255.0f;
		newState->blend[3] = MSG_ReadByte (msg) / 255.0f;
	}

	if (flags & PS_FOV)		newState->fov = MSG_ReadByte (msg);
	if (flags & PS_RDFLAGS)	newState->rdflags = MSG_ReadByte (msg);

	// parse stats
	unsigned statbits = MSG_ReadLong (msg);
	int		i;
	for (i = 0; statbits; i++, statbits >>= 1)
		if (statbits & 1)
			newState->stats[i] = MSG_ReadShort (msg);

	unguard;
}


void MSG_WriteDeltaPlayerstate (sizebuf_t *msg, player_state_t *oldState, player_state_t *newState)
{
	guard(MSG_WriteDeltaPlayerstate);

	player_state_t nullState;
	if (!oldState)
	{
		memset (&nullState, 0, sizeof(player_state_t));
		oldState = &nullState;
	}

	// determine what needs to be sent
	unsigned flags = 0;

	if (newState->pmove.pm_type != oldState->pmove.pm_type)
		flags |= PS_M_TYPE;

	if (newState->pmove.origin[0] != oldState->pmove.origin[0] ||
		newState->pmove.origin[1] != oldState->pmove.origin[1] ||
		newState->pmove.origin[2] != oldState->pmove.origin[2])
		flags |= PS_M_ORIGIN;

	if (newState->pmove.velocity[0] != oldState->pmove.velocity[0] ||
		newState->pmove.velocity[1] != oldState->pmove.velocity[1] ||
		newState->pmove.velocity[2] != oldState->pmove.velocity[2])
		flags |= PS_M_VELOCITY;

	if (newState->pmove.pm_time != oldState->pmove.pm_time)
		flags |= PS_M_TIME;

	if (newState->pmove.pm_flags != oldState->pmove.pm_flags)
		flags |= PS_M_FLAGS;

	if (newState->pmove.gravity != oldState->pmove.gravity)
		flags |= PS_M_GRAVITY;

	if (newState->pmove.delta_angles[0] != oldState->pmove.delta_angles[0] ||
		newState->pmove.delta_angles[1] != oldState->pmove.delta_angles[1] ||
		newState->pmove.delta_angles[2] != oldState->pmove.delta_angles[2])
		flags |= PS_M_DELTA_ANGLES;

	if (newState->viewoffset[0] != oldState->viewoffset[0] ||
		newState->viewoffset[1] != oldState->viewoffset[1] ||
		newState->viewoffset[2] != oldState->viewoffset[2])
		flags |= PS_VIEWOFFSET;

	if (newState->viewangles[0] != oldState->viewangles[0] ||
		newState->viewangles[1] != oldState->viewangles[1] ||
		newState->viewangles[2] != oldState->viewangles[2])
		flags |= PS_VIEWANGLES;

	if (newState->kick_angles[0] != oldState->kick_angles[0] ||
		newState->kick_angles[1] != oldState->kick_angles[1] ||
		newState->kick_angles[2] != oldState->kick_angles[2])
		flags |= PS_KICKANGLES;

	if (newState->blend[0] != oldState->blend[0] ||
		newState->blend[1] != oldState->blend[1] ||
		newState->blend[2] != oldState->blend[2] ||
		newState->blend[3] != oldState->blend[3])
		flags |= PS_BLEND;

	if (newState->fov != oldState->fov)			flags |= PS_FOV;
	if (newState->rdflags != oldState->rdflags)	flags |= PS_RDFLAGS;
	if (newState->gunframe != oldState->gunframe) flags |= PS_WEAPONFRAME;

	flags |= PS_WEAPONINDEX;		//?? always sent

	//------------------------------
	// write it
	MSG_WriteShort (msg, flags);

	// write the pmove_state_t
	if (flags & PS_M_TYPE)
		MSG_WriteByte (msg, newState->pmove.pm_type);

	if (flags & PS_M_ORIGIN)
	{
		MSG_WriteShort (msg, newState->pmove.origin[0]);
		MSG_WriteShort (msg, newState->pmove.origin[1]);
		MSG_WriteShort (msg, newState->pmove.origin[2]);
	}

	if (flags & PS_M_VELOCITY)
	{
		MSG_WriteShort (msg, newState->pmove.velocity[0]);
		MSG_WriteShort (msg, newState->pmove.velocity[1]);
		MSG_WriteShort (msg, newState->pmove.velocity[2]);
	}

	if (flags & PS_M_TIME)
		MSG_WriteByte (msg, newState->pmove.pm_time);

	if (flags & PS_M_FLAGS)
		MSG_WriteByte (msg, newState->pmove.pm_flags);

	if (flags & PS_M_GRAVITY)
		MSG_WriteShort (msg, newState->pmove.gravity);

	if (flags & PS_M_DELTA_ANGLES)
	{
		MSG_WriteShort (msg, newState->pmove.delta_angles[0]);
		MSG_WriteShort (msg, newState->pmove.delta_angles[1]);
		MSG_WriteShort (msg, newState->pmove.delta_angles[2]);
	}

	// write the rest of the player_state_t
	if (flags & PS_VIEWOFFSET)
	{
		MSG_WriteChar (msg, appRound (newState->viewoffset[0]*4));
		MSG_WriteChar (msg, appRound (newState->viewoffset[1]*4));
		MSG_WriteChar (msg, appRound (newState->viewoffset[2]*4));
	}

	if (flags & PS_VIEWANGLES)
	{
		MSG_WriteAngle16 (msg, newState->viewangles[0]);
		MSG_WriteAngle16 (msg, newState->viewangles[1]);
		MSG_WriteAngle16 (msg, newState->viewangles[2]);
	}

	if (flags & PS_KICKANGLES)
	{
		MSG_WriteChar (msg, appRound (newState->kick_angles[0]*4));
		MSG_WriteChar (msg, appRound (newState->kick_angles[1]*4));
		MSG_WriteChar (msg, appRound (newState->kick_angles[2]*4));
	}

	if (flags & PS_WEAPONINDEX)
	{
		MSG_WriteByte (msg, newState->gunindex);
	}

	if (flags & PS_WEAPONFRAME)
	{
		MSG_WriteByte (msg, newState->gunframe);
		MSG_WriteChar (msg, appRound (newState->gunoffset[0]*4));
		MSG_WriteChar (msg, appRound (newState->gunoffset[1]*4));
		MSG_WriteChar (msg, appRound (newState->gunoffset[2]*4));
		MSG_WriteChar (msg, appRound (newState->gunangles[0]*4));
		MSG_WriteChar (msg, appRound (newState->gunangles[1]*4));
		MSG_WriteChar (msg, appRound (newState->gunangles[2]*4));
	}

	if (flags & PS_BLEND)
	{
		MSG_WriteByte (msg, appRound (newState->blend[0]*255));
		MSG_WriteByte (msg, appRound (newState->blend[1]*255));
		MSG_WriteByte (msg, appRound (newState->blend[2]*255));
		MSG_WriteByte (msg, appRound (newState->blend[3]*255));
	}
	if (flags & PS_FOV)		MSG_WriteByte (msg, appRound (newState->fov));
	if (flags & PS_RDFLAGS) MSG_WriteByte (msg, newState->rdflags);

	// send stats
	unsigned statbits = 0;
	int i;
	for (i = 0; i < MAX_STATS; i++)
		if (newState->stats[i] != oldState->stats[i])
			statbits |= 1<<i;
	MSG_WriteLong (msg, statbits);
	for (i = 0; statbits; i++, statbits >>= 1)
		if (statbits & 1)
			MSG_WriteShort (msg, newState->stats[i]);

	unguard;
}
