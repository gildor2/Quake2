#include "qcommon.h"


void SZ_Init (sizebuf_t *buf, void *data, int length)
{
	memset (buf, 0, sizeof(sizebuf_t));
	buf->data = (byte*)data;
	buf->maxsize = length;
}

void SZ_Clear (sizebuf_t *buf)
{
	buf->cursize = 0;
	buf->overflowed = false;
}

static void *SZ_GetSpace (sizebuf_t *buf, int length)
{
	int need = buf->cursize + length;
	if (need > buf->maxsize)
	{
		if (!buf->allowoverflow)
			Com_FatalError ("SZ_GetSpace: overflow without allowoverflow (size is %d)", buf->maxsize);

		if (length > buf->maxsize)
			Com_FatalError ("SZ_GetSpace: %d is > full buffer size", length);

		Com_Printf ("SZ_GetSpace: overflow (max=%d, need=%d)\n", buf->maxsize, need);
		SZ_Clear (buf);
		buf->overflowed = true;
	}

	void *data = buf->data + buf->cursize;
	buf->cursize = need;

	return data;
}

void SZ_Write (sizebuf_t *buf, const void *data, int length)
{
	memcpy (SZ_GetSpace (buf,length), data, length);
}


/*
==============================================================================

			MESSAGE IO FUNCTIONS

Handles byte ordering and avoids alignment errors
==============================================================================
*/

// writing functions

void MSG_WriteChar (sizebuf_t *sb, int c)
{
	byte *buf = (byte*)SZ_GetSpace (sb, 1);
	buf[0] = c;
}

void MSG_WriteByte (sizebuf_t *sb, int c)
{
	byte *buf = (byte*)SZ_GetSpace (sb, 1);
	buf[0] = c;
}

void MSG_WriteShort (sizebuf_t *sb, int c)
{
	byte *buf = (byte*)SZ_GetSpace (sb, 2);
	//!! optimize for little-endian machines
	buf[0] = c&0xff;
	buf[1] = c>>8;
}

void MSG_WriteLong (sizebuf_t *sb, int c)
{
	byte *buf = (byte*)SZ_GetSpace (sb, 4);
	//!! optimize
	*buf++ = c & 0xff;
	*buf++ = (c>>8) & 0xff;
	*buf++ = (c>>16) & 0xff;
	*buf = c>>24;
}

void MSG_WriteFloat (sizebuf_t *sb, float f)
{
	union {
		float	f;
		int		l;
	} dat;

	dat.f = f;
	dat.l = LittleLong (dat.l);

	SZ_Write (sb, &dat.l, 4);
}

void MSG_WriteString (sizebuf_t *sb, const char *s)
{
	if (!s)	SZ_Write (sb, "", 1);
	else	SZ_Write (sb, s, strlen(s)+1);
}

void MSG_WritePos (sizebuf_t *sb, vec3_t pos)
{
	MSG_WriteShort (sb, appRound (pos[0]*8));
	MSG_WriteShort (sb, appRound (pos[1]*8));
	MSG_WriteShort (sb, appRound (pos[2]*8));
}

void MSG_WriteAngle (sizebuf_t *sb, float f)
{
	MSG_WriteByte (sb, appRound (f*256.0f/360) & 255);
}

void MSG_WriteAngle16 (sizebuf_t *sb, float f)
{
	MSG_WriteShort (sb, ANGLE2SHORT(f));
}


#if 0

static byte dirTable[3*8*8*8];		// value=0 -- uninitialized, else idx+1
	// 3*8 - cube: 6 planes, each subdivided to 4 squares (x<0,x>0,y<0,y>0)
	// 8*8 - coordinates inside square

static int GetDirCell (vec3_t dir)
{
	vec3_t	adir;
	float	m;
	int		sign, base, base2;

	adir[0] = fabs (dir[0]);
	adir[1] = fabs (dir[1]);
	adir[2] = fabs (dir[2]);
	sign = 0;
	if (dir[0] < 0) sign |= 1;
	if (dir[1] < 0) sign |= 2;
	if (dir[2] < 0) sign |= 4;

	if (adir[0] < adir[1])
	{
		base = 1;
		m = adir[1];
	}
	else
	{
		base = 0;
		m = adir[0];
	}
	if (m < adir[2])
	{
		base = 2;
		m = adir[2];
	}

	m = 8.0f / m;
	switch (base)
	{
	case 0:
		base2 = (appFloor (adir[1] * m) << 3) + appFloor (adir[2] * m);
		break;
	case 1:
		base2 = (appFloor (adir[0] * m) << 3) + appFloor (adir[2] * m);
		break;
	case 2:
		base2 = (appFloor (adir[0] * m) << 3) + appFloor (adir[1] * m);
		break;
	}
	return (base << 9) + (sign << 6) + base2;
}

