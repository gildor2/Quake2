#include "qcommon.h"


#define NET_POS_SCALER	8	// float pos * SCALER -> int pos


/*-----------------------------------------------------------------------------
	Debug tools
-----------------------------------------------------------------------------*/

#if 0

#define TEMPLATE(Name,type,fmt)						\
static void M_Write##Name (sizebuf_t *m, type c)	\
{													\
	Com_Printf (S_GREEN"w" fmt "\n", c);			\
	MSG_Write##Name (m, c);							\
}													\
static type M_Read##Name (sizebuf_t *m)				\
{													\
	type c = MSG_Read##Name (m);					\
	Com_Printf (S_RED"r" fmt "\n", c);				\
	return c;										\
}

TEMPLATE(Char,char,"c %02X")
TEMPLATE(Byte,byte,"b %02X")
TEMPLATE(Short,short,"w %04X")
TEMPLATE(Long,int,"d %08X")
TEMPLATE(Float,float,"f %g")
TEMPLATE(Angle,float,"c %g")
TEMPLATE(Angle16,float,"c %g")

#undef TEMPLATE

#define MSG_WriteChar		M_WriteChar
#define MSG_WriteByte		M_WriteByte
#define MSG_WriteShort		M_WriteShort
#define MSG_WriteLong		M_WriteLong
#define MSG_WriteFloat		M_WriteFloat
#define MSG_WriteAngle		M_WriteAngle
#define MSG_WriteAngle16	M_WriteAngle16

#define MSG_ReadChar		M_ReadChar
#define MSG_ReadByte		M_ReadByte
#define MSG_ReadShort		M_ReadShort
#define MSG_ReadLong		M_ReadLong
#define MSG_ReadFloat		M_ReadFloat
#define MSG_ReadAngle		M_ReadAngle
#define MSG_ReadAngle16		M_ReadAngle16

#define LOG(str)	Com_Printf(str)

#else
#define LOG(str)
#endif

/*-----------------------------------------------------------------------------
	Processing delta from different structures
-----------------------------------------------------------------------------*/

// type information

//!! NOTE: scaled float values can be compared as scaled ints (i.e. with scale precision)

enum {
	// simple types
	CHAR, BYTE, SHORT,
	ANGLE, ANGLE16,			// float->byte/short; uses MSG_WriteAngle/MSG_WriteAngle16
	FCHAR, FBYTE, FSHORT,	// float * extra2 -> char/byte/short + extra1
	// complex types
	PSHORT,					// 0..0xFF - byte + extra1, >=0x100 - short + extra2
	PINT,					// 0..0xFF - byte + extra1, 0x100..0xFFFF - short + extra2, >=0x10000 - long + extra1+extra2
	PINT2					// 0..0xFF - byte + extra1, 0x100..0x7FFF - short + extra2, >=0x8000 - long + extra1+extra2;
							//   != PINT, required to avoid MSG_ReadShort() sign extension when number is 0x8000..0xFFFF
};

static const byte typeSize[] = {
	1, 1, 2,
	4, 4,
	4, 4, 4,
	2,
	4, 4
};

// delta info
typedef struct
{
	byte	offset;
	byte	type;
	byte	extra1;
	byte	extra2;
} deltaInfo_t;
#define N(field, type, extra1, extra2)	\
	{ FIELD2OFS(STRUC,field), type, extra1, extra2 }