void MSG_WriteDir (sizebuf_t *sb, vec3_t dir)
{
	int		cell, i, best;
	float	d, bestd;

	if (!dir)
	{
		MSG_WriteByte (sb, 0);
		return;
	}
	// find cache cell
	cell = GetDirCell (dir);
	if (dirTable[cell])	// already computed
	{
		MSG_WriteByte (sb, dirTable[cell] - 1);
		Com_Printf("*");
		return;
	}
	// compute index
	bestd = best = 0;
	for (i = 0; i < NUMVERTEXNORMALS; i++)
	{
		d = DotProduct (dir, bytedirs[i]);
		if (d > bestd)
		{
			bestd = d;
			best = i;
		}
	}
	// cache index
	dirTable[cell] = best+1;
	// write
	MSG_WriteByte (sb, best);
}

#else

void MSG_WriteDir (sizebuf_t *sb, vec3_t dir)
{
	if (!dir)
	{
		MSG_WriteByte (sb, 0);
		return;
	}

	float bestd = 0;
	int best = 0;
	for (int i = 0; i < NUMVERTEXNORMALS; i++)
	{
		float d = DotProduct (dir, bytedirs[i]);
		if (d > bestd)
		{
			bestd = d;
			best = i;
		}
	}
	MSG_WriteByte (sb, best);
}

#endif


void MSG_ReadDir (sizebuf_t *sb, vec3_t dir)
{
	int b = MSG_ReadByte (sb);
	if (b >= NUMVERTEXNORMALS)
		Com_DropError ("MSG_ReadDir: out of range");
	VectorCopy (bytedirs[b], dir);
}

//============================================================

//
// reading functions
//

void MSG_BeginReading (sizebuf_t *msg)
{
	msg->readcount = 0;
}

// returns -1 if no more characters are available
int MSG_ReadChar (sizebuf_t *msg_read)
{
	int	c;

	if (msg_read->readcount+1 > msg_read->cursize)
		c = -1;
	else
		c = (signed char)msg_read->data[msg_read->readcount];
	msg_read->readcount++;

	return c;
}

int MSG_ReadByte (sizebuf_t *msg_read)
{
	int	c;

	if (msg_read->readcount+1 > msg_read->cursize)
		c = -1;
	else
		c = (unsigned char)msg_read->data[msg_read->readcount];
	msg_read->readcount++;

	return c;
}

int MSG_ReadShort (sizebuf_t *msg_read)
{
	int	c;

	//!! optimize
	if (msg_read->readcount+2 > msg_read->cursize)
		c = -1;
	else
		c = (short)(msg_read->data[msg_read->readcount] + (msg_read->data[msg_read->readcount+1]<<8));

	msg_read->readcount += 2;

	return c;
}

int MSG_ReadLong (sizebuf_t *msg_read)
{
	int	c;

	//!! optimize
	if (msg_read->readcount+4 > msg_read->cursize)
		c = -1;
	else
		c = msg_read->data[msg_read->readcount]         +
			(msg_read->data[msg_read->readcount+1]<<8)  +
			(msg_read->data[msg_read->readcount+2]<<16) +
			(msg_read->data[msg_read->readcount+3]<<24);

	msg_read->readcount += 4;

	return c;
}

float MSG_ReadFloat (sizebuf_t *msg_read)
{
	union
	{
		byte	b[4];
		float	f;
		int	l;
	} dat;

	if (msg_read->readcount+4 > msg_read->cursize)
		dat.f = -1;
	else
	{
		//!! optimize
		dat.b[0] =	msg_read->data[msg_read->readcount];
		dat.b[1] =	msg_read->data[msg_read->readcount+1];
		dat.b[2] =	msg_read->data[msg_read->readcount+2];
		dat.b[3] =	msg_read->data[msg_read->readcount+3];
	}
	msg_read->readcount += 4;

	dat.l = LittleLong (dat.l);

	return dat.f;
}

char *MSG_ReadString (sizebuf_t *msg_read)
{
	static char	string[2048];		//?? make non-static

	int l = 0;
	do
	{
		int c = MSG_ReadChar (msg_read);
		if (c == -1 || c == 0)
			break;
		string[l++] = c;
	} while (l < sizeof(string)-1);
	string[l] = 0;

	return string;
}

void MSG_ReadPos (sizebuf_t *msg_read, vec3_t pos)
{
	pos[0] = MSG_ReadShort(msg_read) * (1.0f/8);
	pos[1] = MSG_ReadShort(msg_read) * (1.0f/8);
	pos[2] = MSG_ReadShort(msg_read) * (1.0f/8);
}

float MSG_ReadAngle (sizebuf_t *msg_read)
{
	return MSG_ReadChar(msg_read) * (360.0f/256);
}

float MSG_ReadAngle16 (sizebuf_t *msg_read)
{
	return SHORT2ANGLE(MSG_ReadShort(msg_read));
}

void MSG_ReadData (sizebuf_t *msg_read, void *data, int len)
{
	for (int i = 0; i < len; i++)
		((byte *)data)[i] = MSG_ReadByte (msg_read);
}