// parsing delta
//!! function will be smaller, if use msg->Serialize(&data) -- single function for reading/writting
static unsigned ParseDelta (const void *prev, void *next, const deltaInfo_t *info, int count, unsigned bits, sizebuf_t *w, sizebuf_t *r)
{
	unsigned ret = 0;

	guard(ParseDelta);

	for ( ; count; count--, info++)
	{
		void *field = (byte*)next + info->offset; // OffsetPointer (next, info->offset);
		if (!w && !r && !memcmp ((byte*)prev + info->offset, field, typeSize[info->type])) continue;

#define F(type)		*((type*)field)
		unsigned m = 0;
		switch (info->type)
		{
		case CHAR:
		case BYTE:
		case SHORT:
		case ANGLE:
		case ANGLE16:
		case FCHAR:
		case FBYTE:
		case FSHORT:
			m = 1 << info->extra1;
			if (bits & m)
			{
				switch (info->type)
				{
				case CHAR:
					if (w) MSG_WriteChar (w, F(char));
					if (r) F(char) = MSG_ReadChar (r);
					break;
				case BYTE:
					if (w) MSG_WriteByte (w, F(byte));
					if (r) F(byte) = MSG_ReadByte (r);
					break;
				case SHORT:
					if (w) MSG_WriteShort (w, F(short));
					if (r) F(short) = MSG_ReadShort (r);
					break;
				case ANGLE:
					if (w) MSG_WriteAngle (w, F(float));
					if (r) F(float) = MSG_ReadAngle (r);
					break;
				case ANGLE16:
					if (w) MSG_WriteAngle16 (w, F(float));
					if (r) F(float) = MSG_ReadAngle16 (r);
					break;
				case FCHAR:
					if (w) MSG_WriteChar (w, appRound (F(float) * info->extra2));
					if (r) F(float) = (float) MSG_ReadChar (r) / info->extra2;
					break;
				case FBYTE:
					if (w) MSG_WriteByte (w, appRound (F(float) * info->extra2));
					if (r) F(float) = (float) MSG_ReadByte (r) / info->extra2;
					break;
				case FSHORT:
					if (w) MSG_WriteShort (w, appRound (F(float) * info->extra2));
					if (r) F(float) = (float) MSG_ReadShort (r) / info->extra2;
					break;
				}
			}
			break;

		case PSHORT:
			{
				// NOTE: PSHORT code is VERY similar to PINT, but using F(short) instead of F(int) ...
				unsigned short d = F(unsigned short);
				int e1 = 1<<info->extra1;
				int e2 = 1<<info->extra2;
				m = (d < 256) ? e1 : e2;

				if (bits & e1)
				{
					if (w) MSG_WriteByte (w, d);
					if (r) F(short) = MSG_ReadByte (r);
				}
				else if (bits & e2)
				{
					if (w) MSG_WriteShort (w, d);
					if (r) F(short) = MSG_ReadShort (r);
				}
			}
			break;

		case PINT:
		case PINT2:
			{
				unsigned d = F(unsigned);
				int e1 = 1<<info->extra1;
				int e2 = 1<<info->extra2;
				if (d < 0x100)
					m = e1;
				else if ((d < 0x10000 && info->type == PINT) || (d < 0x8000 && info->type == PINT2))
					m = e2;
				else
					m = e1 | e2;

				int chk = bits & (e1|e2);
				if (chk == e1) {
					if (w) MSG_WriteByte (w, d);
					if (r) F(unsigned) = MSG_ReadByte (r);
				} else if (chk == e2) {
					if (w) MSG_WriteShort (w, d);
					if (r) F(unsigned) = (unsigned short)MSG_ReadShort (r);	// without "unsigned short"
				} else if (chk == (e1|e2)) {
					if (w) MSG_WriteLong (w, d);
					if (r) F(unsigned) = MSG_ReadLong (r);
				}
			}
			break;
		}
		ret |= m;
	}
#undef F

	unguard;

	return ret;
}


inline unsigned ComputeDeltaBits (const void *prev, void *next, const deltaInfo_t *info, int count)
{
	return ParseDelta (prev, next, info, count, 0, NULL, NULL);
}

inline void WriteDelta (const void *prev, void *next, const deltaInfo_t *info, int count, unsigned bits, sizebuf_t *msg)
{
	// NOTE: "prev" unused
	ParseDelta (NULL, next, info, count, bits, msg, NULL);
}

inline void ReadDelta (const void *prev, void *next, const deltaInfo_t *info, int count, unsigned bits, sizebuf_t *msg)
{
	ParseDelta (NULL, next, info, count, bits, NULL, msg);
}


/*-----------------------------------------------------------------------------
	entity_state_t communication
-----------------------------------------------------------------------------*/

//?? can make as enum
// first byte
#define	U_ORIGIN1			0
#define	U_ORIGIN2			1
#define	U_ANGLE2			2
#define	U_ANGLE3			3
#define	U_FRAME8			4		// frame is a byte
#define	U_EVENT				5
#define	U_REMOVE			6		// REMOVE this entity, don't add it
#define	U_MOREBITS1			7		// read one additional byte

// second byte
#define	U_NUMBER16			8		// NUMBER8 is implicit if not set
#define	U_ORIGIN3			9
#define	U_ANGLE1			10
#define	U_MODEL				11
#define U_RENDERFX8			12		// fullbright, etc
#define	U_EFFECTS8			14		// autorotate, trails, etc
#define	U_MOREBITS2			15		// read one additional byte

// third byte
#define	U_SKIN8				16
#define	U_FRAME16			17		// frame is a short
#define	U_RENDERFX16		18		// 8 + 16 = 32
#define	U_EFFECTS16			19		// 8 + 16 = 32
#define	U_MODEL2			20		// weapons, flags, etc
#define	U_MODEL3			21
#define	U_MODEL4			22
#define	U_MOREBITS3			23		// read one additional byte

// fourth byte
#define	U_OLDORIGIN			24		// FIXME: get rid of this ??
#define	U_SKIN16			25
#define	U_SOUND				26
#define	U_SOLID				27


#define STRUC	entity_state_t
static const deltaInfo_t entityStateDelta [] = {
N(	modelindex,		BYTE,	U_MODEL, 0  ),
N(	modelindex2,	BYTE,	U_MODEL2, 0  ),
N(	modelindex3,	BYTE,	U_MODEL3, 0  ),
N(	modelindex4,	BYTE,	U_MODEL4, 0  ),
N(	frame,			PSHORT,	U_FRAME8, U_FRAME16  ),
N(	skinnum,		PINT,	U_SKIN8, U_SKIN16  ),
N(	effects,		PINT2,	U_EFFECTS8, U_EFFECTS16  ),
N(	renderfx,		PINT2,	U_RENDERFX8, U_RENDERFX16  ),
N(	origin[0],		FSHORT, U_ORIGIN1, NET_POS_SCALER  ),
N(	origin[1],		FSHORT, U_ORIGIN2, NET_POS_SCALER  ),
N(	origin[2],		FSHORT, U_ORIGIN3, NET_POS_SCALER  ),
N(	angles[0],		ANGLE,	U_ANGLE1, 0  ),
N(	angles[1],		ANGLE,	U_ANGLE2, 0  ),
N(	angles[2],		ANGLE,	U_ANGLE3, 0  ),
N(	old_origin[0],	FSHORT, U_OLDORIGIN, NET_POS_SCALER  ),	// special
N(	old_origin[1],	FSHORT, U_OLDORIGIN, NET_POS_SCALER  ),
N(	old_origin[2],	FSHORT, U_OLDORIGIN, NET_POS_SCALER  ),
N(	sound,			BYTE,	U_SOUND, 0  ),
N(	event,			BYTE,	U_EVENT, 0  ),					// special: delta from 0
N(	solid,			SHORT,	U_SOLID, 0  )
};
#undef STRUC


void MSG_WriteDeltaEntity (sizebuf_t *msg, const entity_state_t *from, entity_state_t *to, bool force, bool newentity)
{
	guard(MSG_WriteDeltaEntity);

	if (!to->number)
		Com_FatalError ("Unset entity number");
	if (to->number >= MAX_EDICTS)
		Com_FatalError ("Entity number >= MAX_EDICTS");

	LOG("write delta entity\n");

	unsigned bits = ComputeDeltaBits (from, to, ARRAY_ARG(entityStateDelta));
	if (to->number >= 256) bits |= 1<<U_NUMBER16;

	// HACKS:
	bits &= ~((1<<U_OLDORIGIN)|(1<<U_EVENT));
	// event is not delta compressed, just 0 compressed
	if (to->event)
		bits |= 1<<U_EVENT;
	// send old_origin for beams and for new entities only
	if (newentity || (to->renderfx & 0x80)) //!! RF_BEAM
		bits |= 1<<U_OLDORIGIN;

	//----------------------------------------------------
	// write the message
	if (!bits && !force) return;		// nothing to send

	// write ent->num

	if (bits & 0xFF000000)
		MSG_WriteLong (msg, bits | 0x808080);
	else if (bits & 0x00FF0000)
	{
		MSG_WriteShort (msg, bits | 0x8080);
		MSG_WriteByte (msg, bits >> 16);
	}
	else if (bits & 0x0000FF00)
		MSG_WriteShort (msg, bits | 0x80);
	else
		MSG_WriteByte (msg, bits);

	if (bits & (1<<U_NUMBER16))
		MSG_WriteShort (msg, to->number);
	else
		MSG_WriteByte (msg, to->number);

	WriteDelta (from, to, ARRAY_ARG(entityStateDelta), bits, msg);

	LOG("----------\n");

	unguard;
}


void MSG_WriteRemoveEntity (sizebuf_t *msg, int num)
{
	if (num >= 256)
	{
		MSG_WriteShort (msg, (1<<U_REMOVE)|(1<<U_NUMBER16)|(1<<U_MOREBITS1));	// U_NUMBER16 is bit[8], => U_MOREBITS1
		MSG_WriteShort (msg, num);
	}
	else
	{
		MSG_WriteByte (msg, (1<<U_REMOVE));
		MSG_WriteByte (msg, num);
	}
}


// Returns the entity number and the header bits
int MSG_ReadEntityBits (sizebuf_t *msg, unsigned *bits, bool *remove)
{
	unsigned b;

	LOG("read entity bits\n");

	unsigned total = MSG_ReadByte (msg);
	if (total & 0x80)
	{
		b = MSG_ReadByte (msg);
		total |= b<<8;
	}
	if (total & 0x8000)
	{
		b = MSG_ReadByte (msg);
		total |= b<<16;
	}
	if (total & 0x800000)
	{
		b = MSG_ReadByte (msg);
		total |= b<<24;
	}

	int number = (total & (1<<U_NUMBER16)) ?  MSG_ReadShort (msg) : MSG_ReadByte (msg);

	*bits = total;
	if (remove) *remove = (total & (1<<U_REMOVE)) != 0;

	LOG("----------\n");

	return number;
}


void MSG_ReadDeltaEntity (sizebuf_t *msg, const entity_state_t *from, entity_state_t *to, unsigned bits)
{
	guard(MSG_ReadDeltaEntity);

	LOG("read delta entity\n");
	*to = *from;
	VectorCopy (from->origin, to->old_origin);
	to->event = 0;		// dalta from zero

	ReadDelta (from, to, ARRAY_ARG(entityStateDelta), bits, msg);

	LOG("----------\n");

	unguard;
}


/*-----------------------------------------------------------------------------
	usercmd_t communication
-----------------------------------------------------------------------------*/

#define	CM_ANGLE1 			0
#define	CM_ANGLE2		 	1
#define	CM_ANGLE3 			2
#define	CM_FORWARD			3
#define	CM_SIDE				4
#define	CM_UP				5
#define	CM_BUTTONS			6
#define	CM_IMPULSE			7

#define STRUC	usercmd_t
static const deltaInfo_t userCmdDelta [] = {
N(	angles[0],		SHORT,	CM_ANGLE1, 0  ),
N(	angles[1],		SHORT,	CM_ANGLE2, 0  ),
N(	angles[2],		SHORT,	CM_ANGLE3, 0  ),
N(	forwardmove,	SHORT,	CM_FORWARD, 0  ),
N(	sidemove,		SHORT,	CM_SIDE, 0  ),
N(	upmove,			SHORT,	CM_UP, 0  ),
N(	buttons,		BYTE,	CM_BUTTONS, 0  ),
N(	impulse,		BYTE,	CM_IMPULSE, 0  ),
N(	msec,			BYTE,	31, 0  ),			// always
N(	lightlevel,		BYTE,	31, 0  )			// always
};
#undef STRUC


void MSG_WriteDeltaUsercmd (sizebuf_t *msg, const usercmd_t *from, usercmd_t *to)
{
	guard(MSG_WriteDeltaUsercmd);

	LOG("write delta usercmd\n");

	unsigned bits = ComputeDeltaBits (from, to, ARRAY_ARG(userCmdDelta));
	MSG_WriteByte (msg, bits);
	WriteDelta (from, to, ARRAY_ARG(userCmdDelta), bits | (1<<31), msg);	// 31 bit - for msec and lightlevel

	LOG("----------\n");

	unguard;
}


void MSG_ReadDeltaUsercmd (sizebuf_t *msg, const usercmd_t *from, usercmd_t *to)
{
	guard(MSG_ReadDeltaUsercmd);

	LOG("read delta usercmd\n");

	*to = *from;

	unsigned bits = MSG_ReadByte (msg);
	ReadDelta (from, to, ARRAY_ARG(userCmdDelta), bits | (1<<31), msg);

	LOG("----------\n");

	unguard;
}


/*-----------------------------------------------------------------------------
	player_state_t communication
-----------------------------------------------------------------------------*/

#define	PS_M_TYPE			0
#define	PS_M_ORIGIN			1
#define	PS_M_VELOCITY		2
#define	PS_M_TIME			3
#define	PS_M_FLAGS			4
#define	PS_M_GRAVITY		5
#define	PS_M_DELTA_ANGLES	6

#define	PS_VIEWOFFSET		7
#define	PS_VIEWANGLES		8
#define	PS_KICKANGLES		9
#define	PS_BLEND			10
#define	PS_FOV				11
#define	PS_WEAPONINDEX		12
#define	PS_WEAPONFRAME		13
#define	PS_RDFLAGS			14

#define STRUC player_state_t
static const deltaInfo_t playerStateDelta [] = {
// pmove_state_t
N(	pmove.pm_type,	BYTE,	PS_M_TYPE, 0  ),
N(	pmove.origin[0],SHORT,	PS_M_ORIGIN, 0  ),
N(	pmove.origin[1],SHORT,	PS_M_ORIGIN, 0  ),
N(	pmove.origin[2],SHORT,	PS_M_ORIGIN, 0  ),
N(	pmove.velocity[0], SHORT,	PS_M_VELOCITY, 0  ),
N(	pmove.velocity[1], SHORT,	PS_M_VELOCITY, 0  ),
N(	pmove.velocity[2], SHORT,	PS_M_VELOCITY, 0  ),
N(	pmove.pm_time,	BYTE,	PS_M_TIME, 0  ),
N(	pmove.pm_flags,	BYTE,	PS_M_FLAGS, 0  ),
N(	pmove.gravity,	SHORT,	PS_M_GRAVITY, 0  ),
N(	pmove.delta_angles[0], SHORT, PS_M_DELTA_ANGLES, 0  ),
N(	pmove.delta_angles[1], SHORT, PS_M_DELTA_ANGLES, 0  ),
N(	pmove.delta_angles[2], SHORT, PS_M_DELTA_ANGLES, 0  ),
// rest of the player_state_t
N(	viewoffset[0],	FCHAR,	PS_VIEWOFFSET, 4  ),
N(	viewoffset[1],	FCHAR,	PS_VIEWOFFSET, 4  ),
N(	viewoffset[2],	FCHAR,	PS_VIEWOFFSET, 4  ),
N(	viewangles[0],	ANGLE16, PS_VIEWANGLES, 0  ),
N(	viewangles[1],	ANGLE16, PS_VIEWANGLES, 0  ),
N(	viewangles[2],	ANGLE16, PS_VIEWANGLES, 0  ),
N(	kick_angles[0],	FCHAR,	PS_KICKANGLES, 4  ),
N(	kick_angles[1],	FCHAR,	PS_KICKANGLES, 4  ),
N(	kick_angles[2],	FCHAR,	PS_KICKANGLES, 4  ),
N(	gunindex,		BYTE,	PS_WEAPONINDEX, 0  ),
N(	gunframe,		BYTE,	PS_WEAPONFRAME, 0  ),
N(	gunoffset[0],	FCHAR,	PS_WEAPONFRAME, 4  ),
N(	gunoffset[1],	FCHAR,	PS_WEAPONFRAME, 4  ),
N(	gunoffset[2],	FCHAR,	PS_WEAPONFRAME, 4  ),
N(	gunangles[0],	FCHAR,	PS_WEAPONFRAME, 4  ),
N(	gunangles[1],	FCHAR,	PS_WEAPONFRAME, 4  ),
N(	gunangles[2],	FCHAR,	PS_WEAPONFRAME, 4  ),
N(	blend[0],		FBYTE,	PS_BLEND, 255  ),
N(	blend[1],		FBYTE,	PS_BLEND, 255  ),
N(	blend[2],		FBYTE,	PS_BLEND, 255  ),
N(	blend[3],		FBYTE,	PS_BLEND, 255  ),
N(	fov,			FBYTE,	PS_FOV, 1  ),
N(	rdflags,		BYTE,	PS_RDFLAGS, 0  )
};
#undef STRUC


void MSG_WriteDeltaPlayerstate (sizebuf_t *msg, const player_state_t *oldState, player_state_t *newState)
{
	guard(MSG_WriteDeltaPlayerstate);

	LOG("write delta playerstate\n");

	player_state_t nullState;
	if (!oldState)
	{
		memset (&nullState, 0, sizeof(player_state_t));
		oldState = &nullState;
	}

	unsigned bits = ComputeDeltaBits (oldState, newState, ARRAY_ARG(playerStateDelta));
	bits |= 1<<PS_WEAPONINDEX;		//?? always sent

	//------------------------------
	// write it
	MSG_WriteShort (msg, bits);

	WriteDelta (oldState, newState, ARRAY_ARG(playerStateDelta), bits, msg);

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

	LOG("----------\n");

	unguard;
}


void MSG_ReadDeltaPlayerstate (sizebuf_t *msg, const player_state_t *oldState, player_state_t *newState)
{
	guard(MSG_ReadDeltaPlayerstate);

	LOG("read delta playerstate\n");

	// clear to old value before delta parsing
	if (oldState)
		*newState = *oldState;
	else
		memset (newState, 0, sizeof(player_state_t));

	unsigned bits = MSG_ReadShort (msg);
	ReadDelta (oldState, newState, ARRAY_ARG(playerStateDelta), bits, msg);

	// parse stats
	unsigned statbits = MSG_ReadLong (msg);
	int		i;
	for (i = 0; statbits; i++, statbits >>= 1)
		if (statbits & 1)
			newState->stats[i] = MSG_ReadShort (msg);

	LOG("----------\n");

	unguard;
}
